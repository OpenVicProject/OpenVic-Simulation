#include "BuildingInstance.hpp"

using namespace OpenVic;

BuildingInstance::BuildingInstance(BuildingType const& new_building_type, building_level_t new_level)
  : HasIdentifier { new_building_type },
	building_type { new_building_type },
	level { new_level } {}

bool BuildingInstance::_can_expand() const {
	return level < building_type.get_max_level();
}

bool BuildingInstance::expand() {
	if (expansion_state == ExpansionState::CanExpand) {
		expansion_state = ExpansionState::Preparing;
		expansion_progress = 0;
		return true;
	}
	return false;
}

/* REQUIREMENTS:
 * MAP-71, MAP-74, MAP-77
 */
void BuildingInstance::update_gamestate(Date today) {
	switch (expansion_state) {
	case ExpansionState::Preparing:
		start_date = today;
		end_date = start_date + building_type.get_build_time();
		break;
	case ExpansionState::Expanding:
		expansion_progress = fixed_point_t { (today - start_date).to_int() } / (end_date - start_date).to_int();
		break;
	default: expansion_state = _can_expand() ? ExpansionState::CanExpand : ExpansionState::CannotExpand;
	}
}

void BuildingInstance::tick(Date today) {
	if (expansion_state == ExpansionState::Preparing) {
		expansion_state = ExpansionState::Expanding;
	}
	if (expansion_state == ExpansionState::Expanding) {
		if (end_date <= today) {
			level++;
			expansion_state = ExpansionState::CannotExpand;
		}
	}
}
