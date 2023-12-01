#pragma once

#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/military/Unit.hpp"
#include "openvic-simulation/military/Wargoal.hpp"

namespace OpenVic {
	struct MilitaryManager {
	private:
		UnitManager PROPERTY_REF(unit_manager);
		LeaderTraitManager PROPERTY_REF(leader_trait_manager);
		DeploymentManager PROPERTY_REF(deployment_manager);
		WargoalTypeManager PROPERTY_REF(wargoal_manager);
	};
}
