#pragma once

#include <cstdint>

#include "openvic-simulation/core/container/HasIdentifier.hpp"
#include "openvic-simulation/core/container/HasIndex.hpp"
#include "openvic-simulation/core/container/IdentifierRegistry.hpp"
#include "openvic-simulation/core/memory/OrderedMap.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct GovernmentType;
	struct GovernmentTypeManager;
	struct Ideology;
	struct IdeologyManager;
	struct ModifierManager;
	struct RebelManager;

	struct RebelType : HasIndex<RebelType, rebel_type_index_t>, HasIdentifier {
		friend struct RebelManager;

		using government_map_t = memory::ordered_map<GovernmentType const*, GovernmentType const*>;
		using icon_t = uint16_t;

		enum class area_t {
			NATION, NATION_RELIGION, NATION_CULTURE,
			CULTURE, CULTURE_GROUP, RELIGION, ALL
		};

		enum class defection_t {
			NONE, CULTURE, CULTURE_GROUP, RELIGION,
			IDEOLOGY, PAN_NATIONALIST, ANY
		};

		enum class independence_t {
			NONE, CULTURE, CULTURE_GROUP, RELIGION,
			COLONIAL, PAN_NATIONALIST, ANY
		};

	private:
		const bool PROPERTY_CUSTOM_PREFIX(break_alliance_on_win, will);
		government_map_t PROPERTY(desired_governments); //government
		const defection_t PROPERTY_CUSTOM_NAME(defection, get_defection_type);
		const independence_t PROPERTY_CUSTOM_NAME(independence, get_independence_type);
		Ideology const* PROPERTY(ideology);
		const bool PROPERTY_CUSTOM_PREFIX(allow_all_cultures, will);
		const bool PROPERTY_CUSTOM_PREFIX(allow_all_culture_groups, will);
		const bool PROPERTY_CUSTOM_PREFIX(allow_all_religions, will);
		const bool PROPERTY_CUSTOM_PREFIX(allow_all_ideologies, will);
		const bool PROPERTY_CUSTOM_PREFIX(resilient, is);
		const bool PROPERTY_CUSTOM_PREFIX(reinforcing, is);
		const bool PROPERTY_CUSTOM_PREFIX(general, can_have);
		const bool PROPERTY_CUSTOM_PREFIX(smart, is);
		const bool PROPERTY_CUSTOM_NAME(unit_transfer, will_transfer_units);
		ConditionalWeightFactorMul PROPERTY(will_rise);
		ConditionalWeightFactorMul PROPERTY(spawn_chance);
		ConditionalWeightFactorMul PROPERTY(movement_evaluation);
		ConditionScript PROPERTY(siege_won_trigger);
		EffectScript PROPERTY(siege_won_effect);
		ConditionScript PROPERTY(demands_enforced_trigger);
		EffectScript PROPERTY(demands_enforced_effect);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const icon_t icon;
		const area_t area;
		const uint16_t defect_delay;
		const fixed_point_t occupation_mult;

		RebelType(
			index_t new_index, std::string_view new_identifier,
			RebelType::icon_t icon, RebelType::area_t area, bool break_alliance_on_win,
			RebelType::government_map_t&& desired_governments, RebelType::defection_t defection,
			RebelType::independence_t independence, uint16_t defect_delay, Ideology const* ideology, bool allow_all_cultures,
			bool allow_all_culture_groups, bool allow_all_religions, bool allow_all_ideologies, bool resilient,
			bool reinforcing, bool general, bool smart, bool unit_transfer, fixed_point_t occupation_mult,
			ConditionalWeightFactorMul&& new_will_rise, ConditionalWeightFactorMul&& new_spawn_chance,
			ConditionalWeightFactorMul&& new_movement_evaluation, ConditionScript&& new_siege_won_trigger,
			EffectScript&& new_siege_won_effect, ConditionScript&& new_demands_enforced_trigger,
			EffectScript&& new_demands_enforced_effect
		);
		RebelType(RebelType&&) = default;
	};

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
			IdeologyManager const& ideology_manager, GovernmentTypeManager const& government_type_manager, ast::NodeCPtr root
		);
		bool generate_modifiers(ModifierManager& modifier_manager) const;

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}