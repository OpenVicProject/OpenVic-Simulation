#pragma once

#include <string_view>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/country/CountryParty.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct CountryDefinitionManager;
	class Dataloader;
	struct DefinitionManager;
	struct GraphicalCultureType;
	struct GovernmentType;
	struct PoliticsManager;
	struct UnitType;

	/* Generic information about a TAG */
	struct CountryDefinition : HasIdentifierAndColour, HasIndex<CountryDefinition> {
		friend struct CountryDefinitionManager;
		
		using unit_names_map_t = ordered_map<UnitType const*, name_list_t>;
		using government_colour_map_t = ordered_map<GovernmentType const*, colour_t>;

	private:
		GraphicalCultureType const& PROPERTY(graphical_culture);
		/* Not const to allow elements to be moved, otherwise a copy is forced
		 * which causes a compile error as the copy constructor has been deleted. */
		IdentifierRegistry<CountryParty> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(party, parties);
		unit_names_map_t PROPERTY(unit_names);
		const bool PROPERTY_CUSTOM_PREFIX(dynamic_tag, is);
		government_colour_map_t PROPERTY(alternative_colours);
		colour_t PROPERTY_RW(primary_unit_colour);
		colour_t PROPERTY_RW(secondary_unit_colour);
		colour_t PROPERTY_RW(tertiary_unit_colour);

	public:
		CountryDefinition(
			std::string_view new_identifier, colour_t new_colour, index_t new_index,
			GraphicalCultureType const& new_graphical_culture, IdentifierRegistry<CountryParty>&& new_parties,
			unit_names_map_t&& new_unit_names, bool new_dynamic_tag, government_colour_map_t&& new_alternative_colours,
			colour_t new_primary_unit_colour, colour_t new_secondary_unit_colour, colour_t new_tertiary_unit_colour
		);
		CountryDefinition(CountryDefinition&&) = default;

		// TODO - get_colour including alternative colours
	};

	struct CountryDefinitionManager {
	private:
		IdentifierRegistry<CountryDefinition> IDENTIFIER_REGISTRY(country_definition);

	public:
		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(country_definition);

		bool add_country(
			std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
			IdentifierRegistry<CountryParty>&& parties, CountryDefinition::unit_names_map_t&& unit_names, bool dynamic_tag,
			CountryDefinition::government_colour_map_t&& alternative_colours
		);
	};
}

extern template struct fmt::formatter<OpenVic::CountryDefinition>;
