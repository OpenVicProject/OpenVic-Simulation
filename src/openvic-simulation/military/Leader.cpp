#include "Leader.hpp"

using namespace OpenVic;

LeaderBase::LeaderBase(
	std::string_view new_name,
	unit_branch_t new_branch,
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

void LeaderBase::set_picture(std::string_view new_picture) {
	picture = new_picture;
}

LeaderInstance::LeaderInstance(unique_id_t new_unique_id, LeaderBase const& leader_base, CountryInstance const& new_country)
	: LeaderBase { leader_base }, unique_id { new_unique_id }, country { new_country } {}
