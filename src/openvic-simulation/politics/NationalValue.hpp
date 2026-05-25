#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"

namespace OpenVic {
	struct NationalValueManager;

	struct NationalValue : Modifier {
	public:
		NationalValue(std::string_view new_identifier, ModifierValue&& new_modifiers);
		NationalValue(NationalValue&&) = default;
	};
}
