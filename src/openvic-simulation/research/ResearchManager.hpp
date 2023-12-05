#pragma once

#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/research/Technology.hpp"

namespace OpenVic {
	struct ResearchManager {
	private:
		TechnologyManager PROPERTY_REF(technology_manager);
		InventionManager PROPERTY_REF(invention_manager);
	};
}
