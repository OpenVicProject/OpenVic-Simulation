#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoResolveSellOrderAndOwnerShareSystem.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 5b — distributes the worker-side revenue among the hired employees.
	//
	// Writes:    ProvinceRgoEmployeeIncome.{incomes[], total_employee_income}
	// Reads:     ProvinceRgoConfig, ProvinceRgoHired, ProvinceRgoResult
	// run_after: RgoResolveSellOrderAndOwnerShareSystem
	//
	// Three branches reproducing the legacy pay_employees worker behaviour:
	//   - revenue <= total_minimum_wage      → proportional distribution by min_wage
	//   - total_paid_employees == 0          → slaves only, revenue is silently lost
	//   - otherwise                          → min-wage-pinning algorithm (clean rewrite)
	//
	// "Sufficient" revenue uses revenue_left = revenue_yesterday * (1 - owner_share). Stage 4
	// guarantees that owner_share is 0 in the insufficient-revenue branch so this division is
	// safe (revenue_yesterday > 0 by an earlier check).
	struct RgoComputeEmployeeIncomeSystem : ecs::SystemThreaded<RgoComputeEmployeeIncomeSystem> {
		void tick(
			ecs::TickContext const& ctx,
			ProvinceRgoConfig const& cfg,
			ProvinceRgoHired const& hired,
			ProvinceRgoResult const& result,
			ProvinceRgoEmployeeIncome& out
		);

		static constexpr std::array<ecs::system_type_id_t, 1> declared_run_after() {
			return { ecs::system_type_id_of<RgoResolveSellOrderAndOwnerShareSystem>() };
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::RgoComputeEmployeeIncomeSystem)
