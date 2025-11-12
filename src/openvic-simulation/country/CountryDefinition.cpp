#include "CountryDefinition.hpp"

#include <string_view>

#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

CountryDefinition::CountryDefinition(
	std::string_view new_identifier,
	colour_t new_colour,
	index_t new_index,
	GraphicalCultureType const& new_graphical_culture,
	IdentifierRegistry<CountryParty>&& new_parties,
	unit_names_map_t&& new_unit_names,
	bool new_dynamic_tag,
	government_colour_map_t&& new_alternative_colours,
	colour_t new_primary_unit_colour,
	colour_t new_secondary_unit_colour,
	colour_t new_tertiary_unit_colour
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	HasIndex { new_index },
	graphical_culture { new_graphical_culture },
	parties { std::move(new_parties) },
	unit_names { std::move(new_unit_names) },
	dynamic_tag { new_dynamic_tag },
	alternative_colours { std::move(new_alternative_colours) },
	primary_unit_colour { new_primary_unit_colour },
	secondary_unit_colour { new_secondary_unit_colour },
	tertiary_unit_colour { new_tertiary_unit_colour } {}

bool CountryDefinitionManager::add_country(
	std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
	IdentifierRegistry<CountryParty>&& parties, CountryDefinition::unit_names_map_t&& unit_names, bool dynamic_tag,
	CountryDefinition::government_colour_map_t&& alternative_colours
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid country identifier - empty!");
		return false;
	}
	if (!valid_basic_identifier(identifier)) {
		spdlog::error_s(
			"Invalid country identifier: {} (can only contain alphanumeric characters and underscores)", identifier
		);
		return false;
	}
	if (graphical_culture == nullptr) {
		spdlog::error_s("Null graphical culture for country {}", identifier);
		return false;
	}

	static constexpr colour_t default_colour = colour_t::fill_as(colour_t::max_value);

	return country_definitions.emplace_item(
		identifier,
		identifier, colour, get_country_definition_count(), *graphical_culture, std::move(parties), std::move(unit_names),
		dynamic_tag, std::move(alternative_colours),
		/* Default to country colour for the chest and grey for the others. Update later if necessary. */
		colour, default_colour, default_colour
	);
}

template struct fmt::formatter<OpenVic::CountryDefinition>;
