#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct NationalValueManager;

	struct NationalValue : Modifier {
	public:
		NationalValue(std::string_view new_identifier, ModifierValue&& new_modifiers);
		NationalValue(NationalValue&&) = default;
	};

	struct NationalValueManager {
	private:
		IdentifierRegistry<NationalValue> IDENTIFIER_REGISTRY(national_value);

	public:
		bool add_national_value(std::string_view identifier, ModifierValue&& modifiers);

		bool load_national_values_file(ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root);
	};
}
