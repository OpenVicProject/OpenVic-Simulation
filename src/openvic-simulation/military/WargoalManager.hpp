#pragma once

#include <functional>

#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/military/Wargoal.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace ovdl::v2script {
	class Parser;
}

namespace OpenVic {
	struct DefinitionManager;

	struct WargoalTypeManager {
	private:
		IdentifierRegistry<WargoalType> IDENTIFIER_REGISTRY(wargoal_type);
		memory::vector<std::reference_wrapper<const WargoalType>> SPAN_PROPERTY(peace_priorities);

	public:
		bool add_wargoal_type(
			std::string_view identifier, std::string_view war_name, Timespan available_length,
			Timespan truce_length, WargoalType::sprite_t sprite_index, bool triggered_only, bool civil_war,
			bool constructing, bool crisis, bool great_war_obligatory, bool mutual, bool all_allowed_states,
			bool always, WargoalType::peace_modifiers_t&& modifiers, WargoalType::peace_options_t peace_options,
			ConditionScript&& can_use, ConditionScript&& is_valid, ConditionScript&& allowed_states,
			ConditionScript&& allowed_substate_regions, ConditionScript&& allowed_states_in_crisis,
			ConditionScript&& allowed_countries, EffectScript&& on_add, EffectScript&& on_po_accepted
		);

		bool load_wargoal_file(ovdl::v2script::Parser const& parser);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}
