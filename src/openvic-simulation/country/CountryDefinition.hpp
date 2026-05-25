#pragma once

#include <cstddef>
#include <string_view>
#include <type_traits>

#include "openvic-simulation/core/memory/FixedVector.hpp"
#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/country/CountryParty.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct CountryDefinitionManager;
	struct GraphicalCultureType;
	struct GovernmentType;

	/* Generic information about a TAG */
	struct CountryDefinition : HasIdentifierAndColour, HasIndex<CountryDefinition, country_index_t> {
		friend struct CountryDefinitionManager;
		IdentifierRegistry<CountryParty> IDENTIFIER_REGISTRY_CUSTOM_PLURAL(party, parties);

	private:
		memory::FixedVector< // for each ship type
			memory::FixedVector< // collection of names
				memory::string
			>,
			ship_type_index_t
		> PROPERTY(ship_names);
		ordered_map<GovernmentType const*, colour_t> PROPERTY(alternative_colours);
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
			GraphicalCultureType const& new_graphical_culture,
			std::remove_const_t<decltype(parties)>&& new_parties,
			decltype(ship_names)&& new_ship_names,
			bool new_is_dynamic_tag,
			decltype(alternative_colours)&& new_alternative_colours,
			colour_t new_primary_unit_colour, colour_t new_secondary_unit_colour, colour_t new_tertiary_unit_colour
		);

		// TODO - get_colour including alternative colours
	};
}

extern template struct fmt::formatter<OpenVic::CountryDefinition>;
