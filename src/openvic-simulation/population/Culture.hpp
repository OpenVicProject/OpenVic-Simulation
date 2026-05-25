#pragma once

#include <optional>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct GraphicalCultureType : HasIdentifier {
		friend struct CultureManager;

	public:
		GraphicalCultureType(std::string_view new_identifier);
		GraphicalCultureType(GraphicalCultureType&&) = default;
	};

	struct CultureGroup : HasIdentifier {
		friend struct CultureManager;

	private:
		memory::string PROPERTY(leader);

	public:
		GraphicalCultureType const& unit_graphical_culture_type;
		const bool is_overseas;
		const std::optional<country_index_t> union_country;

		CultureGroup(
			std::string_view new_identifier, std::string_view new_leader,
			GraphicalCultureType const& new_unit_graphical_culture_type, bool new_is_overseas,
			std::optional<country_index_t> new_union_country
		);
		CultureGroup(CultureGroup&&) = default;
	};

	struct Culture : HasIdentifierAndColour {
	private:
		memory::vector<memory::string> PROPERTY(first_names);
		memory::vector<memory::string> PROPERTY(last_names);

	public:
		CultureGroup const& group;
		const fixed_point_t radicalism;
		const std::optional<country_index_t> primary_country;

		Culture(
			std::string_view new_identifier, colour_t new_colour, CultureGroup const& new_group, memory::vector<memory::string>&& new_first_names,
			memory::vector<memory::string>&& new_last_names, fixed_point_t new_radicalism, std::optional<country_index_t> new_primary_country
		);
		Culture(Culture&&) = default;

		constexpr bool has_union_country() const {
			return group.union_country.has_value();
		}
	};
}
