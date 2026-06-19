#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RegisterAllSystems.hpp"
#include "openvic-simulation/ecs_rgo/RgoMath.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

// Test fixture for the parallel-reference RGO. Builds a deterministic World with a configurable
// number of states / provinces / pops, populates the singletons (production-type registry,
// market price table, defines, game rules), wires up state↔province↔pop topology, computes
// size_multiplier + max_employee_count via RgoMath kernels (mirroring the legacy
// initialise_rgo_size_multiplier path), and registers every RGO system.
//
// Header-only — the fixture is consumed inline by each TEST_CASE that wants a working World.
// Returns a `BuiltWorld` struct that owns the World and exposes the EntityID lists for
// assertion purposes.

namespace OpenVic::ecs_rgo::test_support {

	// Plain test-pop description used by the fixture builder. Pop ordering inside each
	// province follows the order supplied here.
	struct TestPop {
		pop_type_idx_t pop_type_idx = INVALID_IDX;
		pop_size_t size = 0;
		bool is_in_state_owner_list = false; // mirrors "is the owner pop type for some RGO"
	};

	// Plain test-province description. `production_type_idx == INVALID_IDX` keeps the RGO
	// disabled — exercises the Stage-1 / Stage-2 / Stage-3 early-out paths.
	struct TestProvince {
		production_type_idx_t production_type_idx = INVALID_IDX;
		uint32_t state_idx = 0;
		std::vector<TestPop> pops;
		// Modifier baselines — copied verbatim into the province's modifier components.
		ProvinceRgoModifiers mods {};
		ProvinceRgoFarmMineModifiers fm {};
		ProvinceRgoGoodModifiers gm {};
	};

	// Plain test-state description — a list of state-indexed provinces is enough, since
	// pops live in provinces.
	struct TestState {
		std::vector<uint32_t> province_indices;
	};

	struct BuiltWorld {
		ecs::World world;

		// Indexed by their order in TestProvince/TestState vectors.
		std::vector<ecs::EntityID> province_entities;
		std::vector<ecs::EntityID> state_entities;
		// Flat list of all pop entities, in (province, pop) creation order.
		std::vector<ecs::EntityID> pop_entities;
		// pop_entities[pop_entity_start_per_province[i] .. pop_entity_start_per_province[i+1])
		// belongs to province i.
		std::vector<std::size_t> pop_entity_start_per_province;
	};

	inline std::unique_ptr<BuiltWorld> build_world(
		std::vector<ProductionTypeDef> production_types,
		std::vector<fixed_point_t> unit_prices,
		std::vector<TestState> const& states,
		std::vector<TestProvince> const& provinces,
		std::size_t pop_type_count,
		bool use_simple_farm_mine_logic,
		uint32_t worker_count
	) {
		auto built = std::make_unique<BuiltWorld>();
		built->world.set_ecs_worker_count(worker_count);

		// Singletons first — owned by the World, lifetime is the World's lifetime.
		RgoProductionTypeRegistry registry;
		registry.production_types = std::move(production_types);
		built->world.set_singleton<RgoProductionTypeRegistry>(std::move(registry));

		RgoMarketPriceTable prices;
		prices.unit_price = std::move(unit_prices);
		built->world.set_singleton<RgoMarketPriceTable>(std::move(prices));

		built->world.set_singleton<RgoEconomyDefines>();
		RgoGameRules rules;
		rules.use_simple_farm_mine_logic = use_simple_farm_mine_logic;
		built->world.set_singleton<RgoGameRules>(std::move(rules));

		// Create state entities first (each pop / province needs to reference its state).
		built->state_entities.reserve(states.size());
		for (std::size_t s = 0; s < states.size(); ++s) {
			StateOwnerPopList sop;
			sop.pops_by_type.resize(pop_type_count);
			sop.total_population = 0;
			ecs::EntityID const sid = built->world.create_entity(
				StateProvinceList {},
				std::move(sop)
			);
			built->state_entities.push_back(sid);
		}

		// Create province + pop entities in interleaved order so within-state province
		// iteration is contiguous-by-index. Pops are created right after their owning
		// province to keep pop_entities[] in (province, pop)-order.
		built->province_entities.reserve(provinces.size());
		built->pop_entity_start_per_province.reserve(provinces.size() + 1);
		built->pop_entity_start_per_province.push_back(0);

		// Per-state EntityID accumulators — filled inline.
		std::vector<std::vector<ecs::EntityID>> state_provinces(states.size());
		std::vector<std::vector<std::vector<ecs::EntityID>>> state_owner_pops(states.size());
		for (auto& v : state_owner_pops) {
			v.resize(pop_type_count);
		}
		std::vector<pop_sum_t> state_total_pop(states.size(), 0);

		RgoProductionTypeRegistry const* registry_ptr =
			built->world.get_singleton<RgoProductionTypeRegistry>();
		RgoGameRules const* rules_ptr = built->world.get_singleton<RgoGameRules>();

		for (std::size_t p = 0; p < provinces.size(); ++p) {
			TestProvince const& prov = provinces[p];
			ecs::EntityID const state_eid = built->state_entities[prov.state_idx];

			// Size multiplier + max_employee_count are computed once at fixture build, exactly
			// as the legacy `initialise_rgo_size_multiplier` does.
			fixed_point_t size_multiplier = fixed_point_t::_0;
			pop_size_t max_employee_count = 0;
			pop_sum_t total_workers_in_province = 0;
			if (prov.production_type_idx != INVALID_IDX
				&& prov.production_type_idx < registry_ptr->production_types.size()) {
				ProductionTypeDef const& pt =
					registry_ptr->production_types[prov.production_type_idx];
				for (TestPop const& pop : prov.pops) {
					for (Job const& job : pt.jobs) {
						if (pop.pop_type_idx == job.pop_type_idx) {
							total_workers_in_province += static_cast<pop_sum_t>(pop.size);
							break;
						}
					}
				}
				fixed_point_t const size_modifier =
					detail::calculate_size_modifier(pt, prov.fm, prov.gm, *rules_ptr);
				size_multiplier = detail::calculate_size_multiplier_from_workforce(
					total_workers_in_province, pt.base_workforce_size, size_modifier
				);
				max_employee_count = detail::calculate_max_employee_count(
					size_modifier, size_multiplier, pt.base_workforce_size
				);
			}

			// Pre-reserve worst-case capacity on per-tick vectors so the hot path never
			// reallocates — this is the "pre-attach output components at create_entity time"
			// pitfall from ECS.md.
			ProvinceRgoHired hired;
			hired.employees.reserve(prov.pops.size());
			hired.employee_count_per_type.assign(pop_type_count, 0);
			hired.max_employee_count = max_employee_count;

			ProvinceRgoEmployeeIncome emp_income;
			emp_income.incomes.reserve(prov.pops.size());

			ProvincePopList pop_list;
			pop_list.pop_ids.reserve(prov.pops.size());

			// Country idx: stub — for the reference impl, each state's first province writes a
			// non-INVALID idx so the "no owner country" early-out doesn't fire universally.
			// Treat the state_idx itself as a placeholder country idx; the legacy uses a
			// CountryInstance* but our market resolver doesn't actually consume it.
			ProvinceLocation loc {};
			loc.state_id = state_eid;
			// owner_country_id: distinct from default-constructed EntityID so the Stage-3 owner
			// check doesn't reject the province. Reuses the state entity as a stand-in country
			// (no country-instance data is consumed by the reference impl).
			loc.owner_country_id = state_eid;
			loc.country_to_report_economy_idx = prov.state_idx;

			ProvinceRgoConfig cfg {};
			cfg.production_type_idx = prov.production_type_idx;
			cfg.size_multiplier = size_multiplier;

			ProvinceRgoSellOrder order {};
			ProvinceRgoResult result {};
			ProvinceRgoOwnerIncome owner_income {};
			ProvinceRgoCacheTotals cache_totals {};

			ecs::EntityID const prov_eid = built->world.create_entity(
				std::move(cfg),
				std::move(cache_totals),
				std::move(hired),
				std::move(order),
				std::move(result),
				std::move(owner_income),
				std::move(emp_income),
				std::move(loc),
				std::move(pop_list),
				ProvinceRgoModifiers { prov.mods },
				ProvinceRgoFarmMineModifiers { prov.fm },
				ProvinceRgoGoodModifiers { prov.gm }
			);
			built->province_entities.push_back(prov_eid);
			state_provinces[prov.state_idx].push_back(prov_eid);

			// Pops. Their PopLocation references the just-created province + the state. Each
			// pop carries the four pop-side components pre-attached (PopCore, PopLocation,
			// PopWorkerIncome, PopOwnerIncome, PopIncomeTotals).
			for (TestPop const& tp : prov.pops) {
				PopCore core {};
				core.pop_type_idx = tp.pop_type_idx;
				core.size = tp.size;

				PopLocation pl {};
				pl.province_id = prov_eid;
				pl.state_id = state_eid;

				ecs::EntityID const pop_eid = built->world.create_entity(
					std::move(core),
					std::move(pl),
					PopWorkerIncome {},
					PopOwnerIncome {},
					PopIncomeTotals {}
				);
				built->pop_entities.push_back(pop_eid);
				if (tp.pop_type_idx < pop_type_count) {
					state_owner_pops[prov.state_idx][tp.pop_type_idx].push_back(pop_eid);
				}
				state_total_pop[prov.state_idx] += static_cast<pop_sum_t>(tp.size);
			}
			built->pop_entity_start_per_province.push_back(built->pop_entities.size());

			// Now write the province's pop list — referenced by every system that needs to
			// walk pops by province.
			ProvincePopList* ppl_out = built->world.get_component<ProvincePopList>(prov_eid);
			for (std::size_t i = built->pop_entity_start_per_province[p];
				 i < built->pop_entity_start_per_province[p + 1]; ++i) {
				ppl_out->pop_ids.push_back(built->pop_entities[i]);
			}
		}

		// Backfill StateProvinceList + StateOwnerPopList.total_population now that pop
		// totals are known.
		for (std::size_t s = 0; s < states.size(); ++s) {
			StateProvinceList* spl =
				built->world.get_component<StateProvinceList>(built->state_entities[s]);
			spl->province_ids = std::move(state_provinces[s]);

			StateOwnerPopList* sop =
				built->world.get_component<StateOwnerPopList>(built->state_entities[s]);
			sop->total_population = state_total_pop[s];
			sop->pops_by_type = std::move(state_owner_pops[s]);
		}

		register_all_rgo_systems(built->world);
		return built;
	}
}
