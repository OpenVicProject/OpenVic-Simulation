#pragma once

#include "openvic-simulation/scripts/ConditionManager.hpp"

namespace OpenVic {
	struct ScriptManager {
	private:
		ConditionManager PROPERTY_REF(condition_manager);
	};
}
