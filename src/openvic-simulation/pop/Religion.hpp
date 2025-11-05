#pragma once

#include <cstdint>
#include <string_view>

#include "openvic-simulation/core/container/HasIdentifier.hpp"
#include "openvic-simulation/core/container/HasIdentifierAndColour.hpp"
#include "openvic-simulation/core/container/IdentifierRegistry.hpp"
#include "openvic-simulation/core/object/Colour.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	struct ReligionGroup : HasIdentifier {
	public:
		ReligionGroup(std::string_view new_identifier);
		ReligionGroup(ReligionGroup&&) = default;
	};

	struct Religion : HasIdentifierAndColour {
		using icon_t = uint8_t;

	private:
		ReligionGroup const& PROPERTY(group);
		const icon_t PROPERTY(icon);
		const bool PROPERTY(pagan);

	public:
		Religion(
			std::string_view new_identifier, colour_t new_colour, ReligionGroup const& new_group, icon_t new_icon,
			bool new_pagan
		);
		Religion(Religion&&) = default;
	};

	struct ReligionManager {
	private:
		IdentifierRegistry<ReligionGroup> IDENTIFIER_REGISTRY(religion_group);
		IdentifierRegistry<Religion> IDENTIFIER_REGISTRY(religion);

	public:
		bool add_religion_group(std::string_view identifier);

		bool add_religion(
			std::string_view identifier, colour_t colour, ReligionGroup const& group, Religion::icon_t icon, bool pagan
		);

		bool load_religion_file(ast::NodeCPtr root);
	};
}
