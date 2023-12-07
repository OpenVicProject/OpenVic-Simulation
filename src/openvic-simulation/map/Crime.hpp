#pragma once

#include "openvic-simulation/misc/Modifier.hpp"

namespace OpenVic {
	struct Crime final : TriggeredModifier {
		friend struct CrimeManager;

	private:
		const bool PROPERTY(default_active);

		Crime(std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon, bool new_default_active);

	public:
		Crime(Crime&&) = default;
	};

	struct CrimeManager {
	private:
		IdentifierRegistry<Crime> crime_modifiers;

	public:
		CrimeManager();

		bool add_crime_modifier(
			std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon, bool default_active
		);
		IDENTIFIER_REGISTRY_ACCESSORS(crime_modifier)

		bool load_crime_modifiers(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
}
