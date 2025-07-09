#pragma once

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct DefineManager;

	struct MilitaryDefines {
		friend struct DefineManager;

	private:
		Timespan PROPERTY(dig_in_increase_each_days);
		fixed_point_t PROPERTY(reinforce_speed);
		fixed_point_t PROPERTY(combat_difficulty_impact);
		size_t PROPERTY(base_combat_width, 0);
		pop_size_t PROPERTY(min_pop_size_for_regiment, 0);
		pop_size_t PROPERTY(pop_size_per_regiment, 0);
		fixed_point_t PROPERTY(soldier_to_pop_damage);
		fixed_point_t PROPERTY(land_speed_modifier);
		fixed_point_t PROPERTY(naval_speed_modifier);
		fixed_point_t PROPERTY(exp_gain_div);
		fixed_point_t PROPERTY(leader_recruit_cost);
		fixed_point_t PROPERTY(supply_range);
		fixed_point_t PROPERTY(pop_size_per_regiment_protectorate_multiplier);
		fixed_point_t PROPERTY(pop_size_per_regiment_colony_multiplier);
		fixed_point_t PROPERTY(pop_size_per_regiment_non_core_multiplier);
		size_t PROPERTY(gas_attack_modifier, 0);
		fixed_point_t PROPERTY(combatloss_war_exhaustion);
		fixed_point_t PROPERTY(leader_max_random_prestige);
		fixed_point_t PROPERTY(leader_age_death_factor);
		fixed_point_t PROPERTY(leader_prestige_to_morale_factor);
		fixed_point_t PROPERTY(leader_prestige_to_max_org_factor);
		fixed_point_t PROPERTY(leader_transfer_penalty_on_country_prestige);
		fixed_point_t PROPERTY(leader_prestige_land_gain);
		fixed_point_t PROPERTY(leader_prestige_naval_gain);
		fixed_point_t PROPERTY(naval_combat_seeking_chance);
		fixed_point_t PROPERTY(naval_combat_seeking_chance_min);
		fixed_point_t PROPERTY(naval_combat_self_defence_chance);
		fixed_point_t PROPERTY(naval_combat_shift_back_on_next_target);
		fixed_point_t PROPERTY(naval_combat_shift_back_duration_scale);
		fixed_point_t PROPERTY(naval_combat_speed_to_distance_factor);
		fixed_point_t PROPERTY(naval_combat_change_target_chance);
		fixed_point_t PROPERTY(naval_combat_damage_org_mult);
		fixed_point_t PROPERTY(naval_combat_damage_str_mult);
		fixed_point_t PROPERTY(naval_combat_damage_mult_no_org);
		fixed_point_t PROPERTY(naval_combat_retreat_chance);
		fixed_point_t PROPERTY(naval_combat_retreat_str_org_level);
		fixed_point_t PROPERTY(naval_combat_retreat_speed_mod);
		fixed_point_t PROPERTY(naval_combat_retreat_min_distance);
		fixed_point_t PROPERTY(naval_combat_damaged_target_selection);
		fixed_point_t PROPERTY(naval_combat_stacking_target_change);
		fixed_point_t PROPERTY(naval_combat_stacking_target_select);
		size_t PROPERTY(naval_combat_max_targets, 0);
		fixed_point_t PROPERTY(ai_bigship_proportion);
		fixed_point_t PROPERTY(ai_lightship_proportion);
		fixed_point_t PROPERTY(ai_transport_proportion);
		fixed_point_t PROPERTY(ai_cavalry_proportion);
		fixed_point_t PROPERTY(ai_support_proportion);
		fixed_point_t PROPERTY(ai_special_proportion);
		fixed_point_t PROPERTY(ai_escort_ratio);
		fixed_point_t PROPERTY(ai_army_taxbase_fraction);
		fixed_point_t PROPERTY(ai_navy_taxbase_fraction);
		fixed_point_t PROPERTY(ai_blockade_range);
		fixed_point_t PROPERTY(recon_unit_ratio);
		fixed_point_t PROPERTY(engineer_unit_ratio);
		size_t PROPERTY(siege_brigades_min, 0);
		size_t PROPERTY(siege_brigades_max, 0);
		fixed_point_t PROPERTY(siege_brigades_bonus);
		fixed_point_t PROPERTY(recon_siege_effect);
		fixed_point_t PROPERTY(siege_attrition);
		fixed_point_t PROPERTY(base_military_tactics);
		fixed_point_t PROPERTY(naval_low_supply_damage_supply_status);
		Timespan PROPERTY(naval_low_supply_damage_days_delay);
		fixed_point_t PROPERTY(naval_low_supply_damage_min_str);
		fixed_point_t PROPERTY(naval_low_supply_damage_per_day);

		MilitaryDefines();

		std::string_view get_name() const;
		NodeTools::node_callback_t expect_defines();

	public:
		constexpr fixed_point_t get_max_leadership_point_stockpile() const {
			return leader_recruit_cost * 3;
		}
	};
}
