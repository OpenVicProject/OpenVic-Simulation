#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ReligionGroup : HasIdentifier {
	public:
		ReligionGroup(std::string_view new_identifier);
		ReligionGroup(ReligionGroup&&) = default;
	};

	struct Religion : HasIdentifierAndColour {
		using icon_t = uint8_t;

	public:
		ReligionGroup const& group;
		const icon_t icon;
		const bool pagan;

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
