#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DefineManager;

	struct AIDefines {
		friend struct DefineManager;

	private:
		fixed_point_t PROPERTY(colony_weight);
		fixed_point_t PROPERTY(administrator_weight);
		fixed_point_t PROPERTY(industryworker_weight);
		fixed_point_t PROPERTY(educator_weight);
		fixed_point_t PROPERTY(soldier_weight);
		fixed_point_t PROPERTY(soldier_fraction);
		fixed_point_t PROPERTY(capitalist_fraction);
		fixed_point_t PROPERTY(production_weight);
		fixed_point_t PROPERTY(spam_penalty);
		fixed_point_t PROPERTY(one_side_max_warscore);
		fixed_point_t PROPERTY(pop_project_investment_max_budget_factor);
		fixed_point_t PROPERTY(relation_limit_no_alliance_offer);
		fixed_point_t PROPERTY(naval_supply_penalty_limit);
		fixed_point_t PROPERTY(chance_build_railroad);
		fixed_point_t PROPERTY(chance_build_naval_base);
		fixed_point_t PROPERTY(chance_build_fort);
		fixed_point_t PROPERTY(chance_invest_pop_proj);
		fixed_point_t PROPERTY(chance_foreign_invest);
		fixed_point_t PROPERTY(tws_awareness_score_low_cap);
		fixed_point_t PROPERTY(tws_awareness_score_aspect);
		fixed_point_t PROPERTY(peace_base_reluctance);
		Timespan PROPERTY(peace_time_duration);
		fixed_point_t PROPERTY(peace_time_factor);
		fixed_point_t PROPERTY(peace_time_factor_no_goals);
		fixed_point_t PROPERTY(peace_war_exhaustion_factor);
		fixed_point_t PROPERTY(peace_war_direction_factor);
		fixed_point_t PROPERTY(peace_war_direction_winning_mult);
		fixed_point_t PROPERTY(peace_force_balance_factor);
		fixed_point_t PROPERTY(peace_ally_base_reluctance_mult);
		fixed_point_t PROPERTY(peace_ally_time_mult);
		fixed_point_t PROPERTY(peace_ally_war_exhaustion_mult);
		fixed_point_t PROPERTY(peace_ally_war_direction_mult);
		fixed_point_t PROPERTY(peace_ally_force_balance_mult);
		fixed_point_t PROPERTY(aggression_base);
		fixed_point_t PROPERTY(aggression_unciv_bonus);
		size_t PROPERTY(fleet_size, 0);
		size_t PROPERTY(min_fleets, 0);
		size_t PROPERTY(max_fleets, 0);
		Timespan PROPERTY(time_before_disband);

		AIDefines();

		std::string_view get_name() const;
		NodeTools::node_callback_t expect_defines();
	};
}
