#include "BuildingInstance.hpp"

#include "openvic-simulation/economy/BuildingType.hpp"

using namespace OpenVic;

BuildingInstance::BuildingInstance(BuildingType const& new_building_type, building_level_t new_level)
  : HasIdentifier { new_building_type },
	building_type { new_building_type },
	level { new_level } {}

bool BuildingInstance::_can_expand() const {
	return level < building_type.max_level;
}

bool BuildingInstance::expand(
	ModifierEffectCache const& modifier_effect_cache,
	CountryInstance& actor,
	ProvinceInstance const& location
) {
	if (expansion_state != ExpansionState::CanExpand) {
		return false;
	}

	if (!building_type.can_be_built_in(
		modifier_effect_cache,
		level+1,
		actor,
		location
	)) {
		return false;
	}

	//TODO add construction costs to actor
	expansion_state = ExpansionState::Preparing;
	expansion_progress = 0;
	return true;
}

/* REQUIREMENTS:
 * MAP-71, MAP-74, MAP-77
 */
void BuildingInstance::update_gamestate(Date today) {
	switch (expansion_state) {
	case ExpansionState::Preparing:
		start_date = today;
		end_date = start_date + building_type.build_time;
		break;
	case ExpansionState::Expanding:
		expansion_progress = fixed_point_t { static_cast<int32_t>((today - start_date).to_int()) }
			/ static_cast<int32_t>((end_date - start_date).to_int());
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

void BuildingInstance::set_level(const building_level_t new_level) {
	if (new_level == level) {
		return;
	}

	if (new_level < level) {
		if (new_level < building_level_t(0)) {
			spdlog::error_s("Cannot set building level to {}, the minimum is 0.", new_level);
			level = building_level_t(0);
		} else {
			level = new_level;
		}
	} else if (new_level > building_type.max_level) {
		spdlog::error_s("Cannot set building level to {}, the maximum is {}.", new_level, building_type.max_level);
		level = building_type.max_level;
	} else {
		level = new_level;
	}
}