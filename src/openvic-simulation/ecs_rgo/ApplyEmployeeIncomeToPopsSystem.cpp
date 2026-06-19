#include "openvic-simulation/ecs_rgo/ApplyEmployeeIncomeToPopsSystem.hpp"

#include <cstddef>

#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/ecs_rgo/Components.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic::ecs_rgo {

	void ApplyEmployeeIncomeToPopsSystem::tick(
		ecs::TickContext const& ctx,
		ecs::EntityID pop_id,
		PopLocation const& loc,
		PopWorkerIncome& out
	) {
		out.rgo_worker_income_today = fixed_point_t::_0;

		ProvinceRgoHired const* hired = ctx.world.get_component<ProvinceRgoHired>(loc.province_id);
		ProvinceRgoEmployeeIncome const* inc =
			ctx.world.get_component<ProvinceRgoEmployeeIncome>(loc.province_id);
		if (hired == nullptr || inc == nullptr) {
			return;
		}

		// At most one Employee entry per pop (hire iterates province pops once and emplaces a
		// single Employee per matched pop). Loop and stop at the first match.
		for (std::size_t i = 0; i < hired->employees.size(); ++i) {
			if (hired->employees[i].pop_id == pop_id) {
				if (i < inc->incomes.size()) {
					out.rgo_worker_income_today += inc->incomes[i];
				}
				break;
			}
		}
	}
}
