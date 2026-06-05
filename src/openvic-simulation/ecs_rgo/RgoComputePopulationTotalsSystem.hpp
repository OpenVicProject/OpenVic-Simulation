#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 1 — recomputes per-province population totals required by every downstream stage.
	//
	// Writes:    ProvinceRgoCacheTotals.{total_worker_count_in_province, total_owner_count_in_state}
	// Reads:     ProvinceRgoConfig, ProvinceLocation, ProvincePopList
	// Extra:     PopCore, StateOwnerPopList (cross-archetype)
	//
	// total_worker_count = sum of pop sizes whose pop_type_idx matches one of the production
	// type's job pop types (NOT equivalents, mirroring the legacy `not counting equivalents`
	// comment).
	// total_owner_count = state-wide pop-size sum for the production type's owner pop type, if
	// any.
	struct RgoComputePopulationTotalsSystem : ecs::SystemThreaded<RgoComputePopulationTotalsSystem> {
		void tick(
			ecs::TickContext const& ctx,
			ProvinceRgoConfig const& cfg,
			ProvinceLocation const& loc,
			ProvincePopList const& pop_list,
			ProvinceRgoCacheTotals& totals
		);

		static constexpr std::array<ecs::component_type_id_t, 2> extra_reads() {
			return {
				ecs::component_type_id_of<PopCore>(),
				ecs::component_type_id_of<StateOwnerPopList>()
			};
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::RgoComputePopulationTotalsSystem)
