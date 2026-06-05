#include "openvic-simulation/ecs_rgo/RgoComputeEmployeeIncomeSystem.hpp"

#include <cstddef>

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/RgoMath.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic::ecs_rgo {

	void RgoComputeEmployeeIncomeSystem::tick(
		ecs::TickContext const& ctx,
		ProvinceRgoConfig const& cfg,
		ProvinceRgoHired const& hired,
		ProvinceRgoResult const& result,
		ProvinceRgoEmployeeIncome& out
	) {
		(void) ctx;

		// Resize incomes to match the hire list (worst-case capacity already reserved at
		// fixture build, so this is a count change, not a reallocation).
		out.incomes.assign(hired.employees.size(), fixed_point_t::_0);
		out.total_employee_income = fixed_point_t::_0;

		if (cfg.production_type_idx == INVALID_IDX) {
			return;
		}
		if (hired.employees.empty()) {
			return;
		}
		if (result.revenue_yesterday <= fixed_point_t::_0) {
			return;
		}

		fixed_point_t const total_min_wage = result.total_minimum_wage;
		fixed_point_t const revenue = result.revenue_yesterday;

		if (revenue <= total_min_wage) {
			// Insufficient revenue — distribute proportionally to minimum wages. owner_share
			// is guaranteed 0 by Stage 4 in this branch.
			if (total_min_wage <= fixed_point_t::_0) {
				return;
			}
			detail::distribute_insufficient_revenue_proportional(
				hired.employees, revenue, total_min_wage, out.incomes
			);
		} else {
			if (hired.total_paid_employees == 0) {
				// Slaves-only: legacy quirk — revenue is silently lost.
				return;
			}
			fixed_point_t const revenue_left =
				revenue * (fixed_point_t::_1 - result.owner_share);
			detail::distribute_employee_incomes_min_wage_pinning(
				hired.employees,
				revenue_left,
				total_min_wage,
				hired.total_paid_employees,
				out.incomes
			);
		}

		// Sum non-slave incomes for the aggregate (legacy total_employee_income_cache).
		for (std::size_t i = 0; i < hired.employees.size(); ++i) {
			if (hired.employees[i].is_slave) {
				continue;
			}
			if (out.incomes[i] > fixed_point_t::_0) {
				out.total_employee_income += out.incomes[i];
			}
		}
	}
}
