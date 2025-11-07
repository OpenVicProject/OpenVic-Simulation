#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/core/container/HasIndex.hpp"

namespace OpenVic {
	struct CrimeManager;

	struct Crime final : HasIndex<Crime>, TriggeredModifier {
		friend struct CrimeManager;

	private:
		const bool PROPERTY_CUSTOM_PREFIX(default_active, is);

	public:
		Crime(
			index_t new_index,
			std::string_view new_identifier,
			ModifierValue&& new_values,
			icon_t new_icon,
			ConditionScript&& new_trigger,
			bool new_default_active
		);
		Crime(Crime&&) = default;
	};

	struct CrimeManager {
	private:
		IdentifierRegistry<Crime> IDENTIFIER_REGISTRY(crime_modifier);

	public:
		bool add_crime_modifier(
			std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon, ConditionScript&& trigger,
			bool default_active
		);

		bool load_crime_modifiers(ModifierManager const& modifier_manager, ast::NodeCPtr root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
