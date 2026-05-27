#include "openvic-simulation/ecs_rgo/RgoComputeOwnerIncomeSystem.hpp"

#include <algorithm>

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/Math.hpp"

namespace OpenVic::ecs_rgo {

	void RgoComputeOwnerIncomeSystem::tick(
		ecs::TickContext const& ctx,
		ProvinceRgoConfig const& cfg,
		ProvinceLocation const& loc,
		ProvinceRgoResult const& result,
		ProvinceRgoCacheTotals const& totals,
		ProvinceRgoOwnerIncome& out
	) {
		out.total_owner_income = fixed_point_t::_0;

		if (cfg.production_type_idx == INVALID_IDX) {
			return;
		}
		if (result.owner_share <= fixed_point_t::_0 || totals.total_owner_count_in_state <= 0) {
			return;
		}

		RgoProductionTypeRegistry const* registry = ctx.world.get_singleton<RgoProductionTypeRegistry>();
		if (registry == nullptr || cfg.production_type_idx >= registry->production_types.size()) {
			return;
		}
		ProductionTypeDef const& pt = registry->production_types[cfg.production_type_idx];
		if (!pt.owner.has_value()) {
			return;
		}
		StateOwnerPopList const* sop = ctx.world.get_component<StateOwnerPopList>(loc.state_id);
		if (sop == nullptr) {
			return;
		}
		pop_type_idx_t const owner_type_idx = pt.owner->pop_type_idx;
		if (owner_type_idx >= sop->pops_by_type.size()) {
			return;
		}

		// Per-pop owner income = revenue * owner_share * pop.size / total_owner_count, then
		// epsilon-floored — matches legacy pay_employees owner branch.
		fixed_point_t const owner_pool = result.revenue_yesterday * result.owner_share;
		for (ecs::EntityID owner_pop_id : sop->pops_by_type[owner_type_idx]) {
			PopCore const* pc = ctx.world.get_component<PopCore>(owner_pop_id);
			if (pc == nullptr) {
				continue;
			}
			fixed_point_t const pop_income = std::max(
				fp::mul_div<pop_sum_t>(
					owner_pool,
					static_cast<pop_sum_t>(pc->size),
					totals.total_owner_count_in_state
				),
				fixed_point_t::epsilon
			);
			out.total_owner_income += pop_income;
		}
	}
}
