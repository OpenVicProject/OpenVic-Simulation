#pragma once

#include <array>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs_rgo/ApplyEmployeeIncomeToPopsSystem.hpp"
#include "openvic-simulation/ecs_rgo/ApplyOwnerIncomeToPopsSystem.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"

namespace OpenVic::ecs_rgo {

	// Stage 7 — sums the per-source pop incomes into PopIncomeTotals (and into `cash`).
	//
	// Writes:    PopIncomeTotals.{total_income_today, cash}
	// Reads:     PopWorkerIncome, PopOwnerIncome
	// run_after: ApplyEmployeeIncomeToPopsSystem, ApplyOwnerIncomeToPopsSystem
	//
	// total_income_today = worker + owner (both daily-zeroed). cash accumulates across ticks,
	// mirroring the legacy `Pop::pay_income_tax → cash += amount` chain (we don't model the
	// income-tax leak here — a future system would).
	struct AggregatePopIncomeSystem : ecs::SystemThreaded<AggregatePopIncomeSystem> {
		void tick(
			ecs::TickContext const& ctx,
			PopWorkerIncome const& worker,
			PopOwnerIncome const& owner,
			PopIncomeTotals& totals
		);

		static constexpr std::array<ecs::system_type_id_t, 2> declared_run_after() {
			return {
				ecs::system_type_id_of<ApplyEmployeeIncomeToPopsSystem>(),
				ecs::system_type_id_of<ApplyOwnerIncomeToPopsSystem>()
			};
		}
	};
}

ECS_SYSTEM(OpenVic::ecs_rgo::AggregatePopIncomeSystem)
