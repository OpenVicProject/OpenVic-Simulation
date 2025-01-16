#include "Leader.hpp"

using namespace OpenVic;

LeaderBase::LeaderBase(
	std::string_view new_name,
	UnitType::branch_t new_branch,
	Date new_date,
	LeaderTrait const* new_personality,
	LeaderTrait const* new_background,
	fixed_point_t new_prestige,
	std::string_view new_picture
) : name { new_name },
	branch { new_branch },
	date { new_date },
	personality { new_personality },
	background { new_background },
	prestige { new_prestige },
	picture { new_picture } {}
