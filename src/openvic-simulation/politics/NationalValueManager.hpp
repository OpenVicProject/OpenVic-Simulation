#pragma once

#include "openvic-simulation/politics/NationalValue.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct NationalValueManager {
	private:
		IdentifierRegistry<NationalValue> IDENTIFIER_REGISTRY(national_value);

	public:
		bool add_national_value(std::string_view identifier, ModifierValue&& modifiers);

		bool load_national_values_file(ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root);
	};
}
