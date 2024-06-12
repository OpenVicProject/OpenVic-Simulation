#pragma once

#include "openvic-simulation/misc/Modifier.hpp"

namespace OpenVic {
	struct Crime final : TriggeredModifier {
		friend struct CrimeManager;

	private:
		const bool PROPERTY(default_active);

		Crime(
			std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon, ConditionScript&& new_trigger,
			bool new_default_active
		);

	public:
		Crime(Crime&&) = default;
	};

	struct CrimeManager {
	private:
		IdentifierRegistry<Crime> IDENTIFIER_REGISTRY(crime_modifier);

	public:
		bool add_crime_modifier(
			std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon, ConditionScript&& trigger,
			bool default_active
		);

		bool load_crime_modifiers(ModifierManager const& modifier_manager, ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
