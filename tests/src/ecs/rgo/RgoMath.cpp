#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoMath.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/Math.hpp"

#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ecs_rgo;
using namespace OpenVic::ecs_rgo::detail;

// ============================================================================
// resolve_farm_mine_classification — the 4-bucket farm/mine table × 2 game rules.
// ============================================================================

namespace {
	ProductionTypeDef make_pt(bool farm, bool mine) {
		ProductionTypeDef pt;
		pt.is_farm = farm;
		pt.is_mine = mine;
		return pt;
	}
}

TEST_CASE("RgoMath: farm/mine classification with vanilla rules", "[ecs_rgo][RgoMath]") {
	RgoGameRules vanilla {};
	vanilla.use_simple_farm_mine_logic = false;

	// farm-only: is_farm_for_tech yes (not mine), is_farm_for_non_tech yes, mines no.
	{
		FarmMineClassification const c = resolve_farm_mine_classification(make_pt(true, false), vanilla);
		CHECK(c.is_farm_for_tech);
		CHECK(c.is_farm_for_non_tech);
		CHECK_FALSE(c.is_mine_for_tech);
		CHECK_FALSE(c.is_mine_for_non_tech);
	}
	// mine-only: is_mine_for_tech yes, is_mine_for_non_tech yes (not farm), farms no.
	{
		FarmMineClassification const c = resolve_farm_mine_classification(make_pt(false, true), vanilla);
		CHECK_FALSE(c.is_farm_for_tech);
		CHECK_FALSE(c.is_farm_for_non_tech);
		CHECK(c.is_mine_for_tech);
		CHECK(c.is_mine_for_non_tech);
	}
	// both: farm_for_tech NO (also mine), mine_for_non_tech NO (also farm); non_tech farm yes,
	// tech mine yes — the Vic2 asymmetry.
	{
		FarmMineClassification const c = resolve_farm_mine_classification(make_pt(true, true), vanilla);
		CHECK_FALSE(c.is_farm_for_tech);
		CHECK(c.is_farm_for_non_tech);
		CHECK(c.is_mine_for_tech);
		CHECK_FALSE(c.is_mine_for_non_tech);
	}
	// neither:
	{
		FarmMineClassification const c = resolve_farm_mine_classification(make_pt(false, false), vanilla);
		CHECK_FALSE(c.is_farm_for_tech);
		CHECK_FALSE(c.is_farm_for_non_tech);
		CHECK_FALSE(c.is_mine_for_tech);
		CHECK_FALSE(c.is_mine_for_non_tech);
	}
}

TEST_CASE("RgoMath: farm/mine classification with simple rules", "[ecs_rgo][RgoMath]") {
	RgoGameRules simple {};
	simple.use_simple_farm_mine_logic = true;

	// Under simple rules, the flags pass through verbatim — both farm and mine entries are
	// "for_tech" AND "for_non_tech".
	FarmMineClassification const c = resolve_farm_mine_classification(make_pt(true, true), simple);
	CHECK(c.is_farm_for_tech);
	CHECK(c.is_farm_for_non_tech);
	CHECK(c.is_mine_for_tech);
	CHECK(c.is_mine_for_non_tech);
}

// ============================================================================
// calculate_size_modifier — sums modifiers, clamps at 0.
// ============================================================================

TEST_CASE("RgoMath: size_modifier basic accumulation", "[ecs_rgo][RgoMath]") {
	RgoGameRules vanilla {};
	ProvinceRgoFarmMineModifiers fm {};
	fm.farm_size_global = fixed_point_t::_0_50;
	fm.farm_size_local = fixed_point_t::_0_10;
	ProvinceRgoGoodModifiers gm {};
	gm.rgo_size = fixed_point_t::_0_20;

	// farm-only: starts at 1, + farm_size_global (tech) + farm_size_local (non-tech) +
	// rgo_size = 1 + 0.5 + 0.1 + 0.2 = 1.8.
	fixed_point_t const r = calculate_size_modifier(make_pt(true, false), fm, gm, vanilla);
	CHECK(r == fixed_point_t::_1 + fixed_point_t::_0_50 + fixed_point_t::_0_10 + fixed_point_t::_0_20);
}

TEST_CASE("RgoMath: size_modifier clamps at zero", "[ecs_rgo][RgoMath]") {
	RgoGameRules vanilla {};
	ProvinceRgoFarmMineModifiers fm {};
	fm.farm_size_global = fixed_point_t { -10 }; // would produce a -9 result without clamp
	ProvinceRgoGoodModifiers gm {};

	fixed_point_t const r = calculate_size_modifier(make_pt(true, false), fm, gm, vanilla);
	CHECK(r == fixed_point_t::_0);
}

// ============================================================================
// calculate_size_multiplier_from_workforce — the Vic2 floor(ceil(...) * 1.5) tiering.
// ============================================================================

TEST_CASE("RgoMath: size_multiplier formula", "[ecs_rgo][RgoMath]") {
	// workers=100, base=20, mod=1 → ceil(100/20/1) = 5 → 5 * 1.5 = 7.5 → floor = 7.
	fixed_point_t const r = calculate_size_multiplier_from_workforce(100, 20, fixed_point_t::_1);
	CHECK(r == fixed_point_t { 7 });
}

TEST_CASE("RgoMath: size_multiplier returns zero on zero modifier", "[ecs_rgo][RgoMath]") {
	fixed_point_t const r = calculate_size_multiplier_from_workforce(100, 20, fixed_point_t::_0);
	CHECK(r == fixed_point_t::_0);
}

// ============================================================================
// calculate_max_employee_count — floor of the size_modifier * size_multiplier * base product.
// ============================================================================

TEST_CASE("RgoMath: max_employee_count from sizes and base", "[ecs_rgo][RgoMath]") {
	// 1.5 * 4 * 20 = 120 → floor = 120.
	pop_size_t const r = calculate_max_employee_count(
		fixed_point_t::_1_50, fixed_point_t { 4 }, 20
	);
	CHECK(r == 120);
}

// ============================================================================
// compute_job_effect — the Vic2 capped-share branch.
// ============================================================================

TEST_CASE("RgoMath: job effect — fraction below amount uses multiplier * fraction",
          "[ecs_rgo][RgoMath]") {
	// employees=10, max=100 → fraction=0.1; amount=0.5 → fraction < amount; multiplier!=1
	// → multiplier * fraction = 2 * 0.1 = 0.2.
	fixed_point_t const r = compute_job_effect(
		fixed_point_t { 2 }, fixed_point_t::_0_50, 10, 100
	);
	CHECK(r == fixed_point_t::_0_20);
}

TEST_CASE("RgoMath: job effect — fraction above amount caps at multiplier * amount",
          "[ecs_rgo][RgoMath]") {
	// employees=80, max=100 → fraction=0.8; amount=0.5 → fraction > amount; multiplier!=1
	// → multiplier * amount = 2 * 0.5 = 1.0 (the capped path).
	fixed_point_t const r = compute_job_effect(
		fixed_point_t { 2 }, fixed_point_t::_0_50, 80, 100
	);
	CHECK(r == fixed_point_t::_1);
}

TEST_CASE("RgoMath: job effect — multiplier == 1 bypasses capping", "[ecs_rgo][RgoMath]") {
	// multiplier == 1 → use fraction even if it exceeds amount: 80/100 = 0.8.
	fixed_point_t const r = compute_job_effect(
		fixed_point_t::_1, fixed_point_t::_0_50, 80, 100
	);
	CHECK(r == fp::from_fraction<pop_size_t>(80, 100));
}

// ============================================================================
// compute_owner_share — both bounds of the min(...) clamp.
// ============================================================================

TEST_CASE("RgoMath: owner_share clamps to 0.5 when desired exceeds upper", "[ecs_rgo][RgoMath]") {
	// 2 * 100 / 50 = 4 → desired 4. upper_limit = min(0.5, 1 - 0/100) = 0.5. result = 0.5.
	fixed_point_t const r = compute_owner_share(100, 50, fixed_point_t { 100 }, fixed_point_t::_0);
	CHECK(r == fixed_point_t::_0_50);
}

TEST_CASE("RgoMath: owner_share clamps by min-wage upper limit", "[ecs_rgo][RgoMath]") {
	// revenue=100, total_min_wage=60 → upper = min(0.5, 1 - 0.6) = 0.4.
	// desired = 2 * 100 / 50 = 4 → clamped to 0.4.
	fixed_point_t const r = compute_owner_share(100, 50, fixed_point_t { 100 }, fixed_point_t { 60 });
	CHECK(r == fixed_point_t::_1 - fp::from_fraction<int32_t>(60, 100));
}

TEST_CASE("RgoMath: owner_share returns desired when it's lowest", "[ecs_rgo][RgoMath]") {
	// 2 * 10 / 100 = 0.2 → less than 0.5 upper limit.
	fixed_point_t const r = compute_owner_share(10, 100, fixed_point_t { 100 }, fixed_point_t::_0);
	CHECK(r == fp::from_fraction<int32_t>(20, 100));
}

// ============================================================================
// distribute_insufficient_revenue_proportional — proportional split.
// ============================================================================

TEST_CASE("RgoMath: insufficient revenue proportional distribution",
          "[ecs_rgo][RgoMath]") {
	std::vector<Employee> employees(2);
	employees[0].minimum_wage_cached = fixed_point_t { 30 };
	employees[1].minimum_wage_cached = fixed_point_t { 60 };

	fixed_point_t const revenue = fixed_point_t { 45 };
	fixed_point_t const total_min = fixed_point_t { 90 };

	std::vector<fixed_point_t> incomes(2);
	distribute_insufficient_revenue_proportional(employees, revenue, total_min, incomes);
	// e0 → 45 * 30/90 = 15. e1 → 45 * 60/90 = 30.
	CHECK(incomes[0] == fixed_point_t { 15 });
	CHECK(incomes[1] == fixed_point_t { 30 });
	// Both are above epsilon, so the max-with-epsilon floor is a no-op here.
	CHECK(incomes[0] + incomes[1] == revenue);
}

TEST_CASE("RgoMath: insufficient revenue distribution applies epsilon floor",
          "[ecs_rgo][RgoMath]") {
	std::vector<Employee> employees(1);
	employees[0].minimum_wage_cached = fixed_point_t::epsilon;

	std::vector<fixed_point_t> incomes(1);
	distribute_insufficient_revenue_proportional(
		employees, fixed_point_t::_0, fixed_point_t::epsilon, incomes
	);
	// 0 * eps/eps = 0; clamped up to epsilon.
	CHECK(incomes[0] == fixed_point_t::epsilon);
}

// ============================================================================
// distribute_employee_incomes_min_wage_pinning — multi-round pinning stress.
// ============================================================================

TEST_CASE("RgoMath: pinning algorithm — no pin needed when income meets min",
          "[ecs_rgo][RgoMath]") {
	std::vector<Employee> employees(2);
	employees[0].hired_size = 50;
	employees[0].minimum_wage_cached = fixed_point_t { 5 };
	employees[1].hired_size = 50;
	employees[1].minimum_wage_cached = fixed_point_t { 5 };

	std::vector<fixed_point_t> incomes(2);
	distribute_employee_incomes_min_wage_pinning(
		employees, fixed_point_t { 100 }, fixed_point_t { 10 }, 100, incomes
	);
	// 100 / 100 * 50 = 50 per employee. > min_wage 5. No pin.
	CHECK(incomes[0] == fixed_point_t { 50 });
	CHECK(incomes[1] == fixed_point_t { 50 });
}

TEST_CASE("RgoMath: pinning algorithm — one round pins a single small employee",
          "[ecs_rgo][RgoMath]") {
	std::vector<Employee> employees(2);
	employees[0].hired_size = 50;
	employees[0].minimum_wage_cached = fixed_point_t { 5 };
	employees[1].hired_size = 50;
	employees[1].minimum_wage_cached = fixed_point_t { 80 }; // unrealistic, but forces a pin

	std::vector<fixed_point_t> incomes(2);
	// Revenue 100, count 100. First pass: each gets 50. e1's min wage is 80 > 50 → pinned to 80.
	// After pin: revenue_left = 100 - 80 = 20; paid = 100 - 50 = 50. Second pass: e0 not pinned,
	// proposes 20 * 50/50 = 20. > min_wage 5. Settled.
	distribute_employee_incomes_min_wage_pinning(
		employees, fixed_point_t { 100 }, fixed_point_t { 85 }, 100, incomes
	);
	CHECK(incomes[0] == fixed_point_t { 20 });
	CHECK(incomes[1] == fixed_point_t { 80 });
}

TEST_CASE("RgoMath: pinning algorithm — five-round cascade", "[ecs_rgo][RgoMath]") {
	// Construct a fixture where pinning each subsequent employee forces the NEXT to fall
	// under its min wage. Five employees each of size 20; min wages rising 5, 10, 15, 20, 25.
	std::vector<Employee> employees(5);
	for (std::size_t i = 0; i < 5; ++i) {
		employees[i].hired_size = 20;
	}
	employees[0].minimum_wage_cached = fixed_point_t { 5 };
	employees[1].minimum_wage_cached = fixed_point_t { 30 };
	employees[2].minimum_wage_cached = fixed_point_t { 25 };
	employees[3].minimum_wage_cached = fixed_point_t { 20 };
	employees[4].minimum_wage_cached = fixed_point_t { 15 };

	std::vector<fixed_point_t> incomes(5);
	// Revenue 100. paid = 5 * 20 = 100. First pass: each gets 100/100 * 20 = 20.
	// e1 has min 30 > 20 → pin e1 to 30. revenue_left = 70, paid = 80.
	// 2nd pass: e0=70*20/80=17.5 (> 5 ok); e2 = 17.5 < 25 → pin e2 to 25. rl=45, paid=60.
	// 3rd pass: e0=45*20/60=15 (> 5 ok); e3=15 < 20 → pin e3 to 20. rl=25, paid=40.
	// 4th pass: e0=25*20/40=12.5; e4=12.5 < 15 → pin e4 to 15. rl=10, paid=20.
	// 5th pass: e0=10*20/20=10; settled.
	distribute_employee_incomes_min_wage_pinning(
		employees, fixed_point_t { 100 }, fixed_point_t { 95 }, 100, incomes
	);
	CHECK(incomes[0] == fixed_point_t { 10 });
	CHECK(incomes[1] == fixed_point_t { 30 });
	CHECK(incomes[2] == fixed_point_t { 25 });
	CHECK(incomes[3] == fixed_point_t { 20 });
	CHECK(incomes[4] == fixed_point_t { 15 });
}

TEST_CASE("RgoMath: pinning algorithm — slaves skipped", "[ecs_rgo][RgoMath]") {
	std::vector<Employee> employees(2);
	employees[0].hired_size = 50;
	employees[0].is_slave = true; // slave employee
	employees[0].minimum_wage_cached = fixed_point_t::_0;
	employees[1].hired_size = 50;
	employees[1].is_slave = false;
	employees[1].minimum_wage_cached = fixed_point_t { 5 };

	std::vector<fixed_point_t> incomes(2);
	// paid only counts non-slaves: 50. Revenue 100 / 50 * 50 = 100 for e1.
	distribute_employee_incomes_min_wage_pinning(
		employees, fixed_point_t { 100 }, fixed_point_t { 5 }, 50, incomes
	);
	CHECK(incomes[0] == fixed_point_t::_0); // slave gets nothing from the pinning algorithm
	CHECK(incomes[1] == fixed_point_t { 100 });
}

// ============================================================================
// compute_throughput_and_output_from_workers — the per-job-type fold.
// ============================================================================

TEST_CASE("RgoMath: throughput/output fold accumulates both effect types",
          "[ecs_rgo][RgoMath]") {
	ProductionTypeDef pt;
	Job job_throughput;
	job_throughput.pop_type_idx = 0;
	job_throughput.effect_type = JobEffectType::Throughput;
	job_throughput.effect_multiplier = fixed_point_t { 2 };
	job_throughput.amount = fixed_point_t::_0_50;
	pt.jobs.push_back(job_throughput);

	Job job_output;
	job_output.pop_type_idx = 1;
	job_output.effect_type = JobEffectType::Output;
	job_output.effect_multiplier = fixed_point_t { 3 };
	job_output.amount = fixed_point_t::_0_50;
	pt.jobs.push_back(job_output);

	std::vector<pop_size_t> employees_by_type = { 25, 25 };

	// max=100. fractions = 25/100 = 0.25 for both. 0.25 < 0.5 → effect = mul * fraction.
	// throughput += 2 * 0.25 = 0.5. output += 3 * 0.25 = 0.75.
	WorkersContribution const wc =
		compute_throughput_and_output_from_workers(pt, employees_by_type, 100);
	CHECK(wc.throughput_from_workers == fixed_point_t::_0_50);
	CHECK(wc.output_from_workers == fixed_point_t::_1 + fp::from_fraction<int32_t>(75, 100));
}
