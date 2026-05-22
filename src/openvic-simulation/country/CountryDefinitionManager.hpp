#pragma once

#include <string_view>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryParty.hpp"
#include "openvic-simulation/dataloader/NodeCallbacks.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	class Dataloader;
	struct DefinitionManager;
	struct PoliticsManager;

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

		bool load_country_colours(ovdl::v2script::ast::Node const* root);

		bool load_countries(DefinitionManager const& definition_manager, Dataloader const& dataloader, ovdl::v2script::ast::Node const* root);
		bool load_country_data_file(
			DefinitionManager const& definition_manager, std::string_view name, bool is_dynamic, ovdl::v2script::ast::Node const* root
		);
	};
}
