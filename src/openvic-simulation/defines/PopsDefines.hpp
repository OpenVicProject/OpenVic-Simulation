#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DefineManager;

	struct PopsDefines {
		friend struct DefineManager;

	private:
		fixed_point_t PROPERTY(base_clergy_for_literacy);
		fixed_point_t PROPERTY(max_clergy_for_literacy);
		fixed_point_t PROPERTY(literacy_change_speed);
		fixed_point_t PROPERTY(assimilation_scale);
		fixed_point_t PROPERTY(conversion_scale);
		fixed_point_t PROPERTY(immigration_scale);
		fixed_point_t PROPERTY(promotion_scale);
		fixed_point_t PROPERTY(promotion_assimilation_chance);
		fixed_point_t PROPERTY(luxury_threshold);
		fixed_point_t PROPERTY(base_goods_demand);
		fixed_point_t PROPERTY(base_popgrowth);
		ProvinceInstance::life_rating_t PROPERTY(min_life_rating_for_growth);
		fixed_point_t PROPERTY(life_rating_growth_bonus);
		fixed_point_t PROPERTY(life_need_starvation_limit);
		fixed_point_t PROPERTY(mil_lack_everyday_need);
		fixed_point_t PROPERTY(mil_has_everyday_need);
		fixed_point_t PROPERTY(mil_has_luxury_need);
		fixed_point_t PROPERTY(mil_no_life_need);
		fixed_point_t PROPERTY(mil_require_reform);
		fixed_point_t PROPERTY(mil_ideology);
		fixed_point_t PROPERTY(mil_ruling_party);
		fixed_point_t PROPERTY(mil_reform_impact);
		fixed_point_t PROPERTY(mil_war_exhaustion);
		fixed_point_t PROPERTY(mil_non_accepted);
		fixed_point_t PROPERTY(con_literacy);
		fixed_point_t PROPERTY(con_luxury_goods);
		fixed_point_t PROPERTY(con_poor_clergy);
		fixed_point_t PROPERTY(con_midrich_clergy);
		fixed_point_t PROPERTY(con_reform_impact);
		fixed_point_t PROPERTY(con_colonial_factor);
		fixed_point_t PROPERTY(ruling_party_happy_change);
		fixed_point_t PROPERTY(ruling_party_angry_change);
		fixed_point_t PROPERTY(pdef_base_con);
		fixed_point_t PROPERTY(national_focus_divider);
		fixed_point_t PROPERTY(pop_savings);
		fixed_point_t PROPERTY(state_creation_admin_limit);
		fixed_point_t PROPERTY(mil_to_join_rebel);
		fixed_point_t PROPERTY(mil_to_join_rising);
		fixed_point_t PROPERTY(mil_to_autorise);
		fixed_point_t PROPERTY(reduction_after_riseing);
		fixed_point_t PROPERTY(reduction_after_defeat);
		fixed_point_t PROPERTY(pop_to_leadership);
		fixed_point_t PROPERTY(artisan_min_productivity);
		fixed_point_t PROPERTY(slave_growth_divisor);
		fixed_point_t PROPERTY(mil_hit_from_conquest);
		fixed_point_t PROPERTY(luxury_con_change);
		fixed_point_t PROPERTY(invention_impact_on_demand);
		fixed_point_t PROPERTY(artisan_suppressed_colonial_goods_category);
		fixed_point_t PROPERTY(issue_movement_join_limit);
		fixed_point_t PROPERTY(issue_movement_leave_limit);
		fixed_point_t PROPERTY(movement_con_factor);
		fixed_point_t PROPERTY(movement_lit_factor);
		fixed_point_t PROPERTY(mil_on_reb_move);
		fixed_point_t PROPERTY(population_suppression_factor);
		fixed_point_t PROPERTY(population_movement_radical_factor);
		fixed_point_t PROPERTY(nationalist_movement_mil_cap);
		fixed_point_t PROPERTY(movement_support_uh_factor);
		fixed_point_t PROPERTY(rebel_occupation_strength_bonus);
		pop_size_t PROPERTY(large_population_limit);
		pop_size_t PROPERTY(large_population_influence_penalty_chunk);

		PopsDefines();

		std::string_view get_name() const;
		NodeTools::node_callback_t expect_defines();
	};
}
