#include "PopsDefines.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

PopsDefines::PopsDefines() {}

std::string_view PopsDefines::get_name() const {
	return "pops";
}

node_callback_t PopsDefines::expect_defines() {
	return expect_dictionary_keys(
		"BASE_CLERGY_FOR_LITERACY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base_clergy_for_literacy)),
		"MAX_CLERGY_FOR_LITERACY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_clergy_for_literacy)),
		"LITERACY_CHANGE_SPEED", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(literacy_change_speed)),
		"ASSIMILATION_SCALE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(assimilation_scale)),
		"CONVERSION_SCALE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(conversion_scale)),
		"IMMIGRATION_SCALE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(immigration_scale)),
		"PROMOTION_SCALE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(promotion_scale)),
		"PROMOTION_ASSIMILATION_CHANCE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(promotion_assimilation_chance)),
		"LUXURY_THRESHOLD", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(luxury_threshold)),
		"BASE_GOODS_DEMAND", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base_goods_demand)),
		"BASE_POPGROWTH", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(base_popgrowth)),
		"MIN_LIFE_RATING_FOR_GROWTH", ONE_EXACTLY, expect_uint(assign_variable_callback(min_life_rating_for_growth)),
		"LIFE_RATING_GROWTH_BONUS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(life_rating_growth_bonus)),
		"LIFE_NEED_STARVATION_LIMIT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(life_need_starvation_limit)),
		"MIL_LACK_EVERYDAY_NEED", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_lack_everyday_need)),
		"MIL_HAS_EVERYDAY_NEED", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_has_everyday_need)),
		"MIL_HAS_LUXURY_NEED", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_has_luxury_need)),
		"MIL_NO_LIFE_NEED", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_no_life_need)),
		"MIL_REQUIRE_REFORM", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_require_reform)),
		"MIL_IDEOLOGY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_ideology)),
		"MIL_RULING_PARTY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_ruling_party)),
		"MIL_REFORM_IMPACT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_reform_impact)),
		"MIL_WAR_EXHAUSTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_war_exhaustion)),
		"MIL_NON_ACCEPTED", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_non_accepted)),
		"CON_LITERACY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(con_literacy)),
		"CON_LUXURY_GOODS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(con_luxury_goods)),
		"CON_POOR_CLERGY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(con_poor_clergy)),
		"CON_MIDRICH_CLERGY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(con_midrich_clergy)),
		"CON_REFORM_IMPACT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(con_reform_impact)),
		"CON_COLONIAL_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(con_colonial_factor)),
		"RULING_PARTY_HAPPY_CHANGE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ruling_party_happy_change)),
		"RULING_PARTY_ANGRY_CHANGE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ruling_party_angry_change)),
		"PDEF_BASE_CON", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pdef_base_con)),
		"NATIONAL_FOCUS_DIVIDER", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(national_focus_divider)),
		"POP_SAVINGS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pop_savings)),
		"STATE_CREATION_ADMIN_LIMIT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(state_creation_admin_limit)),
		"MIL_TO_JOIN_REBEL", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_to_join_rebel)),
		"MIL_TO_JOIN_RISING", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_to_join_rising)),
		"MIL_TO_AUTORISE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_to_autorise)),
		"REDUCTION_AFTER_RISEING", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(reduction_after_riseing)),
		"REDUCTION_AFTER_DEFEAT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(reduction_after_defeat)),
		"POP_TO_LEADERSHIP", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pop_to_leadership)),
		"ARTISAN_MIN_PRODUCTIVITY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(artisan_min_productivity)),
		"SLAVE_GROWTH_DIVISOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(slave_growth_divisor)),
		"MIL_HIT_FROM_CONQUEST", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_hit_from_conquest)),
		"LUXURY_CON_CHANGE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(luxury_con_change)),
		"INVENTION_IMPACT_ON_DEMAND", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(invention_impact_on_demand)),
		"ARTISAN_SUPPRESSED_COLONIAL_GOODS_CATEGORY", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(artisan_suppressed_colonial_goods_category)),
		"ISSUE_MOVEMENT_JOIN_LIMIT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(issue_movement_join_limit)),
		"ISSUE_MOVEMENT_LEAVE_LIMIT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(issue_movement_leave_limit)),
		"MOVEMENT_CON_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(movement_con_factor)),
		"MOVEMENT_LIT_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(movement_lit_factor)),
		"MIL_ON_REB_MOVE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(mil_on_reb_move)),
		"POPULATION_SUPPRESSION_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(population_suppression_factor)),
		"POPULATION_MOVEMENT_RADICAL_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(population_movement_radical_factor)),
		"NATIONALIST_MOVEMENT_MIL_CAP", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(nationalist_movement_mil_cap)),
		"MOVEMENT_SUPPORT_UH_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(movement_support_uh_factor)),
		"REBEL_OCCUPATION_STRENGTH_BONUS", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(rebel_occupation_strength_bonus)),
		"LARGE_POPULATION_LIMIT", ONE_EXACTLY, expect_uint(assign_variable_callback(large_population_limit)),
		"LARGE_POPULATION_INFLUENCE_PENALTY_CHUNK", ONE_EXACTLY,
			expect_uint(assign_variable_callback(large_population_influence_penalty_chunk))
	);
}
