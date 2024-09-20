#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct NationalValueManager;

	struct NationalValue : Modifier {
		friend struct NationalValueManager;

	private:
		NationalValue(std::string_view new_identifier, ModifierValue&& new_modifiers);

	public:
		NationalValue(NationalValue&&) = default;
	};

	struct NationalValueManager {
	private:
		IdentifierRegistry<NationalValue> IDENTIFIER_REGISTRY(national_value);

	public:
		bool add_national_value(std::string_view identifier, ModifierValue&& modifiers);

		bool load_national_values_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
}
