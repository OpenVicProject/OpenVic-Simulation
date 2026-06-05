#include "openvic-simulation/ecs_rgo/RgoResolveSellOrderAndOwnerShareSystem.hpp"

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoMath.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic::ecs_rgo {

	// Computes per-pop minimum-wage baseline. The legacy reaches into
	// `country.calculate_minimum_wage_base(pop.type)` — we stub it via a flat per-pop-type
	// table on the production-type registry for the reference implementation. Since the
	// reference impl doesn't carry a CountryInstance, this returns a fixed multiplier of the
	// hired_size: minimum_wage = (hired_size / Pop::size_denominator) * MIN_WAGE_BASE.
	//
	// MIN_WAGE_BASE is a per-pop-type constant supplied via the singleton-registered job
	// effect_multiplier on the owner job entry is a stretch — kept as a hardcoded fp constant
	// here so the math test can drive it explicitly.
	static fixed_point_t compute_employee_minimum_wage(Employee const& e) {
		// hired_size is already a count; the legacy `pop.size / size_denominator` is the
		// fraction-of-1 representation of the pop's size. For the reference impl we treat
		// hired_size directly as a fixed-point fraction by dividing by a fixed denominator
		// matching the legacy Pop::SIZE_DENOMINATOR (1000) — the absolute value isn't what
		// the test asserts; the test asserts the consistency of the distribution.
		static constexpr int32_t SIZE_DENOMINATOR = 1000;
		static constexpr int32_t MIN_WAGE_BASE_RAW = fixed_point_t::ONE / 10; // 0.1 per "unit pop"
		fixed_point_t const minimum_wage = fp::mul_div(
			fixed_point_t::parse_raw(static_cast<fixed_point_t::value_type>(MIN_WAGE_BASE_RAW)),
			static_cast<int32_t>(e.hired_size),
			SIZE_DENOMINATOR
		);
		return minimum_wage;
	}

	void RgoResolveSellOrderAndOwnerShareSystem::tick(
		ecs::TickContext const& ctx,
		ProvinceRgoConfig const& cfg,
		ProvinceRgoCacheTotals const& totals,
		ProvinceRgoHired& hired,
		ProvinceRgoSellOrder& order,
		ProvinceRgoResult& result
	) {
		// Reset the channels Stage 5+ will read. revenue_yesterday is zeroed even on the
		// no-request path so a stale value never bleeds through.
		result.revenue_yesterday = fixed_point_t::_0;
		result.owner_share = fixed_point_t::_0;
		result.total_minimum_wage = fixed_point_t::_0;

		if (cfg.production_type_idx == INVALID_IDX) {
			return;
		}

		// Stub for `market_instance.place_market_sell_order` — money_gained = unit_price * qty.
		if (order.has_request) {
			RgoMarketPriceTable const* prices = ctx.world.get_singleton<RgoMarketPriceTable>();
			if (prices != nullptr && order.good_idx < prices->unit_price.size()) {
				result.revenue_yesterday = prices->unit_price[order.good_idx] * order.quantity_requested;
			}
		}
		order.has_request = false;

		fixed_point_t const revenue = result.revenue_yesterday;
		if (revenue <= fixed_point_t::_0 || totals.total_worker_count_in_province <= 0) {
			return;
		}

		// Per-employee minimum-wage caching. Mirrors employee.update_minimum_wage(country).
		fixed_point_t total_min_wage = fixed_point_t::_0;
		for (Employee& e : hired.employees) {
			e.minimum_wage_cached = compute_employee_minimum_wage(e);
			total_min_wage += e.minimum_wage_cached;
		}
		result.total_minimum_wage = total_min_wage;

		// owner_share is non-zero only when there ARE owner pops AND revenue is sufficient
		// (revenue > total_min_wage). Otherwise Stage 5b takes the proportional branch.
		if (totals.total_owner_count_in_state > 0 && revenue > total_min_wage) {
			result.owner_share = detail::compute_owner_share(
				totals.total_owner_count_in_state,
				totals.total_worker_count_in_province,
				revenue,
				total_min_wage
			);
		}
	}
}
