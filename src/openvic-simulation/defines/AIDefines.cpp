#include "AIDefines.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

AIDefines::AIDefines() {}

std::string_view AIDefines::get_name() const {
	return "ai";
}

node_callback_t AIDefines::expect_defines() {
	return expect_dictionary_keys(
		"COLONY_WEIGHT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(colony_weight)),
		"ADMINISTRATOR_WEIGHT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(administrator_weight)),
		"INDUSTRYWORKER_WEIGHT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(industryworker_weight)),
		"EDUCATOR_WEIGHT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(educator_weight)),
		"SOLDIER_WEIGHT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(soldier_weight)),
		"SOLDIER_FRACTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(soldier_fraction)),
		"CAPITALIST_FRACTION", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(capitalist_fraction)),
		"PRODUCTION_WEIGHT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(production_weight)),
		"SPAM_PENALTY", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(spam_penalty)),
		"ONE_SIDE_MAX_WARSCORE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(one_side_max_warscore)),
		"POP_PROJECT_INVESTMENT_MAX_BUDGET_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(pop_project_investment_max_budget_factor)),
		"RELATION_LIMIT_NO_ALLIANCE_OFFER", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(relation_limit_no_alliance_offer)),
		"NAVAL_SUPPLY_PENALTY_LIMIT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(naval_supply_penalty_limit)),
		"CHANCE_BUILD_RAILROAD", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(chance_build_railroad)),
		"CHANCE_BUILD_NAVAL_BASE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(chance_build_naval_base)),
		"CHANCE_BUILD_FORT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(chance_build_fort)),
		"CHANCE_INVEST_POP_PROJ", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(chance_invest_pop_proj)),
		"CHANCE_FOREIGN_INVEST", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(chance_foreign_invest)),
		"TWS_AWARENESS_SCORE_LOW_CAP", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(tws_awareness_score_low_cap)),
		"TWS_AWARENESS_SCORE_ASPECT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(tws_awareness_score_aspect)),
		"PEACE_BASE_RELUCTANCE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(peace_base_reluctance)),
		"PEACE_TIME_MONTHS", ONE_EXACTLY, expect_months(assign_variable_callback(peace_time_duration)),
		"PEACE_TIME_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(peace_time_factor)),
		"PEACE_TIME_FACTOR_NO_GOALS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(peace_time_factor_no_goals)),
		"PEACE_WAR_EXHAUSTION_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(peace_war_exhaustion_factor)),
		"PEACE_WAR_DIRECTION_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(peace_war_direction_factor)),
		"PEACE_WAR_DIRECTION_WINNING_MULT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(peace_war_direction_winning_mult)),
		"PEACE_FORCE_BALANCE_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(peace_force_balance_factor)),
		"PEACE_ALLY_BASE_RELUCTANCE_MULT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(peace_ally_base_reluctance_mult)),
		"PEACE_ALLY_TIME_MULT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(peace_ally_time_mult)),
		"PEACE_ALLY_WAR_EXHAUSTION_MULT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(peace_ally_war_exhaustion_mult)),
		"PEACE_ALLY_WAR_DIRECTION_MULT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(peace_ally_war_direction_mult)),
		"PEACE_ALLY_FORCE_BALANCE_MULT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(peace_ally_force_balance_mult)),
		"AGGRESSION_BASE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(aggression_base)),
		"AGGRESSION_UNCIV_BONUS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(aggression_unciv_bonus)),
		"FLEET_SIZE", ONE_EXACTLY, expect_uint(assign_variable_callback(fleet_size)),
		"MIN_FLEETS", ONE_EXACTLY, expect_uint(assign_variable_callback(min_fleets)),
		"MAX_FLEETS", ONE_EXACTLY, expect_uint(assign_variable_callback(max_fleets)),
		"MONTHS_BEFORE_DISBAND", ONE_EXACTLY, expect_months(assign_variable_callback(time_before_disband))
	);
}
