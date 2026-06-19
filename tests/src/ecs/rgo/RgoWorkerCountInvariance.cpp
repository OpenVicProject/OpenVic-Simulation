#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include "RgoFixture.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ecs;
using namespace OpenVic::ecs_rgo;
using namespace OpenVic::ecs_rgo::test_support;

// The multiplayer-determinism contract gate for the RGO pipeline. Identical starting World +
// identical inputs must produce a bit-identical post-tick digest for every worker_count in
// {1, 2, 4, 8, 16} across a 10-tick fixture. Stages 5 and 6 each share a stage with two
// SystemThreaded so this gate also exercises the multi-system-stage parallel path.

namespace {
	// 50 provinces, 5 states (so ~10 provinces per state), ~500 pops total. Mixes worker pops
	// (pop_type 0) and owner pops (pop_type 1) across provinces. Deterministic generators.
	struct LargeFixtureParams {
		uint32_t worker_count;
	};

	std::vector<ProductionTypeDef> make_large_registry() {
		// Three RGO production types — different base_output_quantity to break symmetry.
		std::vector<ProductionTypeDef> out;
		for (int k = 0; k < 3; ++k) {
			ProductionTypeDef pt;
			pt.template_type = TemplateType::Rgo;
			Job wjob;
			wjob.pop_type_idx = 0;
			wjob.effect_type = JobEffectType::Throughput;
			wjob.effect_multiplier = fixed_point_t::_1;
			wjob.amount = fixed_point_t::_1;
			pt.jobs.push_back(wjob);

			Job ojob;
			ojob.pop_type_idx = 1;
			ojob.effect_type = JobEffectType::Throughput;
			ojob.effect_multiplier = fixed_point_t::_0;
			ojob.amount = fixed_point_t::_1;
			pt.owner = ojob;

			pt.output_good_idx = static_cast<good_idx_t>(k);
			pt.base_workforce_size = 40;
			pt.base_output_quantity = fixed_point_t { k + 1 }; // 1, 2, 3
			pt.is_farm = (k == 0);
			pt.is_mine = (k == 1);
			out.push_back(pt);
		}
		return out;
	}

	int64_t run_and_digest(LargeFixtureParams params) {
		std::size_t const state_count = 5;
		std::size_t const province_count = 50;

		std::vector<TestState> states(state_count);
		std::vector<TestProvince> provinces(province_count);

		// Round-robin assign provinces to states + deterministic content per province.
		for (std::size_t p = 0; p < province_count; ++p) {
			std::size_t const s = p % state_count;
			states[s].province_indices.push_back(static_cast<uint32_t>(p));

			provinces[p].state_idx = static_cast<uint32_t>(s);
			provinces[p].production_type_idx = static_cast<production_type_idx_t>(p % 3);

			// 8 worker pops + 2 owner pops per province on average.
			for (std::size_t i = 0; i < 10; ++i) {
				TestPop tp;
				tp.pop_type_idx = (i < 8) ? 0u : 1u; // 0..7 workers, 8..9 owners
				tp.size = static_cast<pop_size_t>(10 + (p * 3 + i * 7) % 91); // 10..100
				provinces[p].pops.push_back(tp);
			}

			// Small modifier values to break symmetry.
			provinces[p].mods.rgo_output_country =
				fp::from_fraction<int32_t>(static_cast<int32_t>(p % 10), 100);
			provinces[p].gm.rgo_size = fp::from_fraction<int32_t>(static_cast<int32_t>(s), 100);
		}

		std::vector<fixed_point_t> prices = {
			fixed_point_t::_1,
			fixed_point_t::_1 + fixed_point_t::_0_25,
			fixed_point_t::_2
		};

		auto bw = build_world(
			make_large_registry(), std::move(prices),
			states, provinces, /*pop_type_count*/ 2,
			/*use_simple_farm_mine_logic*/ false,
			params.worker_count
		);

		for (int t = 0; t < 10; ++t) {
			bw->world.tick_systems(Date {});
		}

		// Digest the relevant post-tick state. Order is the deterministic creation order so
		// the digest itself is stable across worker counts (the systems' output is what we're
		// testing for invariance).
		int64_t digest = 0;
		for (EntityID const& prov : bw->province_entities) {
			ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
			if (res != nullptr) {
				digest = digest * 1000003 + res->revenue_yesterday.get_raw_value();
				digest = digest * 1000003 + res->output_quantity_yesterday.get_raw_value();
				digest = digest * 1000003 + res->owner_share.get_raw_value();
			}
		}
		for (EntityID const& pop : bw->pop_entities) {
			PopIncomeTotals const* pit = bw->world.get_component<PopIncomeTotals>(pop);
			if (pit != nullptr) {
				digest = digest * 1000003 + pit->cash.get_raw_value();
			}
		}
		return digest;
	}
}

TEST_CASE("RgoWorkerCountInvariance: 10-tick digest is identical across worker counts",
          "[ecs_rgo][RgoWorkerCountInvariance][determinism]") {
	int64_t const baseline = run_and_digest({ 1 });
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		int64_t const result = run_and_digest({ wc });
		CHECK(result == baseline);
	}
}
