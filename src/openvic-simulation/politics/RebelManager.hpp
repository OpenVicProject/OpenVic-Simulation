#pragma once

#include <cstdint>

#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct GovernmentTypeManager;
	struct IdeologyManager;
	struct ModifierManager;

	struct RebelManager {
	private:
		IdentifierRegistry<RebelType> IDENTIFIER_REGISTRY(rebel_type);

	public:
		bool add_rebel_type(
			std::string_view new_identifier, RebelType::icon_t icon, RebelType::area_t area, bool break_alliance_on_win,
			RebelType::government_map_t&& desired_governments, RebelType::defection_t defection,
			RebelType::independence_t independence, uint16_t defect_delay, Ideology const* ideology, bool allow_all_cultures,
			bool allow_all_culture_groups, bool allow_all_religions, bool allow_all_ideologies, bool resilient,
			bool reinforcing, bool general, bool smart, bool unit_transfer, fixed_point_t occupation_mult,
			ConditionalWeightFactorMul&& will_rise, ConditionalWeightFactorMul&& spawn_chance,
			ConditionalWeightFactorMul&& movement_evaluation, ConditionScript&& siege_won_trigger,
			EffectScript&& siege_won_effect, ConditionScript&& demands_enforced_trigger, EffectScript&& demands_enforced_effect
		);

		bool load_rebels_file(
			IdeologyManager const& ideology_manager, GovernmentTypeManager const& government_type_manager, ovdl::v2script::ast::Node const* root
		);
		bool generate_modifiers(ModifierManager& modifier_manager) const;

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}