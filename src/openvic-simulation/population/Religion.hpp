#pragma once

#include <cstdint>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"

namespace OpenVic {
	struct ReligionGroup : HasIdentifier {
	public:
		ReligionGroup(std::string_view new_identifier);
		ReligionGroup(ReligionGroup&&) = default;
	};

	struct Religion : HasIdentifierAndColour {
		using icon_t = std::uint8_t;

	public:
		ReligionGroup const& group;
		const icon_t icon;
		const bool is_pagan;

		Religion(
			std::string_view new_identifier,
			colour_t new_colour,
			ReligionGroup const& new_group,
			icon_t new_icon,
			bool new_is_pagan
		);
		Religion(Religion&&) = default;
	};
}
