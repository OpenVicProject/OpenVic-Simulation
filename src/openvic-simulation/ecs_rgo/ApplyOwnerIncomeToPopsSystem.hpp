#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoComputeOwnerIncomeSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 6b — applies owner-side income to each owner pop.
	//
	// Writes:    PopOwnerIncome.rgo_owner_income_today
	// Reads:     PopLocation, PopCore
	// Extra:     StateProvinceList, ProvinceRgoConfig, ProvinceRgoResult, ProvinceRgoCacheTotals
	// run_after: RgoComputeOwnerIncomeSystem
	//
	// For each pop, walks the pop's state's province list; for every province whose RGO has an
	// owner job whose pop_type matches this pop's pop_type_idx, the pop accumulates its share
	// of that province's owner-pool (revenue * owner_share * pop.size / total_owner_count).
	// Matches the same formula Stage 5a uses for its province-level aggregate so the worker-
	// and owner-side accounting agree.
	struct ApplyOwnerIncomeToPopsSystem : ecs::SystemThreaded<ApplyOwnerIncomeToPopsSystem> {
		void tick(
			ecs::TickContext const& ctx,
			PopLocation const& loc,
			PopCore const& core,
			PopOwnerIncome& out
		);

		static constexpr std::array<ecs::component_type_id_t, 4> extra_reads() {
			return {
				ecs::component_type_id_of<StateProvinceList>(),
				ecs::component_type_id_of<ProvinceRgoConfig>(),
				ecs::component_type_id_of<ProvinceRgoResult>(),
				ecs::component_type_id_of<ProvinceRgoCacheTotals>()
			};
		}

		static constexpr std::array<ecs::system_type_id_t, 1> declared_run_after() {
			return { ecs::system_type_id_of<RgoComputeOwnerIncomeSystem>() };
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::ApplyOwnerIncomeToPopsSystem)
