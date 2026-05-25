#include "Wargoal.hpp"

#include <range/v3/algorithm/contains.hpp>

using namespace OpenVic;

WargoalType::WargoalType(
	std::string_view new_identifier, std::string_view new_war_name, Timespan new_available_length,
	Timespan new_truce_length, sprite_t new_sprite_index, bool new_triggered_only, bool new_civil_war,
	bool new_constructing, bool new_crisis, bool new_great_war_obligatory, bool new_mutual,
	bool new_all_allowed_states, bool new_always, peace_modifiers_t&& new_modifiers, peace_options_t new_peace_options,
	ConditionScript&& new_can_use, ConditionScript&& new_is_valid, ConditionScript&& new_allowed_states,
	ConditionScript&& new_allowed_substate_regions, ConditionScript&& new_allowed_states_in_crisis,
	ConditionScript&& new_allowed_countries, EffectScript&& new_on_add, EffectScript&& new_on_po_accepted
) : HasIdentifier { new_identifier }, war_name { new_war_name }, available_length { new_available_length },
	truce_length { new_truce_length }, sprite_index { new_sprite_index }, is_triggered_only { new_triggered_only },
	is_civil_war { new_civil_war }, constructing { new_constructing }, crisis { new_crisis },
	is_great_war_obligatory { new_great_war_obligatory }, is_mutual { new_mutual }, all_allowed_states { new_all_allowed_states },
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
