#pragma once

#include "openvic-simulation/research/InventionManager.hpp"
#include "openvic-simulation/research/TechnologyManager.hpp"

namespace OpenVic {
	struct ResearchManager {
	private:
		TechnologyManager PROPERTY_REF(technology_manager);
		InventionManager PROPERTY_REF(invention_manager);
	};
}
