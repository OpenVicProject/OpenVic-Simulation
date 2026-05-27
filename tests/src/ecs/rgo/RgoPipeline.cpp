#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include "RgoFixture.hpp"

#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ecs;
using namespace OpenVic::ecs_rgo;
using namespace OpenVic::ecs_rgo::test_support;

namespace {
	// Minimal production-type registry: pop type 0 = worker, pop type 1 = owner.
	// One RGO production type. base_workforce_size=50, base_output_quantity=1.
	std::vector<ProductionTypeDef> make_minimal_registry() {
		std::vector<ProductionTypeDef> out;
		ProductionTypeDef pt;
		pt.template_type = TemplateType::Rgo;
		Job worker_job;
		worker_job.pop_type_idx = 0;
		worker_job.effect_type = JobEffectType::Throughput;
		worker_job.effect_multiplier = fixed_point_t::_1;
		worker_job.amount = fixed_point_t::_1;
		worker_job.is_slave = false;
		pt.jobs.push_back(worker_job);
		pt.output_good_idx = 0;
		pt.base_workforce_size = 50;
		pt.base_output_quantity = fixed_point_t::_1;
		pt.is_farm = false;
		pt.is_mine = false;
		out.push_back(pt);
		return out;
	}

	std::vector<fixed_point_t> make_minimal_prices() {
		// Single good, unit_price = 1.0 — so revenue == output.
		return { fixed_point_t::_1 };
	}
}

// ============================================================================
// Single-province, single-state, single-worker-pop, no owner.
// Hand-computed expected values:
//   total_workers_in_province = 50
//   size_modifier             = 1
//   size_multiplier           = floor(ceil(50/50/1) * 1.5) = floor(1.5) = 1
//   max_employee_count        = floor(1 * 1 * 50) = 50
//   proportion_to_hire        = 1 (max >= available)
//   employee_count_per_type[0] = 50
//   throughput_from_workers   = 1 (frac = 1, mul=1 → mul*frac=1)
//   output_from_workers       = 1
//   throughput_multiplier     = 1 (no owner, no modifiers)
//   output_multiplier         = 1
//   output                    = 1 * 1 * 1 * 1 * 1 * 1 * 1 = 1.0
//   revenue                   = 1.0 * unit_price (1.0) = 1.0
//   owner_share               = 0 (no owners)
//   minimum_wage per emp      = 0.1 * 50/1000 ≈ 0.005 (parse_raw'd, < revenue)
//   pinning result            = 1.0 for the single employee
//   total_employee_income     = 1.0
//   PopWorkerIncome           = 1.0
//   PopIncomeTotals.cash      = ticks * 1.0
// ============================================================================

TEST_CASE("RgoPipeline: minimal no-owner fixture produces unit-revenue per tick",
          "[ecs_rgo][RgoPipeline]") {
	std::vector<TestState> states(1);
	states[0].province_indices = { 0 };

	std::vector<TestProvince> provinces(1);
	provinces[0].production_type_idx = 0;
	provinces[0].state_idx = 0;
	TestPop worker;
	worker.pop_type_idx = 0;
	worker.size = 50;
	provinces[0].pops.push_back(worker);

	auto bw = build_world(
		make_minimal_registry(), make_minimal_prices(),
		states, provinces, /*pop_type_count*/ 2,
		/*use_simple_farm_mine_logic*/ false,
		/*worker_count*/ 1
	);

	// Single tick.
	bw->world.tick_systems(Date {});

	// Province asserts.
	EntityID const prov = bw->province_entities[0];
	ProvinceRgoCacheTotals const* totals = bw->world.get_component<ProvinceRgoCacheTotals>(prov);
	REQUIRE(totals != nullptr);
	CHECK(totals->total_worker_count_in_province == 50);
	CHECK(totals->total_owner_count_in_state == 0);

	ProvinceRgoHired const* hired = bw->world.get_component<ProvinceRgoHired>(prov);
	REQUIRE(hired != nullptr);
	CHECK(hired->max_employee_count == 50);
	CHECK(hired->total_employees == 50);
	CHECK(hired->total_paid_employees == 50);
	REQUIRE(hired->employees.size() == 1);
	CHECK(hired->employees[0].hired_size == 50);

	ProvinceRgoResult const* result = bw->world.get_component<ProvinceRgoResult>(prov);
	REQUIRE(result != nullptr);
	CHECK(result->output_quantity_yesterday == fixed_point_t::_1);
	CHECK(result->revenue_yesterday == fixed_point_t::_1);
	CHECK(result->owner_share == fixed_point_t::_0);

	ProvinceRgoEmployeeIncome const* einc =
		bw->world.get_component<ProvinceRgoEmployeeIncome>(prov);
	REQUIRE(einc != nullptr);
	REQUIRE(einc->incomes.size() == 1);
	CHECK(einc->incomes[0] == fixed_point_t::_1);
	CHECK(einc->total_employee_income == fixed_point_t::_1);

	// Pop asserts.
	EntityID const pop = bw->pop_entities[0];
	PopWorkerIncome const* pwi = bw->world.get_component<PopWorkerIncome>(pop);
	PopOwnerIncome const* poi = bw->world.get_component<PopOwnerIncome>(pop);
	PopIncomeTotals const* pit = bw->world.get_component<PopIncomeTotals>(pop);
	REQUIRE(pwi != nullptr);
	REQUIRE(poi != nullptr);
	REQUIRE(pit != nullptr);
	CHECK(pwi->rgo_worker_income_today == fixed_point_t::_1);
	CHECK(poi->rgo_owner_income_today == fixed_point_t::_0);
	CHECK(pit->total_income_today == fixed_point_t::_1);
	CHECK(pit->cash == fixed_point_t::_1);

	// Run 9 more ticks — cash should accumulate linearly.
	for (int i = 0; i < 9; ++i) {
		bw->world.tick_systems(Date {});
	}
	PopIncomeTotals const* pit_after = bw->world.get_component<PopIncomeTotals>(pop);
	REQUIRE(pit_after != nullptr);
	CHECK(pit_after->cash == fixed_point_t { 10 });
	CHECK(pit_after->total_income_today == fixed_point_t::_1);
}

// ============================================================================
// Three-province, two-state fixture with mixed RGO + idle provinces.
// Verifies that inactive provinces (production_type_idx == INVALID_IDX) stay at zero,
// state-level owner totals are computed correctly across provinces in the same state,
// and pop incomes are accumulated only for pops in active RGO provinces.
// ============================================================================

TEST_CASE("RgoPipeline: 3-province 2-state mixed fixture", "[ecs_rgo][RgoPipeline]") {
	// pop_type_count = 2: 0 = worker, 1 = idle-non-rgo (no jobs use this).
	std::vector<TestState> states(2);
	states[0].province_indices = { 0, 1 };
	states[1].province_indices = { 2 };

	std::vector<TestProvince> provinces(3);
	// Province 0 — active RGO with 100 workers.
	provinces[0].production_type_idx = 0;
	provinces[0].state_idx = 0;
	TestPop worker_pop;
	worker_pop.pop_type_idx = 0;
	worker_pop.size = 100;
	provinces[0].pops.push_back(worker_pop);

	// Province 1 — INACTIVE RGO (no production type) but still has pops. Must early-out.
	provinces[1].production_type_idx = INVALID_IDX;
	provinces[1].state_idx = 0;
	TestPop idle_pop;
	idle_pop.pop_type_idx = 0;
	idle_pop.size = 25;
	provinces[1].pops.push_back(idle_pop);

	// Province 2 — active RGO with 50 workers, in state 1.
	provinces[2].production_type_idx = 0;
	provinces[2].state_idx = 1;
	TestPop worker2;
	worker2.pop_type_idx = 0;
	worker2.size = 50;
	provinces[2].pops.push_back(worker2);

	auto bw = build_world(
		make_minimal_registry(), make_minimal_prices(),
		states, provinces, /*pop_type_count*/ 2,
		/*use_simple_farm_mine_logic*/ false,
		/*worker_count*/ 4
	);

	bw->world.tick_systems(Date {});

	// Province 0 — active.
	{
		EntityID const prov = bw->province_entities[0];
		ProvinceRgoCacheTotals const* totals =
			bw->world.get_component<ProvinceRgoCacheTotals>(prov);
		CHECK(totals->total_worker_count_in_province == 100);
		ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
		CHECK(res->output_quantity_yesterday > fixed_point_t::_0);
		CHECK(res->revenue_yesterday > fixed_point_t::_0);
	}
	// Province 1 — inactive. Stage-3 zeroes output_quantity_yesterday + revenue_yesterday.
	{
		EntityID const prov = bw->province_entities[1];
		ProvinceRgoCacheTotals const* totals =
			bw->world.get_component<ProvinceRgoCacheTotals>(prov);
		CHECK(totals->total_worker_count_in_province == 0);
		ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
		CHECK(res->output_quantity_yesterday == fixed_point_t::_0);
		CHECK(res->revenue_yesterday == fixed_point_t::_0);
	}
	// Province 2 — active in state 1.
	{
		EntityID const prov = bw->province_entities[2];
		ProvinceRgoCacheTotals const* totals =
			bw->world.get_component<ProvinceRgoCacheTotals>(prov);
		CHECK(totals->total_worker_count_in_province == 50);
		ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
		CHECK(res->revenue_yesterday > fixed_point_t::_0);
	}

	// Pop in province 1 (idle RGO) should have zero income.
	{
		EntityID const pop = bw->pop_entities[1]; // province 1's only pop
		PopIncomeTotals const* pit = bw->world.get_component<PopIncomeTotals>(pop);
		REQUIRE(pit != nullptr);
		CHECK(pit->total_income_today == fixed_point_t::_0);
		CHECK(pit->cash == fixed_point_t::_0);
	}

	// Pops in active provinces should have non-zero income.
	{
		EntityID const pop0 = bw->pop_entities[0]; // province 0's only pop
		PopIncomeTotals const* pit0 = bw->world.get_component<PopIncomeTotals>(pop0);
		REQUIRE(pit0 != nullptr);
		CHECK(pit0->total_income_today > fixed_point_t::_0);

		EntityID const pop2 = bw->pop_entities[2]; // province 2's only pop
		PopIncomeTotals const* pit2 = bw->world.get_component<PopIncomeTotals>(pop2);
		REQUIRE(pit2 != nullptr);
		CHECK(pit2->total_income_today > fixed_point_t::_0);
	}
}

// ============================================================================
// Owner-share fixture: 1 province with workers + 1 owner pop in the same state.
// Verifies that owner_share is non-zero, owner_income flows to the owner pop, and worker
// income is reduced by the owner's slice (revenue_left = revenue * (1 - owner_share)).
// ============================================================================

TEST_CASE("RgoPipeline: owner-share fixture splits revenue between worker and owner",
          "[ecs_rgo][RgoPipeline]") {
	// Production type: worker job (pop type 0) + owner job (pop type 1, Throughput).
	std::vector<ProductionTypeDef> registry;
	{
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
		ojob.effect_multiplier = fixed_point_t::_0; // no production effect from owners
		ojob.amount = fixed_point_t::_1;
		pt.owner = ojob;

		pt.output_good_idx = 0;
		pt.base_workforce_size = 50;
		pt.base_output_quantity = fixed_point_t::_1;
		registry.push_back(pt);
	}

	std::vector<TestState> states(1);
	states[0].province_indices = { 0 };

	std::vector<TestProvince> provinces(1);
	provinces[0].production_type_idx = 0;
	provinces[0].state_idx = 0;
	TestPop wp;
	wp.pop_type_idx = 0;
	wp.size = 100;
	provinces[0].pops.push_back(wp);
	TestPop op;
	op.pop_type_idx = 1;
	op.size = 25;
	provinces[0].pops.push_back(op);

	auto bw = build_world(
		std::move(registry), make_minimal_prices(),
		states, provinces, /*pop_type_count*/ 2,
		/*use_simple_farm_mine_logic*/ false,
		/*worker_count*/ 1
	);

	bw->world.tick_systems(Date {});

	EntityID const prov = bw->province_entities[0];
	ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
	REQUIRE(res != nullptr);
	CHECK(res->owner_share > fixed_point_t::_0);
	CHECK(res->revenue_yesterday > fixed_point_t::_0);

	EntityID const owner_pop = bw->pop_entities[1];
	PopOwnerIncome const* poi = bw->world.get_component<PopOwnerIncome>(owner_pop);
	REQUIRE(poi != nullptr);
	CHECK(poi->rgo_owner_income_today > fixed_point_t::_0);

	// Worker pop should ALSO have income (just smaller than full revenue).
	EntityID const worker_pop = bw->pop_entities[0];
	PopWorkerIncome const* pwi = bw->world.get_component<PopWorkerIncome>(worker_pop);
	REQUIRE(pwi != nullptr);
	CHECK(pwi->rgo_worker_income_today > fixed_point_t::_0);

	// Sanity: worker income + owner income ≈ revenue (modulo epsilon flooring).
	fixed_point_t const total = pwi->rgo_worker_income_today + poi->rgo_owner_income_today;
	CHECK(total <= res->revenue_yesterday + fixed_point_t::epsilon * fixed_point_t { 4 });
	CHECK(total >= res->revenue_yesterday - fixed_point_t::epsilon * fixed_point_t { 4 });
}
