#include "Wargoal.hpp"

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

WargoalType::WargoalType(
	std::string_view new_identifier, std::string_view new_war_name, Timespan new_available_length,
	Timespan new_truce_length, sprite_t new_sprite_index, bool new_triggered_only, bool new_civil_war,
	bool new_constructing, bool new_crisis, bool new_great_war_obligatory, bool new_mutual,
	bool new_all_allowed_states, bool new_always, peace_modifiers_t&& new_modifiers, peace_options_t new_peace_options,
	ConditionScript&& new_can_use, ConditionScript&& new_is_valid, ConditionScript&& new_allowed_states,
	ConditionScript&& new_allowed_substate_regions, ConditionScript&& new_allowed_states_in_crisis,
	ConditionScript&& new_allowed_countries, EffectScript&& new_on_add, EffectScript&& new_on_po_accepted
) : HasIdentifier { new_identifier }, war_name { new_war_name }, available_length { new_available_length },
	truce_length { new_truce_length }, sprite_index { new_sprite_index }, triggered_only { new_triggered_only },
	civil_war { new_civil_war }, constructing { new_constructing }, crisis { new_crisis },
	great_war_obligatory { new_great_war_obligatory }, mutual { new_mutual }, all_allowed_states { new_all_allowed_states },
	always { new_always }, modifiers { std::move(new_modifiers) }, peace_options { new_peace_options },
	can_use { std::move(new_can_use) }, is_valid { std::move(new_is_valid) }, allowed_states { std::move(new_allowed_states) },
	allowed_substate_regions { std::move(new_allowed_substate_regions) },
	allowed_states_in_crisis { std::move(new_allowed_states_in_crisis) },
	allowed_countries { std::move(new_allowed_countries) }, on_add { std::move(new_on_add) },
	on_po_accepted { std::move(new_on_po_accepted) } {}

bool WargoalType::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	ret &= can_use.parse_script(true, definition_manager);
	ret &= is_valid.parse_script(true, definition_manager);
	ret &= allowed_states.parse_script(true, definition_manager);
	ret &= allowed_substate_regions.parse_script(true, definition_manager);
	ret &= allowed_states_in_crisis.parse_script(true, definition_manager);
	ret &= allowed_countries.parse_script(true, definition_manager);
	ret &= on_add.parse_script(true, definition_manager);
	ret &= on_po_accepted.parse_script(true, definition_manager);
	return ret;
}

bool WargoalTypeManager::add_wargoal_type(
	std::string_view identifier, std::string_view war_name, Timespan available_length,
	Timespan truce_length, WargoalType::sprite_t sprite_index, bool triggered_only, bool civil_war,
	bool constructing, bool crisis, bool great_war_obligatory, bool mutual, bool all_allowed_states,
	bool always, WargoalType::peace_modifiers_t&& modifiers, WargoalType::peace_options_t peace_options,
	ConditionScript&& can_use, ConditionScript&& is_valid, ConditionScript&& allowed_states,
	ConditionScript&& allowed_substate_regions, ConditionScript&& allowed_states_in_crisis,
	ConditionScript&& allowed_countries, EffectScript&& on_add, EffectScript&& on_po_accepted
) {
	if (identifier.empty()) {
		Logger::error("Invalid wargoal identifier - empty!");
		return false;
	}

	if (war_name.empty()) {
		Logger::error("Invalid war name for wargoal ", identifier, " - empty!");
		return false;
	}

	if (sprite_index == 0) {
		Logger::warning("Invalid sprite for wargoal ", identifier, " - 0");
	}

	return wargoal_types.add_item({
		identifier, war_name, available_length, truce_length, sprite_index, triggered_only, civil_war, constructing, crisis,
		great_war_obligatory, mutual, all_allowed_states, always, std::move(modifiers), peace_options, std::move(can_use),
		std::move(is_valid), std::move(allowed_states), std::move(allowed_substate_regions),
		std::move(allowed_states_in_crisis), std::move(allowed_countries), std::move(on_add), std::move(on_po_accepted)
	});
}

bool WargoalTypeManager::load_wargoal_file(ovdl::v2script::Parser const& parser) {
	using namespace std::string_view_literals;
	ovdl::symbol<char> peace_order_symbol = parser.find_intern("peace_order"sv);
	if (!peace_order_symbol) {
		Logger::error("peace_order could not be interned.");
	}

	bool ret = expect_dictionary_reserve_length(
		wargoal_types,
		[this, &peace_order_symbol](std::string_view identifier, ast::NodeCPtr value) -> bool {
			// If peace_order_symbol is false, we know there is no peace_order string in the parser
			if (peace_order_symbol && identifier == peace_order_symbol.c_str()) {
				return true;
			}

			using enum WargoalType::peace_options_t;
			using enum scope_type_t;

			std::string_view war_name;
			Timespan available {}, truce {};
			WargoalType::sprite_t sprite_index = 0;
			bool triggered_only = false, civil_war = false, constructing = true, crisis = true, great_war_obligatory = false,
				mutual = false, all_allowed_states = false, always = false;
			WargoalType::peace_options_t peace_options = NO_PEACE_OPTIONS;
			WargoalType::peace_modifiers_t modifiers;
			ConditionScript can_use { COUNTRY, COUNTRY, COUNTRY };
			ConditionScript is_valid { COUNTRY, COUNTRY, COUNTRY };
			ConditionScript allowed_states { PROVINCE, COUNTRY, COUNTRY };
			ConditionScript allowed_substate_regions { PROVINCE, COUNTRY, COUNTRY };
			ConditionScript allowed_states_in_crisis { PROVINCE, COUNTRY, COUNTRY };
			ConditionScript allowed_countries { COUNTRY, COUNTRY, COUNTRY };
			EffectScript on_add, on_po_accepted; //country as default scope for both

			const auto expect_peace_option = [&peace_options](WargoalType::peace_options_t peace_option) -> node_callback_t {
				return expect_bool([&peace_options, peace_option](bool val) -> bool {
					if (val) {
						peace_options |= peace_option;
					} else {
						peace_options &= ~peace_option;
					}
					return true;
				});
			};

			bool ret = expect_dictionary_keys_and_default(
				[&modifiers, &identifier](std::string_view key, ast::NodeCPtr value) -> bool {
					using enum WargoalType::PEACE_MODIFIERS;
					static const string_map_t<WargoalType::PEACE_MODIFIERS> peace_modifier_map {
						{ "badboy_factor", BADBOY_FACTOR },
						{ "prestige_factor", PRESTIGE_FACTOR },
						{ "peace_cost_factor", PEACE_COST_FACTOR },
						{ "penalty_factor", PENALTY_FACTOR },
						{ "break_truce_prestige_factor", BREAK_TRUCE_PRESTIGE_FACTOR },
						{ "break_truce_infamy_factor", BREAK_TRUCE_INFAMY_FACTOR },
						{ "break_truce_militancy_factor", BREAK_TRUCE_MILITANCY_FACTOR },
						{ "good_relation_prestige_factor", GOOD_RELATION_PRESTIGE_FACTOR },
						{ "good_relation_infamy_factor", GOOD_RELATION_INFAMY_FACTOR },
						{ "good_relation_militancy_factor", GOOD_RELATION_MILITANCY_FACTOR },
						{ "tws_battle_factor", WAR_SCORE_BATTLE_FACTOR },
						{ "construction_speed", CONSTRUCTION_SPEED }
					};
					const decltype(peace_modifier_map)::const_iterator it = peace_modifier_map.find(key);
					if (it != peace_modifier_map.end()) {
						return expect_fixed_point(
							[&modifiers, &identifier, &key, peace_modifier = it->second](fixed_point_t val) -> bool {
								if (modifiers.emplace(peace_modifier, val).second) {
									return true;
								}
								Logger::error("Duplicate peace modifier ", key, " in wargoal ", identifier, "!");
								return false;
							}
						)(value);
					}

					Logger::error("Modifier ", key, " in wargoal ", identifier, " is invalid.");
					return false;
				},
				"war_name", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(war_name)),
				"months", ZERO_OR_ONE, expect_months(assign_variable_callback(available)),
				"truce_months", ONE_EXACTLY, expect_months(assign_variable_callback(truce)),
				"sprite_index", ONE_EXACTLY, expect_uint(assign_variable_callback(sprite_index)),
				"is_triggered_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(triggered_only)),
				"is_civil_war", ZERO_OR_ONE, expect_bool(assign_variable_callback(civil_war)),
				"constructing_cb", ZERO_OR_ONE, expect_bool(assign_variable_callback(constructing)),
				"crisis", ZERO_OR_ONE, expect_bool(assign_variable_callback(crisis)),
				"great_war_obligatory", ZERO_OR_ONE, expect_bool(assign_variable_callback(great_war_obligatory)),
				"mutual", ZERO_OR_ONE, expect_bool(assign_variable_callback(mutual)),
				/* START PEACE OPTIONS */
				"po_annex", ZERO_OR_ONE, expect_peace_option(PO_ANNEX),
				"po_demand_state", ZERO_OR_ONE, expect_peace_option(PO_DEMAND_STATE),
				"po_add_to_sphere", ZERO_OR_ONE, expect_peace_option(PO_ADD_TO_SPHERE),
				"po_disarmament", ZERO_OR_ONE, expect_peace_option(PO_DISARMAMENT),
				"po_destroy_forts", ZERO_OR_ONE, expect_peace_option(PO_REMOVE_FORTS),
				"po_destroy_naval_bases", ZERO_OR_ONE, expect_peace_option(PO_REMOVE_NAVAL_BASES),
				"po_reparations", ZERO_OR_ONE, expect_peace_option(PO_REPARATIONS),
				"po_transfer_provinces", ZERO_OR_ONE, expect_peace_option(PO_TRANSFER_PROVINCES),
				"po_remove_prestige", ZERO_OR_ONE, expect_peace_option(PO_REMOVE_PRESTIGE),
				"po_make_puppet", ZERO_OR_ONE, expect_peace_option(PO_MAKE_PUPPET),
				"po_release_puppet", ZERO_OR_ONE, expect_peace_option(PO_RELEASE_PUPPET),
				"po_status_quo", ZERO_OR_ONE, expect_peace_option(PO_STATUS_QUO),
				"po_install_communist_gov_type", ZERO_OR_ONE, expect_peace_option(PO_INSTALL_COMMUNISM),
				"po_uninstall_communist_gov_type", ZERO_OR_ONE, expect_peace_option(PO_REMOVE_COMMUNISM),
				"po_remove_cores", ZERO_OR_ONE, expect_peace_option(PO_REMOVE_CORES),
				"po_colony", ZERO_OR_ONE, expect_peace_option(PO_COLONY),
				"po_gunboat", ZERO_OR_ONE, expect_peace_option(PO_REPAY_DEBT),
				"po_clear_union_sphere", ZERO_OR_ONE, expect_peace_option(PO_CLEAR_UNION_SPHERE),
				/* END PEACE OPTIONS */
				"can_use", ZERO_OR_ONE, can_use.expect_script(),
				"is_valid", ZERO_OR_ONE, is_valid.expect_script(),
				"on_add", ZERO_OR_ONE, on_add.expect_script(),
				"on_po_accepted", ZERO_OR_ONE, on_po_accepted.expect_script(),
				"allowed_states", ZERO_OR_ONE, allowed_states.expect_script(),
				"all_allowed_states", ZERO_OR_ONE, expect_bool(assign_variable_callback(all_allowed_states)),
				"allowed_substate_regions", ZERO_OR_ONE, allowed_substate_regions.expect_script(),
				"allowed_states_in_crisis", ZERO_OR_ONE, allowed_states_in_crisis.expect_script(),
				"allowed_countries", ZERO_OR_ONE, allowed_countries.expect_script(),
				"always", ZERO_OR_ONE, expect_bool(assign_variable_callback(always))
			)(value);

			add_wargoal_type(
				identifier, war_name, available, truce, sprite_index, triggered_only, civil_war, constructing, crisis,
				great_war_obligatory, mutual, all_allowed_states, always, std::move(modifiers), peace_options,
				std::move(can_use), std::move(is_valid), std::move(allowed_states), std::move(allowed_substate_regions),
				std::move(allowed_states_in_crisis), std::move(allowed_countries), std::move(on_add), std::move(on_po_accepted)
			);
			return ret;
		}
	)(parser.get_file_node());

	/* load order in which CBs are prioritised by AI */
	ret &= expect_key(
		peace_order_symbol,
		expect_list(
			expect_wargoal_type_identifier(
				[this](WargoalType const& wargoal) -> bool {
					if (std::find(peace_priorities.begin(), peace_priorities.end(), &wargoal) == peace_priorities.end()) {
						peace_priorities.push_back(&wargoal);
					} else {
						Logger::warning("Wargoal ", wargoal.get_identifier(), " is already in the peace priority list!");
					}
					return true;
				},
				true // warn instead of error
			)
		)
	)(parser.get_file_node());

	lock_wargoal_types();
	return ret;
}

bool WargoalTypeManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	for (WargoalType& wargoal_type : wargoal_types.get_items()) {
		ret &= wargoal_type.parse_scripts(definition_manager);
	}
	return ret;
}
