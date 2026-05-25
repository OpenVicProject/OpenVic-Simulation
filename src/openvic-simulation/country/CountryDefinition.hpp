#pragma once

#include <string_view>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/country/CountryParty.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct CountryDefinitionManager;
	struct GraphicalCultureType;
	struct GovernmentType;
	struct UnitType;

	/* Generic information about a TAG */
	struct CountryDefinition : HasIdentifierAndColour, HasIndex<CountryDefinition, country_index_t> {
		friend struct CountryDefinitionManager;
		
		using unit_names_map_t = ordered_map<UnitType const*, memory::vector<memory::string>>;
		using government_colour_map_t = ordered_map<GovernmentType const*, colour_t>;

	private:
		/* Not const to allow elements to be moved, otherwise a copy is forced
		 * which causes a compile error as the copy constructor has been deleted. */
		IdentifierRegistry<CountryParty> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(party, parties);
		unit_names_map_t PROPERTY(unit_names);
		government_colour_map_t PROPERTY(alternative_colours);
		colour_t PROPERTY(primary_unit_colour);
		colour_t PROPERTY(secondary_unit_colour);
		colour_t PROPERTY(tertiary_unit_colour);
		// Unit colours not const due to being added after construction


	public:
		const bool is_dynamic_tag;
		GraphicalCultureType const& graphical_culture;

		OV_ALWAYS_INLINE bool is_rebel_country() const {
			return index == country_index_t(0);
		}

		CountryDefinition(
			std::string_view new_identifier, colour_t new_colour, index_t new_index,
			GraphicalCultureType const& new_graphical_culture, IdentifierRegistry<CountryParty>&& new_parties,
			unit_names_map_t&& new_unit_names, bool new_is_dynamic_tag, government_colour_map_t&& new_alternative_colours,
			colour_t new_primary_unit_colour, colour_t new_secondary_unit_colour, colour_t new_tertiary_unit_colour
		);
		CountryDefinition(CountryDefinition&&) = default;

		// TODO - get_colour including alternative colours
	};
}

extern template struct fmt::formatter<OpenVic::CountryDefinition>;
