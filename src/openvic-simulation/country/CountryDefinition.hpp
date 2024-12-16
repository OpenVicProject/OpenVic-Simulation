#pragma once

#include <string_view>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/politics/Government.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/politics/Issue.hpp"
#include "openvic-simulation/politics/PoliticsManager.hpp"
#include "openvic-simulation/pop/Culture.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct DefinitionManager;
	struct CountryDefinitionManager;

	struct CountryParty : HasIdentifierAndColour {
		friend struct CountryDefinitionManager;

		using policy_map_t = IndexedMap<IssueGroup, Issue const*>;

	private:
		const Date PROPERTY(start_date);
		const Date PROPERTY(end_date);
		Ideology const* PROPERTY(ideology); // Can be nullptr, shows up as "No Ideology" in game
		policy_map_t PROPERTY(policies);

		CountryParty(
			std::string_view new_identifier, Date new_start_date, Date new_end_date, Ideology const* new_ideology,
			policy_map_t&& new_policies
		);

	public:
		CountryParty(CountryParty&&) = default;
	};

	/* Generic information about a TAG */
	struct CountryDefinition : HasIdentifierAndColour, HasIndex<> {
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
		colour_t PROPERTY(primary_unit_colour);
		colour_t PROPERTY(secondary_unit_colour);
		colour_t PROPERTY(tertiary_unit_colour);
		// Unit colours not const due to being added after construction

		CountryDefinition(
			std::string_view new_identifier, colour_t new_colour, index_t new_index,
			GraphicalCultureType const& new_graphical_culture, IdentifierRegistry<CountryParty>&& new_parties,
			unit_names_map_t&& new_unit_names, bool new_dynamic_tag, government_colour_map_t&& new_alternative_colours,
			colour_t new_primary_unit_colour, colour_t new_secondary_unit_colour, colour_t new_tertiary_unit_colour
		);

	public:
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
