#include "openvic-simulation/ecs_rgo/RgoProduceAndPlaceOrderSystem.hpp"

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoMath.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/Math.hpp"

namespace OpenVic::ecs_rgo {

	void RgoProduceAndPlaceOrderSystem::tick(
		ecs::TickContext const& ctx,
		ProvinceRgoConfig const& cfg,
		ProvinceRgoHired const& hired,
		ProvinceRgoModifiers const& mods,
		ProvinceRgoFarmMineModifiers const& fm,
		ProvinceRgoGoodModifiers const& gm,
		ProvinceLocation const& loc,
		ProvinceRgoCacheTotals const& totals,
		ProvinceRgoResult& result,
		ProvinceRgoSellOrder& order
	) {
		// Always start by zeroing the output channel so that a subsequent early-out doesn't
		// leak last tick's value (mirrors `output_quantity_yesterday = 0` in the legacy early-out).
		result.output_quantity_yesterday = fixed_point_t::_0;
		order.has_request = false;
		order.quantity_requested = fixed_point_t::_0;
		order.good_idx = INVALID_IDX;

		if (cfg.production_type_idx == INVALID_IDX) {
			return;
		}
		// Legacy: rgo_tick zeros revenue + output and returns if owner country is null.
		if (loc.owner_country_id == ecs::EntityID {}) {
			return;
		}

		RgoProductionTypeRegistry const* registry = ctx.world.get_singleton<RgoProductionTypeRegistry>();
		RgoGameRules const* rules = ctx.world.get_singleton<RgoGameRules>();
		if (registry == nullptr || rules == nullptr
			|| cfg.production_type_idx >= registry->production_types.size()) {
			return;
		}
		ProductionTypeDef const& pt = registry->production_types[cfg.production_type_idx];

		fixed_point_t const size_modifier = detail::calculate_size_modifier(pt, fm, gm, *rules);
		if (size_modifier == fixed_point_t::_0) {
			return;
		}
		if (hired.max_employee_count <= 0) {
			return;
		}

		// Owner-job contribution to the multipliers — scaled by total_owner_count_in_state /
		// state_total_population (legacy fp::mul_div).
		fixed_point_t owner_throughput_contribution = fixed_point_t::_0;
		fixed_point_t owner_output_contribution = fixed_point_t::_0;
		if (pt.owner.has_value() && totals.total_owner_count_in_state > 0) {
			StateOwnerPopList const* sop = ctx.world.get_component<StateOwnerPopList>(loc.state_id);
			if (sop != nullptr && sop->total_population > 0) {
				switch (pt.owner->effect_type) {
					case JobEffectType::Output:
						owner_output_contribution = fp::mul_div(
							pt.owner->effect_multiplier,
							totals.total_owner_count_in_state,
							sop->total_population
						);
						break;
					case JobEffectType::Throughput:
						owner_throughput_contribution = fp::mul_div(
							pt.owner->effect_multiplier,
							totals.total_owner_count_in_state,
							sop->total_population
						);
						break;
					case JobEffectType::Input:
						// Legacy quirk: INPUT effect on the owner job is logged-and-ignored.
						break;
				}
			}
		}

		detail::FarmMineClassification const cls = detail::resolve_farm_mine_classification(pt, *rules);
		fixed_point_t const throughput_multiplier = detail::compose_throughput_multiplier(
			cls, mods, fm, gm, owner_throughput_contribution
		);
		fixed_point_t const output_multiplier = detail::compose_output_multiplier(
			cls, mods, fm, gm, owner_output_contribution
		);

		detail::WorkersContribution const wc = detail::compute_throughput_and_output_from_workers(
			pt, hired.employee_count_per_type, hired.max_employee_count
		);

		// Final output — same product as legacy produce().
		fixed_point_t const output =
			pt.base_output_quantity
			* size_modifier * cfg.size_multiplier
			* throughput_multiplier * wc.throughput_from_workers
			* output_multiplier * wc.output_from_workers;

		result.output_quantity_yesterday = output;
		if (output > fixed_point_t::_0) {
			order.good_idx = pt.output_good_idx;
			order.quantity_requested = output;
			order.has_request = true;
		}
	}
}
