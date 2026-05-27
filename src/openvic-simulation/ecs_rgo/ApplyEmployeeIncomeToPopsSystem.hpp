#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoComputeEmployeeIncomeSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 6a — applies worker-side income to each hired pop.
	//
	// Writes:    PopWorkerIncome.rgo_worker_income_today
	// Reads:     PopLocation
	// Extra:     ProvinceRgoHired, ProvinceRgoEmployeeIncome
	// run_after: RgoComputeEmployeeIncomeSystem
	//
	// The deliberate inversion (see ECS.md / the RGO plan): instead of a province writing
	// into its hired pops directly (which would require extra_writes — which the ECS only
	// supports as extra_reads), each pop scans its province's employees list looking for the
	// entry matching its own EntityID and adds the corresponding incomes[i] entry. Walk is
	// O(employees_in_my_province) per pop; total work per tick is O(total_employees).
	struct ApplyEmployeeIncomeToPopsSystem : ecs::SystemThreaded<ApplyEmployeeIncomeToPopsSystem> {
		void tick(
			ecs::TickContext const& ctx,
			ecs::EntityID pop_id,
			PopLocation const& loc,
			PopWorkerIncome& out
		);

		static constexpr std::array<ecs::component_type_id_t, 2> extra_reads() {
			return {
				ecs::component_type_id_of<ProvinceRgoHired>(),
				ecs::component_type_id_of<ProvinceRgoEmployeeIncome>()
			};
		}

		static constexpr std::array<ecs::system_type_id_t, 1> declared_run_after() {
			return { ecs::system_type_id_of<RgoComputeEmployeeIncomeSystem>() };
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::ApplyEmployeeIncomeToPopsSystem)
