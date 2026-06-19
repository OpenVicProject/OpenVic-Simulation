#include "openvic-simulation/ecs_rgo/RgoHireSystem.hpp"

#include <cstddef>

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Singletons.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/Math.hpp"

namespace OpenVic::ecs_rgo {

	void RgoHireSystem::tick(
		ecs::TickContext const& ctx,
		ProvinceRgoConfig const& cfg,
		ProvinceLocation const& loc,
		ProvincePopList const& pop_list,
		ProvinceRgoCacheTotals const& totals,
		ProvinceRgoHired& hired
	) {
		(void) loc;

		// Reset the per-tick state. `.clear()` keeps capacity — the constructor reserves
		// worst-case (one Employee per pop in the province).
		hired.employees.clear();
		for (pop_size_t& v : hired.employee_count_per_type) {
			v = 0;
		}
		hired.total_employees = 0;
		hired.total_paid_employees = 0;

		if (cfg.production_type_idx == INVALID_IDX) {
			return;
		}
		if (hired.max_employee_count <= 0) {
			return;
		}
		if (totals.total_worker_count_in_province <= 0) {
			return;
		}

		RgoProductionTypeRegistry const* registry = ctx.world.get_singleton<RgoProductionTypeRegistry>();
		if (registry == nullptr || cfg.production_type_idx >= registry->production_types.size()) {
			return;
		}
		ProductionTypeDef const& pt = registry->production_types[cfg.production_type_idx];

		pop_sum_t const available_worker_count = totals.total_worker_count_in_province;
		fixed_point_t const proportion_to_hire =
			(static_cast<pop_sum_t>(hired.max_employee_count) >= available_worker_count)
				? fixed_point_t::_1
				: fp::from_fraction(
					static_cast<int64_t>(hired.max_employee_count),
					static_cast<int64_t>(available_worker_count)
				);

		for (ecs::EntityID pop_id : pop_list.pop_ids) {
			PopCore const* pc = ctx.world.get_component<PopCore>(pop_id);
			if (pc == nullptr) {
				continue;
			}
			for (Job const& job : pt.jobs) {
				if (pc->pop_type_idx != job.pop_type_idx) {
					continue;
				}
				pop_size_t const pop_size_to_hire =
					(proportion_to_hire * fixed_point_t { pc->size }).floor<pop_size_t>();
				if (pop_size_to_hire <= 0) {
					break;
				}
				if (pc->pop_type_idx < hired.employee_count_per_type.size()) {
					hired.employee_count_per_type[pc->pop_type_idx] += pop_size_to_hire;
				}

				Employee e;
				e.pop_id = pop_id;
				e.pop_type_idx = pc->pop_type_idx;
				e.hired_size = pop_size_to_hire;
				e.is_slave = job.is_slave;
				e.minimum_wage_cached = fixed_point_t::_0; // filled in by Stage 4
				hired.employees.push_back(e);

				hired.total_employees += pop_size_to_hire;
				if (!job.is_slave) {
					hired.total_paid_employees += pop_size_to_hire;
				}
				break;
			}
		}
	}
}
