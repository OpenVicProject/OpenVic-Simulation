#include "Wargoal.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

WargoalType::WargoalType(
	std::string_view new_identifier,
	std::string_view new_sprite,
	std::string_view new_war_name,
	Timespan new_available_length,
	Timespan new_truce_length,
	bool new_triggered_only,
	bool new_civil_war,
	bool new_constructing,
	bool new_crisis,
	bool new_great_war,
	bool new_mutual,
	const peace_modifiers_t&& new_modifiers,
	peace_options_t new_peace_options
) : HasIdentifier { new_identifier },
	sprite { new_sprite },
	war_name { new_war_name },
	available_length { new_available_length },
	truce_length { new_truce_length },
	triggered_only { new_triggered_only },
	civil_war { new_civil_war },
	constructing { new_constructing },
	crisis { new_crisis },
	great_war { new_great_war },
	mutual { new_mutual },
	modifiers { std::move(new_modifiers) },
	peace_options { new_peace_options } {}

std::string_view WargoalType::get_sprite() const {
	return sprite;
}

std::string_view WargoalType::get_war_name() const {
	return war_name;
}

const Timespan WargoalType::get_available_length() const {
	return available_length;
}

const Timespan WargoalType::get_truce_length() const {
	return truce_length;
}

const bool WargoalType::is_triggered_only() const {
	return triggered_only;
}

const bool WargoalType::is_civil_war() const {
	return civil_war;
}

const bool WargoalType::is_constructing() const {
	return constructing;
}

const bool WargoalType::is_crisis() const {
	return crisis;
}

const bool WargoalType::is_great_war() const {
	return great_war;
}

const bool WargoalType::is_mutual() const {
	return mutual;
}

WargoalType::peace_modifiers_t const& WargoalType::get_modifiers() const {
	return modifiers;
}

const peace_options_t WargoalType::get_peace_options() const {
	return peace_options;
}

WargoalTypeManager::WargoalTypeManager() : wargoal_types { "wargoal types" } {}

const std::vector<WargoalType const*>& WargoalTypeManager::get_peace_priority_list() const {
	return peace_priorities;
}

bool WargoalTypeManager::add_wargoal_type(
	std::string_view identifier,
	std::string_view sprite,
	std::string_view war_name,
	Timespan available_length,
	Timespan truce_length,
	bool triggered_only,
	bool civil_war,
	bool constructing,
	bool crisis,
	bool great_war,
	bool mutual,
	WargoalType::peace_modifiers_t&& modifiers,
	peace_options_t peace_options
) {
	if (identifier.empty()) {
		Logger::error("Invalid wargoal identifier - empty!");
		return false;
	}
	
	if (sprite.empty()) {
		Logger::error("Invalid sprite for wargoal ", identifier, " - empty!");
		return false;
	}

	if (war_name.empty()) {
		Logger::error("Invalid war name for wargoal ", identifier, " - empty!");
		return false;
	}

	return wargoal_types.add_item({ identifier, sprite, war_name, available_length, truce_length, triggered_only, civil_war, constructing, crisis, great_war, mutual, std::move(modifiers), peace_options });
}

bool WargoalTypeManager::load_wargoal_file(ast::NodeCPtr root) {
	bool ret = expect_dictionary(
		[this](std::string_view identifier, ast::NodeCPtr value) -> bool {
			if (identifier == "peace_order") return true;
		
			std::string_view sprite, war_name;
			Timespan available, truce;
			bool triggered_only = false, civil_war = false, constructing = false, crisis = false, great_war = false, mutual = false;
			peace_options_t peace_options;
			WargoalType::peace_modifiers_t modifiers;

			bool ret = expect_dictionary_keys_and_default(
				[&modifiers, &identifier](std::string_view key, ast::NodeCPtr value) -> bool {
					fixed_point_t modifier;
					expect_fixed_point(assign_variable_callback(modifier))(value);

					if (key == "badboy_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::BADBOY_FACTOR] += modifier;
						return true;
					}
					if (key == "prestige_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::PRESTIGE_FACTOR] += modifier;
						return true;
					}
					if (key == "peace_cost_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::PEACE_COST_FACTOR] += modifier;
						return true;
					}
					if (key == "penalty_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::PENALTY_FACTOR] += modifier;
						return true;
					}
					if (key == "break_truce_prestige_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::BREAK_TRUCE_PRESTIGE_FACTOR] += modifier;
						return true;
					}
					if (key == "break_truce_infamy_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::BREAK_TRUCE_INFAMY_FACTOR] += modifier;
						return true;
					}
					if (key == "break_truce_militancy_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::BREAK_TRUCE_MILITANCY_FACTOR] += modifier;
						return true;
					}
					if (key == "good_relation_prestige_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::GOOD_RELATION_PRESTIGE_FACTOR] += modifier;
						return true;
					}
					if (key == "good_relation_infamy_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::GOOD_RELATION_INFAMY_FACTOR] += modifier;
						return true;
					}
					if (key == "good_relation_militancy_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::GOOD_RELATION_MILITANCY_FACTOR] += modifier;
						return true;
					}
					if (key == "tws_battle_factor") {
						modifiers[WargoalType::PEACE_MODIFIERS::WAR_SCORE_BATTLE_FACTOR] += modifier;
						return true;
					}
					if (key == "construction_speed") {
						modifiers[WargoalType::PEACE_MODIFIERS::CONSTRUCTION_SPEED] += modifier;
						return true;
					}

					Logger::error("Modifier ", key, " in wargoal ", identifier, " is invalid.");
					return false;
				},
				"sprite_index", ONE_EXACTLY, expect_identifier(assign_variable_callback(sprite)),
				"war_name", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(war_name)),
				"months", ONE_EXACTLY, expect_months(assign_variable_callback(available)),
				"truce_months", ONE_EXACTLY, expect_months(assign_variable_callback(truce)),
				"is_triggered_only", ZERO_OR_ONE, expect_bool(assign_variable_callback(triggered_only)),
				"is_civil_war", ZERO_OR_ONE, expect_bool(assign_variable_callback(civil_war)),
				"constructing_cb", ZERO_OR_ONE, expect_bool(assign_variable_callback(constructing)),
				"crisis", ZERO_OR_ONE, expect_bool(assign_variable_callback(crisis)),
				"great_war_obligatory", ZERO_OR_ONE, expect_bool(assign_variable_callback(great_war)),
				"mutual", ZERO_OR_ONE, expect_bool(assign_variable_callback(mutual)),
				/* PEACE OPTIONS */
				"po_annex", ZERO_OR_ONE, expect_bool([&peace_options](bool annex) -> bool {
					if (annex) peace_options |= peace_options_t::PO_ANNEX;
					return true;
				}),
				"po_demand_state", ZERO_OR_ONE, expect_bool([&peace_options](bool demand_state) -> bool {
					if (demand_state) peace_options |= peace_options_t::PO_DEMAND_STATE;
					return true;
				}),
				"po_add_to_sphere", ZERO_OR_ONE, expect_bool([&peace_options](bool add_to_sphere) -> bool {
					if (add_to_sphere) peace_options |= peace_options_t::PO_ADD_TO_SPHERE;
					return true;
				}),
				"po_disarmament", ZERO_OR_ONE, expect_bool([&peace_options](bool disarm) -> bool {
					if (disarm) peace_options |= peace_options_t::PO_DISARMAMENT;
					return true;
				}),
				"po_destroy_forts", ZERO_OR_ONE, expect_bool([&peace_options](bool disarm) -> bool {
					if (disarm) peace_options |= peace_options_t::PO_REMOVE_FORTS;
					return true;
				}),
				"po_destroy_naval_bases", ZERO_OR_ONE, expect_bool([&peace_options](bool disarm) -> bool {
					if (disarm) peace_options |= peace_options_t::PO_REMOVE_NAVAL_BASES;
					return true;
				}),
				"po_reparations", ZERO_OR_ONE, expect_bool([&peace_options](bool reps) -> bool {
					if (reps) peace_options |= peace_options_t::PO_REPARATIONS;
					return true;
				}),
				"po_transfer_provinces", ZERO_OR_ONE, expect_bool([&peace_options](bool provinces) -> bool {
					if (provinces) peace_options |= peace_options_t::PO_TRANSFER_PROVINCES;
					return true;
				}),
				"po_remove_prestige", ZERO_OR_ONE, expect_bool([&peace_options](bool humiliate) -> bool {
					if (humiliate) peace_options |= peace_options_t::PO_REMOVE_PRESTIGE;
					return true;
				}),
				"po_make_puppet", ZERO_OR_ONE, expect_bool([&peace_options](bool puppet) -> bool {
					if (puppet) peace_options |= peace_options_t::PO_MAKE_PUPPET;
					return true;
				}),
				"po_release_puppet", ZERO_OR_ONE, expect_bool([&peace_options](bool puppet) -> bool {
					if (puppet) peace_options |= peace_options_t::PO_RELEASE_PUPPET;
					return true;
				}),
				"po_status_quo", ZERO_OR_ONE, expect_bool([&peace_options](bool status_quo) -> bool {
					if (status_quo) peace_options |= peace_options_t::PO_STATUS_QUO;
					return true;
				}),
				"po_install_communist_gov_type", ZERO_OR_ONE, expect_bool([&peace_options](bool puppet) -> bool {
					if (puppet) peace_options |= peace_options_t::PO_INSTALL_COMMUNISM;
					return true;
				}),
				"po_uninstall_communist_gov_type", ZERO_OR_ONE, expect_bool([&peace_options](bool puppet) -> bool {
					if (puppet) peace_options |= peace_options_t::PO_REMOVE_COMMUNISM;
					return true;
				}),
				"po_remove_cores", ZERO_OR_ONE, expect_bool([&peace_options](bool uncore) -> bool {
					if (uncore) peace_options |= peace_options_t::PO_REMOVE_CORES;
					return true;
				}),
				"po_colony", ZERO_OR_ONE, expect_bool([&peace_options](bool colony) -> bool {
					if (colony) peace_options |= peace_options_t::PO_COLONY;
					return true;
				}),
				"po_gunboat", ZERO_OR_ONE, expect_bool([&peace_options](bool gunboat) -> bool {
					if (gunboat) peace_options |= peace_options_t::PO_REPAY_DEBT;
					return true;
				}),
				"po_clear_union_sphere", ZERO_OR_ONE, expect_bool([&peace_options](bool clear) -> bool {
					if (clear) peace_options |= peace_options_t::PO_CLEAR_UNION_SPHERE;
					return true;
				}),
				/* TODO: CONDITION & EFFECT BLOCKS */
				"can_use", ZERO_OR_ONE, success_callback,
				"is_valid", ZERO_OR_ONE, success_callback,
				"on_add", ZERO_OR_ONE, success_callback,
				"on_po_accepted", ZERO_OR_ONE, success_callback,
				"allowed_states", ZERO_OR_ONE, success_callback,
				"all_allowed_states", ZERO_OR_ONE, success_callback,
				"allowed_substate_regions", ZERO_OR_ONE, success_callback,
				"allowed_states_in_crisis", ZERO_OR_ONE, success_callback,
				"allowed_countries", ZERO_OR_ONE, success_callback,
				"always", ZERO_OR_ONE, success_callback // usage unknown / quirk
			)(value);

			add_wargoal_type(identifier, sprite, war_name, available, truce, triggered_only, civil_war, constructing, crisis, great_war, mutual, std::move(modifiers), peace_options);
			return ret;
		}
	)(root);

	/* load order in which CBs are prioritised by AI */
	ret &= expect_dictionary_keys_and_default(
		key_value_success_callback,
		"peace_order", ONE_EXACTLY, expect_list([this](ast::NodeCPtr value) -> bool {
			WargoalType const* wargoal;
			bool ret = expect_wargoal_type_identifier(assign_variable_callback_pointer(wargoal))(value);
			if (ret) {
				peace_priorities.push_back(wargoal);
			} else {
				Logger::warning("Attempted to add invalid wargoal type to AI peace order!");
			}
			return true;
		})
	)(root);

	lock_wargoal_types();
	return ret;
}