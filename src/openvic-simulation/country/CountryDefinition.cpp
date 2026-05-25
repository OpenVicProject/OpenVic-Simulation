#include "CountryDefinition.hpp"

#include <string_view>

#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;

CountryDefinition::CountryDefinition(
	std::string_view new_identifier,
	colour_t new_colour,
	index_t new_index,
	GraphicalCultureType const& new_graphical_culture,
	std::remove_const_t<decltype(parties)>&& new_parties,
	decltype(ship_names)&& new_ship_names,
	bool new_is_dynamic_tag,
	decltype(alternative_colours)&& new_alternative_colours,
	colour_t new_primary_unit_colour,
	colour_t new_secondary_unit_colour,
	colour_t new_tertiary_unit_colour
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	HasIndex { new_index },
	graphical_culture { new_graphical_culture },
	parties { std::move(new_parties) },
	ship_names { std::move(new_ship_names) },
	is_dynamic_tag { new_is_dynamic_tag },
	alternative_colours { std::move(new_alternative_colours) },
	primary_unit_colour { new_primary_unit_colour },
	secondary_unit_colour { new_secondary_unit_colour },
	tertiary_unit_colour { new_tertiary_unit_colour } {}

template struct fmt::formatter<OpenVic::CountryDefinition>;
