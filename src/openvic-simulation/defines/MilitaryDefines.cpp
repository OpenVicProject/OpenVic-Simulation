#include "MilitaryDefines.hpp"

#include <cstdint>

#include "openvic-simulation/military/CombatWidth.hpp"

#include <type_safe/strong_typedef.hpp>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

MilitaryDefines::MilitaryDefines() {}

std::string_view MilitaryDefines::get_name() const {
	return "military";
}

node_callback_t MilitaryDefines::expect_defines() {
	return expect_dictionary_keys(
		"DIG_IN_INCREASE_EACH_DAYS", ONE_EXACTLY, expect_days(assign_variable_callback(dig_in_increase_each_days)),
		"REINFORCE_SPEED", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(reinforce_speed)),
		"COMBAT_DIFFICULTY_IMPACT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(combat_difficulty_impact)),
		"BASE_COMBAT_WIDTH", ONE_EXACTLY, expect_int<int8_t>([this](int8_t val)->bool{
			base_combat_width = combat_width_t(static_cast<type_safe::underlying_type<combat_width_t>>(val));
			return true;
		}),
		"POP_MIN_SIZE_FOR_REGIMENT", ONE_EXACTLY, expect_uint(assign_variable_callback(min_pop_size_for_regiment)),
		"POP_SIZE_PER_REGIMENT", ONE_EXACTLY, expect_uint(assign_variable_callback(pop_size_per_regiment)),
		"SOLDIER_TO_POP_DAMAGE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(soldier_to_pop_damage)),
		"LAND_SPEED_MODIFIER", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(land_speed_modifier)),
		"NAVAL_SPEED_MODIFIER", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(naval_speed_modifier)),
		"EXP_GAIN_DIV", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(exp_gain_div)),
		"LEADER_RECRUIT_COST", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(leader_recruit_cost)),
		"SUPPLY_RANGE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(supply_range)),
		"POP_MIN_SIZE_FOR_REGIMENT_PROTECTORATE_MULTIPLIER", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(pop_size_per_regiment_protectorate_multiplier)),
		"POP_MIN_SIZE_FOR_REGIMENT_COLONY_MULTIPLIER", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(pop_size_per_regiment_colony_multiplier)),
		"POP_MIN_SIZE_FOR_REGIMENT_NONCORE_MULTIPLIER", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(pop_size_per_regiment_non_core_multiplier)),
		"GAS_ATTACK_MODIFIER", ONE_EXACTLY, expect_uint(assign_variable_callback(gas_attack_modifier)),
		"COMBATLOSS_WAR_EXHAUSTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(combatloss_war_exhaustion)),
		"LEADER_MAX_RANDOM_PRESTIGE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(leader_max_random_prestige)),
		"LEADER_AGE_DEATH_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(leader_age_death_factor)),
		"LEADER_PRESTIGE_TO_MORALE_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(leader_prestige_to_morale_factor)),
		"LEADER_PRESTIGE_TO_MAX_ORG_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(leader_prestige_to_max_org_factor)),
		"LEADER_TRANSFER_PENALTY_ON_COUNTRY_PRESTIGE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(leader_transfer_penalty_on_country_prestige)),
		"LEADER_PRESTIGE_LAND_GAIN", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(leader_prestige_land_gain)),
		"LEADER_PRESTIGE_NAVAL_GAIN", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(leader_prestige_naval_gain)),
		"NAVAL_COMBAT_SEEKING_CHANCE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(naval_combat_seeking_chance)),
		"NAVAL_COMBAT_SEEKING_CHANCE_MIN", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_seeking_chance_min)),
		"NAVAL_COMBAT_SELF_DEFENCE_CHANCE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_self_defence_chance)),
		"NAVAL_COMBAT_SHIFT_BACK_ON_NEXT_TARGET", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_shift_back_on_next_target)),
		"NAVAL_COMBAT_SHIFT_BACK_DURATION_SCALE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_shift_back_duration_scale)),
		"NAVAL_COMBAT_SPEED_TO_DISTANCE_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_speed_to_distance_factor)),
		"NAVAL_COMBAT_CHANGE_TARGET_CHANCE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_change_target_chance)),
		"NAVAL_COMBAT_DAMAGE_ORG_MULT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_damage_org_mult)),
		"NAVAL_COMBAT_DAMAGE_STR_MULT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_damage_str_mult)),
		"NAVAL_COMBAT_DAMAGE_MULT_NO_ORG", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_damage_mult_no_org)),
		"NAVAL_COMBAT_RETREAT_CHANCE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(naval_combat_retreat_chance)),
		"NAVAL_COMBAT_RETREAT_STR_ORG_LEVEL", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_retreat_str_org_level)),
		"NAVAL_COMBAT_RETREAT_SPEED_MOD", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_retreat_speed_mod)),
		"NAVAL_COMBAT_RETREAT_MIN_DISTANCE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_retreat_min_distance)),
		"NAVAL_COMBAT_DAMAGED_TARGET_SELECTION", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_damaged_target_selection)),
		"NAVAL_COMBAT_STACKING_TARGET_CHANGE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_stacking_target_change)),
		"NAVAL_COMBAT_STACKING_TARGET_SELECT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_combat_stacking_target_select)),
		"NAVAL_COMBAT_MAX_TARGETS", ONE_EXACTLY, expect_uint(assign_variable_callback(naval_combat_max_targets)),
		"AI_BIGSHIP_PROPORTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_bigship_proportion)),
		"AI_LIGHTSHIP_PROPORTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_lightship_proportion)),
		"AI_TRANSPORT_PROPORTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_transport_proportion)),
		"AI_CAVALRY_PROPORTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_cavalry_proportion)),
		"AI_SUPPORT_PROPORTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_support_proportion)),
		"AI_SPECIAL_PROPORTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_special_proportion)),
		"AI_ESCORT_RATIO", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_escort_ratio)),
		"AI_ARMY_TAXBASE_FRACTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_army_taxbase_fraction)),
		"AI_NAVY_TAXBASE_FRACTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_navy_taxbase_fraction)),
		"AI_BLOCKADE_RANGE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ai_blockade_range)),
		"RECON_UNIT_RATIO", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(recon_unit_ratio)),
		"ENGINEER_UNIT_RATIO", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(engineer_unit_ratio)),
		"SIEGE_BRIGADES_MIN", ONE_EXACTLY, expect_uint(assign_variable_callback(siege_brigades_min)),
		"SIEGE_BRIGADES_MAX", ONE_EXACTLY, expect_uint(assign_variable_callback(siege_brigades_max)),
		"SIEGE_BRIGADES_BONUS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(siege_brigades_bonus)),
		"RECON_SIEGE_EFFECT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(recon_siege_effect)),
		"SIEGE_ATTRITION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(siege_attrition)),
		"BASE_MILITARY_TACTICS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base_military_tactics)),
		"NAVAL_LOW_SUPPLY_DAMAGE_SUPPLY_STATUS", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_low_supply_damage_supply_status)),
		"NAVAL_LOW_SUPPLY_DAMAGE_DAYS_DELAY", ONE_EXACTLY,
			expect_days(assign_variable_callback(naval_low_supply_damage_days_delay)),
		"NAVAL_LOW_SUPPLY_DAMAGE_MIN_STR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_low_supply_damage_min_str)),
		"NAVAL_LOW_SUPPLY_DAMAGE_PER_DAY", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(naval_low_supply_damage_per_day))
	);
}
