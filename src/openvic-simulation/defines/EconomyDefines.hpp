#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DefineManager;

	struct EconomyDefines {
		friend struct DefineManager;

	private:
		fixed_point_t PROPERTY(max_daily_research);
		fixed_point_t PROPERTY(loan_base_interest);
		Timespan PROPERTY(bankruptcy_external_loan_duration);
		fixed_point_t PROPERTY(bankruptcy_factor);
		fixed_point_t PROPERTY(shadowy_financiers_max_loan_amount);
		fixed_point_t PROPERTY(max_loan_cap_from_banks);
		fixed_point_t PROPERTY(gunboat_low_tax_cap);
		fixed_point_t PROPERTY(gunboat_high_tax_cap);
		fixed_point_t PROPERTY(gunboat_fleet_size_factor);
		fixed_point_t PROPERTY(province_size_divider);
		fixed_point_t PROPERTY(capitalist_build_factory_state_employment_percent);
		fixed_point_t PROPERTY(goods_focus_swap_chance);
		size_t PROPERTY(num_closed_factories_per_state_lassiez_faire);
		size_t PROPERTY(min_num_factories_per_state_before_deleting_lassiez_faire);
		Timespan PROPERTY(bankrupcy_duration);
		fixed_point_t PROPERTY(second_rank_base_share_factor);
		fixed_point_t PROPERTY(civ_base_share_factor);
		fixed_point_t PROPERTY(unciv_base_share_factor);
		fixed_point_t PROPERTY(factory_paychecks_leftover_factor);
		fixed_point_t PROPERTY(max_factory_money_save);
		fixed_point_t PROPERTY(small_debt_limit);
		fixed_point_t PROPERTY(factory_upgrade_employee_factor);
		fixed_point_t PROPERTY(rgo_supply_demand_factor_hire_hi);
		fixed_point_t PROPERTY(rgo_supply_demand_factor_hire_lo);
		fixed_point_t PROPERTY(rgo_supply_demand_factor_fire);
		fixed_point_t PROPERTY(employment_hire_lowest);
		fixed_point_t PROPERTY(employment_fire_lowest);
		fixed_point_t PROPERTY(trade_cap_low_limit_land);
		fixed_point_t PROPERTY(trade_cap_low_limit_naval);
		fixed_point_t PROPERTY(trade_cap_low_limit_constructions);
		fixed_point_t PROPERTY(factory_purchase_min_factor);
		fixed_point_t PROPERTY(factory_purchase_drawdown_factor);

		EconomyDefines();

		std::string_view get_name() const;
		NodeTools::node_callback_t expect_defines();
	};
}
