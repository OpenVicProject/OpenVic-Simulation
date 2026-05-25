#include "Religion.hpp"

using namespace OpenVic;

ReligionGroup::ReligionGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Religion::Religion(
	std::string_view new_identifier,
	colour_t new_colour,
	ReligionGroup const& new_group,
	icon_t new_icon,
	bool new_is_pagan
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	group { new_group },
	icon { new_icon },
	is_pagan { new_is_pagan } {}
