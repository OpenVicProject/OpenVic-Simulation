#pragma once

#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/military/UnitType.hpp"

namespace OpenVic {
	struct Leader {
	private:
		std::string        PROPERTY(name);
		UnitType::branch_t PROPERTY(branch); /* type in defines */
		Date               PROPERTY(date);
		LeaderTrait const* PROPERTY(personality);
		LeaderTrait const* PROPERTY(background);
		fixed_point_t      PROPERTY(prestige);
		std::string        PROPERTY(picture);

	public:
		Leader(
			std::string_view new_name, UnitType::branch_t new_branch, Date new_date, LeaderTrait const* new_personality,
			LeaderTrait const* new_background, fixed_point_t new_prestige, std::string_view new_picture
		);
	};
}