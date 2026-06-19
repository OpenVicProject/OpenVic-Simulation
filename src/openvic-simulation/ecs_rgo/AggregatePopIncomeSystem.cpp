#include "openvic-simulation/ecs_rgo/AggregatePopIncomeSystem.hpp"

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic::ecs_rgo {

	void AggregatePopIncomeSystem::tick(
		ecs::TickContext const& ctx,
		PopWorkerIncome const& worker,
		PopOwnerIncome const& owner,
		PopIncomeTotals& totals
	) {
		(void) ctx;

		fixed_point_t const today = worker.rgo_worker_income_today + owner.rgo_owner_income_today;
		totals.total_income_today = today;
		totals.cash += today;
	}
}
