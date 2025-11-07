#pragma once

#include <string_view>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/core/container/HasIdentifierAndColour.hpp"
#include "openvic-simulation/core/container/HasIndex.hpp"
#include "openvic-simulation/core/container/IdentifierRegistry.hpp"
#include "openvic-simulation/core/memory/OrderedMap.hpp"
#include "openvic-simulation/country/CountryParty.hpp"

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
		
		using unit_names_map_t = memory::ordered_map<UnitType const*, name_list_t>;
		using government_colour_map_t = memory::ordered_map<GovernmentType const*, colour_t>;

	private:
		GraphicalCultureType const& PROPERTY(graphical_culture);
		/* Not const to allow elements to be moved, otherwise a copy is forced
		 * which causes a compile error as the copy constructor has been deleted. */
		IdentifierRegistry<CountryParty> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(party, parties);
		unit_names_map_t PROPERTY(unit_names);
		const bool PROPERTY_CUSTOM_PREFIX(dynamic_tag, is);
		government_colour_map_t PROPERTY(alternative_colours);
		colour_t PROPERTY(primary_unit_colour);
		colour_t PROPERTY(secondary_unit_colour);
		colour_t PROPERTY(tertiary_unit_colour);
		// Unit colours not const due to being added after construction


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

		NodeTools::node_callback_t load_country_party(
			PoliticsManager const& politics_manager, IdentifierRegistry<CountryParty>& country_parties
		) const;

	public:
		bool add_country(
			std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
			IdentifierRegistry<CountryParty>&& parties, CountryDefinition::unit_names_map_t&& unit_names, bool dynamic_tag,
			CountryDefinition::government_colour_map_t&& alternative_colours
		);

		bool load_country_colours(ast::NodeCPtr root);

		bool load_countries(DefinitionManager const& definition_manager, Dataloader const& dataloader, ast::NodeCPtr root);
		bool load_country_data_file(
			DefinitionManager const& definition_manager, std::string_view name, bool is_dynamic, ast::NodeCPtr root
		);
	};
}

extern template struct fmt::formatter<OpenVic::CountryDefinition>;
