#pragma once

#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/diplomacy/DiplomaticAction.hpp"

namespace OpenVic {
	class DiplomacyManager {
		DiplomaticActionManager PROPERTY_REF(diplomatic_action_manager);
		CountryRelationManager PROPERTY_REF(country_relation_manager);
	};
}
