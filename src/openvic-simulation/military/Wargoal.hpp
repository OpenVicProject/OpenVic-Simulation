#pragma once

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/scripts/ConditionScript.hpp"
#include "openvic-simulation/scripts/EffectScript.hpp"
#include "openvic-simulation/types/EnumBitfield.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct WargoalTypeManager;
	struct DefinitionManager;

	struct WargoalType : HasIdentifier {
		friend struct WargoalTypeManager;

		using sprite_t = uint8_t;

		enum class PEACE_MODIFIERS : uint8_t {
			BADBOY_FACTOR,
			PRESTIGE_FACTOR,
			PEACE_COST_FACTOR,
			PENALTY_FACTOR,
			BREAK_TRUCE_PRESTIGE_FACTOR,
			BREAK_TRUCE_INFAMY_FACTOR,
			BREAK_TRUCE_MILITANCY_FACTOR,
			GOOD_RELATION_PRESTIGE_FACTOR,
			GOOD_RELATION_INFAMY_FACTOR,
			GOOD_RELATION_MILITANCY_FACTOR,
			WAR_SCORE_BATTLE_FACTOR,
			CONSTRUCTION_SPEED
		};
		using peace_modifiers_t = fixed_point_map_t<PEACE_MODIFIERS>;

		enum class peace_options_t : uint32_t {
			NO_PEACE_OPTIONS      = 0,
			PO_ANNEX              = 1 << 0,
			PO_DEMAND_STATE       = 1 << 1,
			PO_COLONY             = 1 << 2,
			PO_ADD_TO_SPHERE      = 1 << 3,
			PO_DISARMAMENT        = 1 << 4,
			PO_REMOVE_FORTS       = 1 << 5,
			PO_REMOVE_NAVAL_BASES = 1 << 6,
			PO_REPARATIONS        = 1 << 7,
			PO_REPAY_DEBT         = 1 << 8,
			PO_REMOVE_PRESTIGE    = 1 << 9,
			PO_MAKE_PUPPET        = 1 << 10,
			PO_RELEASE_PUPPET     = 1 << 11,
			PO_STATUS_QUO         = 1 << 12,
			PO_INSTALL_COMMUNISM  = 1 << 13,
			PO_REMOVE_COMMUNISM   = 1 << 14,
			PO_REMOVE_CORES       = 1 << 15, // only usable with ANNEX, DEMAND_STATE, or TRANSFER_PROVINCES
			PO_TRANSFER_PROVINCES = 1 << 16,
			PO_CLEAR_UNION_SPHERE = 1 << 17
		};

	private:
		memory::string PROPERTY(war_name);
		const bool PROPERTY_CUSTOM_PREFIX(triggered_only, is); // only able to be added via effects or on_actions
		const bool PROPERTY_CUSTOM_PREFIX(civil_war, is);
		const bool PROPERTY_CUSTOM_PREFIX(great_war_obligatory, is); // automatically add to great war peace offers/demands
		const bool PROPERTY_CUSTOM_PREFIX(mutual, is); // attacked and defender share wargoal
		peace_modifiers_t PROPERTY(modifiers);
		peace_options_t PROPERTY(peace_options);
		ConditionScript PROPERTY(can_use);
		ConditionScript PROPERTY(is_valid);
		ConditionScript PROPERTY(allowed_states);
		ConditionScript PROPERTY(allowed_substate_regions);
		ConditionScript PROPERTY(allowed_states_in_crisis);
		ConditionScript PROPERTY(allowed_countries);
		EffectScript PROPERTY(on_add);
		EffectScript PROPERTY(on_po_accepted);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const Timespan available_length;
		const Timespan truce_length;
		const sprite_t sprite_index;
		const bool constructing; // can be added to existing wars or justified
		const bool crisis; // able to be added to crises
		const bool all_allowed_states; // take all valid states rather than just one
		const bool always; // available without justifying

		WargoalType(
			std::string_view new_identifier, std::string_view new_war_name, Timespan new_available_length,
			Timespan new_truce_length, sprite_t new_sprite_index, bool new_triggered_only, bool new_civil_war,
			bool new_constructing, bool new_crisis, bool new_great_war_obligatory, bool new_mutual,
			bool new_all_allowed_states, bool new_always, peace_modifiers_t&& new_modifiers, peace_options_t new_peace_options,
			ConditionScript&& new_can_use, ConditionScript&& new_is_valid, ConditionScript&& new_allowed_states,
			ConditionScript&& new_allowed_substate_regions, ConditionScript&& new_allowed_states_in_crisis,
			ConditionScript&& new_allowed_countries, EffectScript&& new_on_add, EffectScript&& new_on_po_accepted
		);
		WargoalType(WargoalType&&) = default;
	};

	template<> struct enable_bitfield<WargoalType::peace_options_t> : std::true_type{};

	struct WargoalTypeManager {
	private:
		IdentifierRegistry<WargoalType> IDENTIFIER_REGISTRY(wargoal_type);
		memory::vector<WargoalType const*> SPAN_PROPERTY(peace_priorities);

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
