#pragma once

#include "openvic-simulation/dataloader/Node_forwarded.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct ConditionScript;

	struct CrimeManager {
	private:
		IdentifierRegistry<Crime> IDENTIFIER_REGISTRY(crime_modifier);

	public:
		bool add_crime_modifier(
			std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon, ConditionScript&& trigger,
			bool default_active
		);

		bool load_crime_modifiers(ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
