#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DefineManager;

	struct CountryDefines {
		friend struct DefineManager;

	private:
		Timespan PROPERTY(nationalism_duration);
		Timespan PROPERTY(rebels_hold_capital_success_duration); // NOT USED
		Timespan PROPERTY(rebel_success_duration);
		fixed_point_t PROPERTY(base_country_tax_efficiency);
		fixed_point_t PROPERTY(base_country_admin_efficiency);
		fixed_point_t PROPERTY(gold_to_cash_rate);
		fixed_point_t PROPERTY(gold_to_worker_pay_rate);
		size_t PROPERTY(great_power_rank);
		Timespan PROPERTY(lose_great_power_grace_days);
		fixed_point_t PROPERTY(infamy_containment_limit);
		fixed_point_t PROPERTY(max_bureaucracy_percentage);
		fixed_point_t PROPERTY(bureaucracy_percentage_increment);
		fixed_point_t PROPERTY(min_crimefight_percent);
		fixed_point_t PROPERTY(max_crimefight_percent);
		fixed_point_t PROPERTY(admin_efficiency_crimefight_percent);
		fixed_point_t PROPERTY(conservative_increase_after_reform);
		Timespan PROPERTY(campaign_event_base_duration);
		Timespan PROPERTY(campaign_event_min_duration); // NOT USED
		Timespan PROPERTY(campaign_event_state_duration_modifier); // NOT USED
		Timespan PROPERTY(campaign_duration);
		size_t PROPERTY(secondary_power_rank);
		fixed_point_t PROPERTY(colony_to_state_prestige_gain);
		ProvinceInstance::life_rating_t PROPERTY(colonial_liferating);
		fixed_point_t PROPERTY(base_greatpower_daily_influence);
		fixed_point_t PROPERTY(ai_support_reform);
		fixed_point_t PROPERTY(base_monthly_diplopoints);
		Timespan PROPERTY(diplomat_travel_duration);
		fixed_point_t PROPERTY(province_overseas_penalty);
		fixed_point_t PROPERTY(noncore_tax_penalty);
		fixed_point_t PROPERTY(base_tariff_efficiency);
		fixed_point_t PROPERTY(colony_formed_prestige);
		Timespan PROPERTY(created_cb_valid_time);
		fixed_point_t PROPERTY(loyalty_boost_on_party_win);
		fixed_point_t PROPERTY(movement_radicalism_base);
		fixed_point_t PROPERTY(movement_radicalism_passed_reform_effect);
		fixed_point_t PROPERTY(movement_radicalism_nationalism_factor);
		fixed_point_t PROPERTY(suppression_points_gain_base);
		fixed_point_t PROPERTY(suppress_bureaucrat_factor);
		fixed_point_t PROPERTY(wrong_reform_militancy_impact);
		fixed_point_t PROPERTY(suppression_radicalisation_hit);
		fixed_point_t PROPERTY(country_investment_industrial_score_factor);
		fixed_point_t PROPERTY(unciv_tech_spread_max);
		fixed_point_t PROPERTY(unciv_tech_spread_min);
		Timespan PROPERTY(min_delay_duration_between_reforms);
		fixed_point_t PROPERTY(economic_reform_uh_factor);
		fixed_point_t PROPERTY(military_reform_uh_factor);
		fixed_point_t PROPERTY(wrong_reform_radical_impact);
		fixed_point_t PROPERTY(tech_year_span);
		fixed_point_t PROPERTY(tech_factor_vassal);
		fixed_point_t PROPERTY(max_suppression);
		fixed_point_t PROPERTY(prestige_hit_on_break_country);
		size_t PROPERTY(min_mobilize_limit);
		Timespan PROPERTY(pop_growth_country_cache_days);
		Timespan PROPERTY(newspaper_printing_frequency);
		Timespan PROPERTY(newspaper_timeout_period);
		fixed_point_t PROPERTY(newspaper_max_tension);
		fixed_point_t PROPERTY(naval_base_supply_score_base);
		fixed_point_t PROPERTY(naval_base_supply_score_empty);
		fixed_point_t PROPERTY(naval_base_non_core_supply_score);
		fixed_point_t PROPERTY(colonial_points_from_supply_factor);
		fixed_point_t PROPERTY(colonial_points_for_non_core_base);
		fixed_point_t PROPERTY(mobilization_speed_base);
		fixed_point_t PROPERTY(mobilization_speed_rails_mult);
		fixed_point_t PROPERTY(colonization_interest_lead);
		fixed_point_t PROPERTY(colonization_influence_lead);
		Timespan PROPERTY(colonization_duration);
		Timespan PROPERTY(colonization_days_between_investment);
		Timespan PROPERTY(colonization_days_for_initial_investment);
		fixed_point_t PROPERTY(colonization_protectorate_province_maintenance);
		fixed_point_t PROPERTY(colonization_colony_province_maintenance);
		fixed_point_t PROPERTY(colonization_colony_industry_maintenance);
		fixed_point_t PROPERTY(colonization_colony_railway_maintenance);
		fixed_point_t PROPERTY(colonization_interest_cost_initial);
		fixed_point_t PROPERTY(colonization_interest_cost_neighbor_modifier);
		fixed_point_t PROPERTY(colonization_interest_cost);
		fixed_point_t PROPERTY(colonization_influence_cost);
		fixed_point_t PROPERTY(colonization_extra_guard_cost);
		fixed_point_t PROPERTY(colonization_release_dominion_cost);
		fixed_point_t PROPERTY(colonization_create_state_cost);
		fixed_point_t PROPERTY(colonization_create_protectorate_cost);
		fixed_point_t PROPERTY(colonization_create_colony_cost);
		fixed_point_t PROPERTY(colonization_colony_state_distance);
		fixed_point_t PROPERTY(colonization_influence_temperature_per_day);
		fixed_point_t PROPERTY(colonization_influence_temperature_per_level);
		fixed_point_t PROPERTY(party_loyalty_hit_on_war_loss);
		fixed_point_t PROPERTY(research_points_on_conquer_mult);
		fixed_point_t PROPERTY(max_research_points);

		CountryDefines();

		std::string_view get_name() const;
		NodeTools::node_callback_t expect_defines();
	};
}
