#pragma once

#include "openvic-simulation/scripts/Condition.hpp"

namespace OpenVic {
	struct DefinitionManager;

	struct ScriptManager {
	private:
		ConditionManager PROPERTY_REF(condition_manager);

	public:
		ScriptManager(DefinitionManager const& definition_manager) : condition_manager { definition_manager } {}
	};
}
