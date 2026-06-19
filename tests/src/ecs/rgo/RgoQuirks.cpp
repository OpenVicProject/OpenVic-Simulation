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

// ============================================================================
// Vic2-quirk regression tests. Three quirks the legacy code observes that any "clean rewrite"
// might inadvertently break:
//
//   1. production_type == INVALID_IDX: every system early-outs; no allocations, no writes.
//   2. Slaves-only province: pinning algorithm runs but distributes nothing (legacy comment:
//      "scenario slaves only; money is removed from system in Victoria 2").
//   3. Production type without owner: Stage 4 sets owner_share = 0, Stage 5a writes
//      total_owner_income = 0, Stage 6b applies nothing.
// ============================================================================

namespace {
	std::vector<ProductionTypeDef> make_basic_registry() {
		std::vector<ProductionTypeDef> out;
		ProductionTypeDef pt;
		pt.template_type = TemplateType::Rgo;
		Job wjob;
		wjob.pop_type_idx = 0;
		wjob.effect_type = JobEffectType::Throughput;
		wjob.effect_multiplier = fixed_point_t::_1;
		wjob.amount = fixed_point_t::_1;
		pt.jobs.push_back(wjob);
		pt.output_good_idx = 0;
		pt.base_workforce_size = 50;
		pt.base_output_quantity = fixed_point_t::_1;
		out.push_back(pt);
		return out;
	}
}

// ============================================================================
// Quirk 1 — INVALID production type
// ============================================================================

TEST_CASE("RgoQuirks: province with INVALID production type stays at zero",
          "[ecs_rgo][RgoQuirks]") {
	std::vector<TestState> states(1);
	states[0].province_indices = { 0 };

	std::vector<TestProvince> provinces(1);
	provinces[0].production_type_idx = INVALID_IDX;
	provinces[0].state_idx = 0;
	TestPop wp;
	wp.pop_type_idx = 0;
	wp.size = 50;
	provinces[0].pops.push_back(wp);

	auto bw = build_world(
		make_basic_registry(), { fixed_point_t::_1 },
		states, provinces, /*pop_type_count*/ 2,
		false, /*worker_count*/ 1
	);
	bw->world.tick_systems(Date {});

	EntityID const prov = bw->province_entities[0];
	ProvinceRgoCacheTotals const* totals = bw->world.get_component<ProvinceRgoCacheTotals>(prov);
	ProvinceRgoHired const* hired = bw->world.get_component<ProvinceRgoHired>(prov);
	ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
	REQUIRE(totals != nullptr);
	REQUIRE(hired != nullptr);
	REQUIRE(res != nullptr);
	CHECK(totals->total_worker_count_in_province == 0);
	CHECK(hired->total_employees == 0);
	CHECK(res->output_quantity_yesterday == fixed_point_t::_0);
	CHECK(res->revenue_yesterday == fixed_point_t::_0);

	EntityID const pop = bw->pop_entities[0];
	PopIncomeTotals const* pit = bw->world.get_component<PopIncomeTotals>(pop);
	REQUIRE(pit != nullptr);
	CHECK(pit->cash == fixed_point_t::_0);
}

// ============================================================================
// Quirk 2 — slaves-only province; legacy comment: "money is removed from system in Victoria 2"
// ============================================================================

TEST_CASE("RgoQuirks: slaves-only province silently loses revenue", "[ecs_rgo][RgoQuirks]") {
	// Production type whose worker job is_slave = true.
	std::vector<ProductionTypeDef> registry;
	{
		ProductionTypeDef pt;
		pt.template_type = TemplateType::Rgo;
		Job wjob;
		wjob.pop_type_idx = 0;
		wjob.effect_type = JobEffectType::Throughput;
		wjob.effect_multiplier = fixed_point_t::_1;
		wjob.amount = fixed_point_t::_1;
		wjob.is_slave = true;
		pt.jobs.push_back(wjob);
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
	wp.size = 50;
	provinces[0].pops.push_back(wp);

	auto bw = build_world(
		std::move(registry), { fixed_point_t::_1 },
		states, provinces, /*pop_type_count*/ 2,
		false, /*worker_count*/ 1
	);
	bw->world.tick_systems(Date {});

	EntityID const prov = bw->province_entities[0];
	ProvinceRgoHired const* hired = bw->world.get_component<ProvinceRgoHired>(prov);
	REQUIRE(hired != nullptr);
	CHECK(hired->total_employees == 50);
	CHECK(hired->total_paid_employees == 0); // all slaves

	ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
	REQUIRE(res != nullptr);
	CHECK(res->revenue_yesterday > fixed_point_t::_0); // sale happened

	ProvinceRgoEmployeeIncome const* einc =
		bw->world.get_component<ProvinceRgoEmployeeIncome>(prov);
	REQUIRE(einc != nullptr);
	CHECK(einc->total_employee_income == fixed_point_t::_0); // but no income distributed

	// The slave pop gets nothing — total_income_today is zero.
	EntityID const pop = bw->pop_entities[0];
	PopIncomeTotals const* pit = bw->world.get_component<PopIncomeTotals>(pop);
	REQUIRE(pit != nullptr);
	CHECK(pit->total_income_today == fixed_point_t::_0);
	CHECK(pit->cash == fixed_point_t::_0);
}

// ============================================================================
// Quirk 3 — production type without owner job. Stage 4 sets owner_share = 0; Stage 5a / 6b
// write zero.
// ============================================================================

TEST_CASE("RgoQuirks: production type without owner pop skips owner-share path",
          "[ecs_rgo][RgoQuirks]") {
	// make_basic_registry is already owner-less.
	std::vector<TestState> states(1);
	states[0].province_indices = { 0 };
	std::vector<TestProvince> provinces(1);
	provinces[0].production_type_idx = 0;
	provinces[0].state_idx = 0;
	TestPop wp;
	wp.pop_type_idx = 0;
	wp.size = 50;
	provinces[0].pops.push_back(wp);

	auto bw = build_world(
		make_basic_registry(), { fixed_point_t::_1 },
		states, provinces, /*pop_type_count*/ 2,
		false, /*worker_count*/ 1
	);
	bw->world.tick_systems(Date {});

	EntityID const prov = bw->province_entities[0];
	ProvinceRgoResult const* res = bw->world.get_component<ProvinceRgoResult>(prov);
	REQUIRE(res != nullptr);
	CHECK(res->owner_share == fixed_point_t::_0);

	ProvinceRgoOwnerIncome const* oi = bw->world.get_component<ProvinceRgoOwnerIncome>(prov);
	REQUIRE(oi != nullptr);
	CHECK(oi->total_owner_income == fixed_point_t::_0);

	// Pop has worker income but no owner income.
	EntityID const pop = bw->pop_entities[0];
	PopOwnerIncome const* poi = bw->world.get_component<PopOwnerIncome>(pop);
	REQUIRE(poi != nullptr);
	CHECK(poi->rgo_owner_income_today == fixed_point_t::_0);

	PopWorkerIncome const* pwi = bw->world.get_component<PopWorkerIncome>(pop);
	REQUIRE(pwi != nullptr);
	CHECK(pwi->rgo_worker_income_today > fixed_point_t::_0);
}
