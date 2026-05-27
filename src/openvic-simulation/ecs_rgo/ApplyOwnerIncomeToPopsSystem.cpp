#include "openvic-simulation/ecs_rgo/ApplyOwnerIncomeToPopsSystem.hpp"

#include <algorithm>

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/Math.hpp"

namespace OpenVic::ecs_rgo {

	void ApplyOwnerIncomeToPopsSystem::tick(
		ecs::TickContext const& ctx,
		PopLocation const& loc,
		PopCore const& core,
		PopOwnerIncome& out
	) {
		out.rgo_owner_income_today = fixed_point_t::_0;

		StateProvinceList const* spl = ctx.world.get_component<StateProvinceList>(loc.state_id);
		if (spl == nullptr) {
			return;
		}
		RgoProductionTypeRegistry const* registry = ctx.world.get_singleton<RgoProductionTypeRegistry>();
		if (registry == nullptr) {
			return;
		}

		for (ecs::EntityID prov_id : spl->province_ids) {
			ProvinceRgoConfig const* prov_cfg = ctx.world.get_component<ProvinceRgoConfig>(prov_id);
			ProvinceRgoResult const* prov_res = ctx.world.get_component<ProvinceRgoResult>(prov_id);
			ProvinceRgoCacheTotals const* prov_totals =
				ctx.world.get_component<ProvinceRgoCacheTotals>(prov_id);
			if (prov_cfg == nullptr || prov_res == nullptr || prov_totals == nullptr) {
				continue;
			}
			if (prov_cfg->production_type_idx == INVALID_IDX) {
				continue;
			}
			if (prov_res->owner_share <= fixed_point_t::_0
				|| prov_totals->total_owner_count_in_state <= 0) {
				continue;
			}
			if (prov_cfg->production_type_idx >= registry->production_types.size()) {
				continue;
			}
			ProductionTypeDef const& pt = registry->production_types[prov_cfg->production_type_idx];
			if (!pt.owner.has_value()) {
				continue;
			}
			if (pt.owner->pop_type_idx != core.pop_type_idx) {
				continue;
			}

			// Same expression Stage 5a sums into total_owner_income (legacy pay_employees owner
			// branch). Epsilon-floored to match the legacy "rounding up" comment.
			fixed_point_t const owner_pool = prov_res->revenue_yesterday * prov_res->owner_share;
			fixed_point_t const pop_income = std::max(
				fp::mul_div<pop_sum_t>(
					owner_pool,
					static_cast<pop_sum_t>(core.size),
					prov_totals->total_owner_count_in_state
				),
				fixed_point_t::epsilon
			);
			out.rgo_owner_income_today += pop_income;
		}
	}
}
