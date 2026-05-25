#pragma once

#include "openvic-simulation/dataloader/Node_forwarded.hpp"
#include "openvic-simulation/population/Religion.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ReligionManager {
	private:
		IdentifierRegistry<ReligionGroup> IDENTIFIER_REGISTRY(religion_group);
		IdentifierRegistry<Religion> IDENTIFIER_REGISTRY(religion);

	public:
		bool add_religion_group(std::string_view identifier);

		bool add_religion(
			std::string_view identifier, colour_t colour, ReligionGroup const& group, Religion::icon_t icon, bool is_pagan
		);

		bool load_religion_file(ovdl::v2script::ast::Node const* root);
	};
}
