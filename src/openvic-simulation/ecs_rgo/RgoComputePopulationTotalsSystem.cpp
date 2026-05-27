#include "openvic-simulation/ecs_rgo/RgoComputePopulationTotalsSystem.hpp"

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"

namespace OpenVic::ecs_rgo {

	void RgoComputePopulationTotalsSystem::tick(
		ecs::TickContext const& ctx,
		ProvinceRgoConfig const& cfg,
		ProvinceLocation const& loc,
		ProvincePopList const& pop_list,
		ProvinceRgoCacheTotals& totals
	) {
		// Inactive RGO: zero everything, mirroring the legacy "production_type_nullable == nullptr"
		// early-out in rgo_tick. Downstream stages also early-out on this condition; resetting
		// here keeps the cached totals visible to UI / inspection from being stale.
		totals.total_worker_count_in_province = 0;
		totals.total_owner_count_in_state = 0;
		if (cfg.production_type_idx == INVALID_IDX) {
			return;
		}

		RgoProductionTypeRegistry const* registry = ctx.world.get_singleton<RgoProductionTypeRegistry>();
		if (registry == nullptr || cfg.production_type_idx >= registry->production_types.size()) {
			return;
		}
		ProductionTypeDef const& pt = registry->production_types[cfg.production_type_idx];

		// Worker total — sum sizes of pops whose pop_type_idx matches one of the production
		// type's job pop_types. Walk PopCore through ProvincePopList::pop_ids (the static-for-
		// fixture-lifetime pop list).
		for (ecs::EntityID pop_id : pop_list.pop_ids) {
			PopCore const* pc = ctx.world.get_component<PopCore>(pop_id);
			if (pc == nullptr) {
				continue;
			}
			for (Job const& job : pt.jobs) {
				if (pc->pop_type_idx == job.pop_type_idx) {
					totals.total_worker_count_in_province += static_cast<pop_sum_t>(pc->size);
					break;
				}
			}
		}

		// Owner total — only if the production type has an owner job. Reads the state's owner-
		// pop list via StateOwnerPopList; the legacy looks this up through
		// state->get_population_by_type(owner_pop_type).
		if (pt.owner.has_value()) {
			pop_type_idx_t const owner_type_idx = pt.owner->pop_type_idx;
			StateOwnerPopList const* sop = ctx.world.get_component<StateOwnerPopList>(loc.state_id);
			if (sop != nullptr && owner_type_idx < sop->pops_by_type.size()) {
				for (ecs::EntityID owner_pop_id : sop->pops_by_type[owner_type_idx]) {
					PopCore const* pc = ctx.world.get_component<PopCore>(owner_pop_id);
					if (pc != nullptr) {
						totals.total_owner_count_in_state += static_cast<pop_sum_t>(pc->size);
					}
				}
			}
		}
	}
}
