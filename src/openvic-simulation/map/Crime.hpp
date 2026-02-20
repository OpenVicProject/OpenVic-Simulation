#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct CrimeManager;

	struct Crime final : HasIndex<Crime, crime_index_t>, TriggeredModifier {
		friend struct CrimeManager;
	public:
		bool is_default_active;

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
