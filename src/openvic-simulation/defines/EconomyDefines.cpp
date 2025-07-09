#include "EconomyDefines.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

EconomyDefines::EconomyDefines() {}

std::string_view EconomyDefines::get_name() const {
	return "economy";
}

node_callback_t EconomyDefines::expect_defines() {
	return expect_dictionary_keys(
		"MAX_DAILY_RESEARCH", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_daily_research)),
		"LOAN_BASE_INTEREST", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(loan_base_interest)),
		"BANKRUPTCY_EXTERNAL_LOAN_YEARS", ONE_EXACTLY,
			expect_years(assign_variable_callback(bankruptcy_external_loan_duration)),
		"BANKRUPTCY_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(bankruptcy_factor)),
		"SHADOWY_FINANCIERS_MAX_LOAN_AMOUNT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(shadowy_financiers_max_loan_amount)),
		"MAX_LOAN_CAP_FROM_BANKS", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_loan_cap_from_banks)),
		"GUNBOAT_LOW_TAX_CAP", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(gunboat_low_tax_cap)),
		"GUNBOAT_HIGH_TAX_CAP", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(gunboat_high_tax_cap)),
		"GUNBOAT_FLEET_SIZE_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(gunboat_fleet_size_factor)),
		"PROVINCE_SIZE_DIVIDER", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(province_size_divider)),
		"CAPITALIST_BUILD_FACTORY_STATE_EMPLOYMENT_PERCENT", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(capitalist_build_factory_state_employment_percent)),
		"GOODS_FOCUS_SWAP_CHANCE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(goods_focus_swap_chance)),
		"NUM_CLOSED_FACTORIES_PER_STATE_LASSIEZ_FAIRE", ONE_EXACTLY,
			expect_uint(assign_variable_callback(num_closed_factories_per_state_lassiez_faire)),
		"MIN_NUM_FACTORIES_PER_STATE_BEFORE_DELETING_LASSIEZ_FAIRE", ONE_EXACTLY,
			expect_uint(assign_variable_callback(min_num_factories_per_state_before_deleting_lassiez_faire)),
		"BANKRUPCY_DURATION", ONE_EXACTLY, expect_years(assign_variable_callback(bankruptcy_duration)), // paradox typo
		"SECOND_RANK_BASE_SHARE_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(second_rank_base_share_factor)),
		"CIV_BASE_SHARE_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(civ_base_share_factor)),
		"UNCIV_BASE_SHARE_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(unciv_base_share_factor)),
		"FACTORY_PAYCHECKS_LEFTOVER_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(factory_paychecks_leftover_factor)),
		"MAX_FACTORY_MONEY_SAVE", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_factory_money_save)),
		"SMALL_DEBT_LIMIT", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(small_debt_limit)),
		"FACTORY_UPGRADE_EMPLOYEE_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(factory_upgrade_employee_factor)),
		"RGO_SUPPLY_DEMAND_FACTOR_HIRE_HI", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(rgo_supply_demand_factor_hire_hi)),
		"RGO_SUPPLY_DEMAND_FACTOR_HIRE_LO", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(rgo_supply_demand_factor_hire_lo)),
		"RGO_SUPPLY_DEMAND_FACTOR_FIRE", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(rgo_supply_demand_factor_fire)),
		"EMPLOYMENT_HIRE_LOWEST", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(employment_hire_lowest)),
		"EMPLOYMENT_FIRE_LOWEST", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(employment_fire_lowest)),
		"TRADE_CAP_LOW_LIMIT_LAND", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(minimum_army_spending_slider_value)),
		"TRADE_CAP_LOW_LIMIT_NAVAL", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(minimum_navy_spending_slider_value)),
		"TRADE_CAP_LOW_LIMIT_CONSTRUCTIONS", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(minimum_construction_spending_slider_value)),
		"FACTORY_PURCHASE_MIN_FACTOR", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(factory_purchase_min_factor)),
		"FACTORY_PURCHASE_DRAWDOWN_FACTOR", ONE_EXACTLY,
			expect_fixed_point(assign_variable_callback(factory_purchase_drawdown_factor))
	);
}
