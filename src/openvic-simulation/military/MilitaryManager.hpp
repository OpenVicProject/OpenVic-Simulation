#pragma once

#include "openvic-simulation/military/DeploymentManager.hpp"
#include "openvic-simulation/military/LeaderTraitManager.hpp"
#include "openvic-simulation/military/UnitTypeManager.hpp"
#include "openvic-simulation/military/WargoalManager.hpp"

namespace OpenVic {
	struct MilitaryManager {
	private:
		UnitTypeManager PROPERTY_REF(unit_type_manager);
		LeaderTraitManager PROPERTY_REF(leader_trait_manager);
		DeploymentManager PROPERTY_REF(deployment_manager);
		WargoalTypeManager PROPERTY_REF(wargoal_type_manager);
	};
}
