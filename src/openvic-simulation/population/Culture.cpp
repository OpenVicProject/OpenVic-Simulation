#include "Culture.hpp"

#include <string_view>

#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;

GraphicalCultureType::GraphicalCultureType(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

CultureGroup::CultureGroup(
	std::string_view new_identifier, std::string_view new_leader, GraphicalCultureType const& new_unit_graphical_culture_type,
	bool new_is_overseas, std::optional<country_index_t> new_union_country
) : HasIdentifier { new_identifier }, leader { new_leader }, unit_graphical_culture_type { new_unit_graphical_culture_type },
	is_overseas { new_is_overseas }, union_country { new_union_country } {}

Culture::Culture(
	std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group, memory::vector<memory::string>&& new_first_names,
	memory::vector<memory::string>&& new_last_names, fixed_point_t new_radicalism, std::optional<country_index_t> new_primary_country
) : HasIdentifierAndColour { new_identifier, new_colour, false }, group { new_group },
	first_names { std::move(new_first_names) }, last_names { std::move(new_last_names) }, radicalism { new_radicalism },
	primary_country { new_primary_country } {}
