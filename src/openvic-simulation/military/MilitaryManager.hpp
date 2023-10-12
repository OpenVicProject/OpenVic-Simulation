#pragma once

#include "openvic-simulation/military/Unit.hpp"

namespace OpenVic {
	struct MilitaryManager {
	private:
		UnitManager unit_manager;
	public:
		REF_GETTERS(unit_manager)
	};
}
