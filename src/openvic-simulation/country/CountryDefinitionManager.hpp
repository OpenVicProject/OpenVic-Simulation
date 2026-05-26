#pragma once

#include <string_view>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/dataloader/Node_forwarded.hpp"
#include "openvic-simulation/types/ConstructorTags.hpp"
#include "openvic-simulation/types/registries/OwningRegistry.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	class Dataloader;
	struct DefinitionManager;
	struct PoliticsManager;

	struct CountryDefinitionManager {
	private:
		OwningRegistry<CountryDefinition, country_index_t> PROPERTY(country_definitions);

	public:
		constexpr CountryDefinitionManager() : country_definitions(create_empty) {}

		bool add_country(
			std::string_view identifier, colour_t colour, GraphicalCultureType const* graphical_culture,
			std::remove_const_t<decltype(CountryDefinition::parties)>&& parties,
			decltype(CountryDefinition::ship_names)&& ship_names,
			bool is_dynamic_tag,
			decltype(CountryDefinition::alternative_colours)&& alternative_colours
		);

		bool load_country_colours(ovdl::v2script::ast::Node const* root);

		bool load_countries(
			DefinitionManager const& definition_manager,
			Dataloader const& dataloader,
			ovdl::v2script::ast::Node const* root
		);
		bool load_country_data_file(
			DefinitionManager const& definition_manager,
			std::string_view name, bool is_dynamic_tag,
			ovdl::v2script::ast::Node const* root
		);
	};
}
