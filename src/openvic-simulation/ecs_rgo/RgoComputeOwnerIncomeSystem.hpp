#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoResolveSellOrderAndOwnerShareSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 5a — sums the province's owner-income contribution (the per-state owner-pop
	// total). Runs in parallel with Stage 5b — both read ProvinceRgoResult (which carries
	// owner_share) and write disjoint output components.
	//
	// Writes:    ProvinceRgoOwnerIncome.total_owner_income
	// Reads:     ProvinceRgoConfig, ProvinceLocation, ProvinceRgoResult, ProvinceRgoCacheTotals
	// Extra:     StateOwnerPopList, PopCore
	// run_after: RgoResolveSellOrderAndOwnerShareSystem
	struct RgoComputeOwnerIncomeSystem : ecs::SystemThreaded<RgoComputeOwnerIncomeSystem> {
		void tick(
			ecs::TickContext const& ctx,
			ProvinceRgoConfig const& cfg,
			ProvinceLocation const& loc,
			ProvinceRgoResult const& result,
			ProvinceRgoCacheTotals const& totals,
			ProvinceRgoOwnerIncome& out
		);

		static constexpr std::array<ecs::component_type_id_t, 2> extra_reads() {
			return {
				ecs::component_type_id_of<StateOwnerPopList>(),
				ecs::component_type_id_of<PopCore>()
			};
		}

		static constexpr std::array<ecs::system_type_id_t, 1> declared_run_after() {
			return { ecs::system_type_id_of<RgoResolveSellOrderAndOwnerShareSystem>() };
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::RgoComputeOwnerIncomeSystem)
