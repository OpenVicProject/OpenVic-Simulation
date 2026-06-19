#pragma once

#include <cstdint>
#include <vector>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

// Components for the parallel-reference RGO implementation. Every component is pre-attached to
// its host entity at construction time — no in-tick add_component / remove_component migrations.
// `production_type_idx == INVALID_IDX` (in ProvinceRgoConfig) is the early-out sentinel: a
// disabled RGO entity keeps its components but every system skips it.
//
// Sibling components are split deliberately so disjoint Stage-N systems can write disjoint
// components in parallel — see RGO.md / ECS.md for the rationale. ProvinceRgoCacheTotals is
// peeled out of ProvinceRgoHired so Stage 1 can write the narrow totals component without
// touching the hire output; ProvinceRgoResult.owner_share is cached at Stage 4 so Stages 5a/5b
// can run sibling-parallel without serialising on each other.

namespace OpenVic::ecs_rgo {

	// ============================================================================
	// Province components
	// ============================================================================

	// Per-province RGO configuration. production_type_idx == INVALID_IDX disables this RGO.
	// size_multiplier is recomputed once at fixture build (mirrors the legacy
	// initialise_rgo_size_multiplier) — never mutated by the tick path.
	struct ProvinceRgoConfig {
		production_type_idx_t production_type_idx = INVALID_IDX;
		fixed_point_t size_multiplier {};
	};

	// Stage-1 output. Narrow component so Stage 1's only write is to this struct.
	struct ProvinceRgoCacheTotals {
		pop_sum_t total_worker_count_in_province = 0;
		pop_sum_t total_owner_count_in_state = 0;
	};

	// Stage-2 output — the hire decision list and the totals derived from it. employees /
	// employee_count_per_type are .clear()'d (size→0) every tick but the capacity is preserved
	// to the worst-case bound reserved at construction.
	struct ProvinceRgoHired {
		std::vector<Employee> employees;
		// Parallel to employees; populated each tick by RgoHireSystem and read by
		// RgoProduceAndPlaceOrderSystem and RgoComputeEmployeeIncomeSystem.
		std::vector<pop_size_t> employee_count_per_type; // indexed by pop_type_idx
		pop_size_t max_employee_count = 0;
		pop_size_t total_employees = 0;
		pop_size_t total_paid_employees = 0;
	};

	// Persistent per-province sell order. Filled by Stage 3, drained by Stage 4 (which clears
	// `has_request`). Pre-attached to every province at construction → no per-tick entity
	// creation, no archetype migration.
	struct ProvinceRgoSellOrder {
		good_idx_t good_idx = INVALID_IDX;
		fixed_point_t quantity_requested {};
		bool has_request = false;
	};

	// Stage-3/4 output bundle. owner_share + total_minimum_wage are cached at Stage 4 so the
	// two Stage-5 sibling systems read them without sequencing on each other (the legacy code
	// computed both inline inside pay_employees).
	//
	// Stage 4 sets owner_share to 0 when there are no owner pops OR when revenue is too low
	// (revenue <= total_minimum_wage) — the latter forces Stage 5b into the proportional-
	// distribution branch and Stage 5a into the no-op branch.
	struct ProvinceRgoResult {
		fixed_point_t output_quantity_yesterday {};
		fixed_point_t revenue_yesterday {};
		fixed_point_t unsold_quantity_yesterday {}; // written-but-unused legacy placeholder
		fixed_point_t owner_share {};
		fixed_point_t total_minimum_wage {};
	};

	struct ProvinceRgoOwnerIncome {
		fixed_point_t total_owner_income {};
	};

	struct ProvinceRgoEmployeeIncome {
		std::vector<fixed_point_t> incomes; // parallel-indexed against ProvinceRgoHired::employees
		fixed_point_t total_employee_income {};
	};

	// Static-for-fixture-lifetime topological info. Pre-attached at construction; never
	// mutated by the tick path.
	struct ProvinceLocation {
		EntityID state_id {};
		EntityID owner_country_id {}; // invalid → no owner (RGO early-outs)
		uint32_t country_to_report_economy_idx = INVALID_IDX;
	};

	struct ProvincePopList {
		std::vector<EntityID> pop_ids;
	};

	// Province-scope modifier sums (would come from the modifier cache + country/local
	// modifier merge in the legacy code). Pre-baked once at fixture setup.
	struct ProvinceRgoModifiers {
		fixed_point_t rgo_output_tech {};
		fixed_point_t rgo_output_country {};
		fixed_point_t rgo_throughput_tech {};
		fixed_point_t rgo_throughput_country {};
		fixed_point_t local_rgo_output {};
		fixed_point_t local_rgo_throughput {};
	};

	struct ProvinceRgoFarmMineModifiers {
		fixed_point_t farm_throughput_and_output {};
		fixed_point_t farm_output_global {};
		fixed_point_t farm_output_local {};
		fixed_point_t farm_size_global {};
		fixed_point_t farm_size_local {};
		fixed_point_t mine_throughput_and_output {};
		fixed_point_t mine_output_global {};
		fixed_point_t mine_output_local {};
		fixed_point_t mine_size_global {};
		fixed_point_t mine_size_local {};
	};

	struct ProvinceRgoGoodModifiers {
		fixed_point_t rgo_goods_throughput {};
		fixed_point_t rgo_goods_output {};
		fixed_point_t rgo_size {};
	};

	// ============================================================================
	// Pop components
	// ============================================================================

	struct PopCore {
		pop_type_idx_t pop_type_idx = INVALID_IDX;
		pop_size_t size = 0;
	};

	struct PopLocation {
		EntityID province_id {};
		EntityID state_id {};
	};

	// Each pop type writes income through exactly one of {worker, owner} — sibling components
	// so Stage 6's two systems can write them truly in parallel.
	struct PopWorkerIncome {
		fixed_point_t rgo_worker_income_today {};
	};

	struct PopOwnerIncome {
		fixed_point_t rgo_owner_income_today {};
	};

	struct PopIncomeTotals {
		fixed_point_t total_income_today {};
		fixed_point_t cash {};
	};

	// ============================================================================
	// State components
	// ============================================================================

	struct StateProvinceList {
		std::vector<EntityID> province_ids;
	};

	// Owner-pop lookup, indexed by pop_type_idx → list of pop EntityIDs of that type in this
	// state. total_population is the state-wide pop-size sum (used to scale the legacy
	// owner-job contribution to multipliers in produce()).
	struct StateOwnerPopList {
		std::vector<std::vector<EntityID>> pops_by_type; // indexed by pop_type_idx
		pop_sum_t total_population = 0;
	};
}

ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoConfig, "OpenVic::ecs_rgo::ProvinceRgoConfig")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoCacheTotals, "OpenVic::ecs_rgo::ProvinceRgoCacheTotals")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoHired, "OpenVic::ecs_rgo::ProvinceRgoHired")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoSellOrder, "OpenVic::ecs_rgo::ProvinceRgoSellOrder")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoResult, "OpenVic::ecs_rgo::ProvinceRgoResult")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoOwnerIncome, "OpenVic::ecs_rgo::ProvinceRgoOwnerIncome")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoEmployeeIncome, "OpenVic::ecs_rgo::ProvinceRgoEmployeeIncome")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceLocation, "OpenVic::ecs_rgo::ProvinceLocation")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvincePopList, "OpenVic::ecs_rgo::ProvincePopList")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoModifiers, "OpenVic::ecs_rgo::ProvinceRgoModifiers")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoFarmMineModifiers, "OpenVic::ecs_rgo::ProvinceRgoFarmMineModifiers")
ECS_COMPONENT(OpenVic::ecs_rgo::ProvinceRgoGoodModifiers, "OpenVic::ecs_rgo::ProvinceRgoGoodModifiers")
ECS_COMPONENT(OpenVic::ecs_rgo::PopCore, "OpenVic::ecs_rgo::PopCore")
ECS_COMPONENT(OpenVic::ecs_rgo::PopLocation, "OpenVic::ecs_rgo::PopLocation")
ECS_COMPONENT(OpenVic::ecs_rgo::PopWorkerIncome, "OpenVic::ecs_rgo::PopWorkerIncome")
ECS_COMPONENT(OpenVic::ecs_rgo::PopOwnerIncome, "OpenVic::ecs_rgo::PopOwnerIncome")
ECS_COMPONENT(OpenVic::ecs_rgo::PopIncomeTotals, "OpenVic::ecs_rgo::PopIncomeTotals")
ECS_COMPONENT(OpenVic::ecs_rgo::StateProvinceList, "OpenVic::ecs_rgo::StateProvinceList")
ECS_COMPONENT(OpenVic::ecs_rgo::StateOwnerPopList, "OpenVic::ecs_rgo::StateOwnerPopList")
