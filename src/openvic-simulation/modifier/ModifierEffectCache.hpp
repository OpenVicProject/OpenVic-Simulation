#pragma once

#include "openvic-simulation/core/memory/IndexedFlatMap.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ModifierEffect;
	struct ModifierManager;

	struct BuildingTypeManager;
	struct BuildingType;
	struct GoodDefinitionManager;
	struct GoodDefinition;
	struct RebelManager;
	struct RebelType;
	struct PopManager;
	struct Strata;
	struct TechnologyManager;
	struct TechnologyFolder;
	struct TerrainTypeManager;
	struct TerrainType;

	struct ModifierEffectCache {
		friend struct ModifierManager;
		friend struct BuildingTypeManager;
		friend struct GoodDefinitionManager;
		friend struct UnitTypeManager;
		friend struct RebelManager;
		friend struct PopManager;
		friend struct TechnologyManager;
		friend struct TerrainTypeManager;

	private:
		/* Tech/inventions only */
		ModifierEffect const* PROPERTY(cb_creation_speed, nullptr);
		ModifierEffect const* PROPERTY(combat_width_additive, nullptr);
		ModifierEffect const* PROPERTY(plurality, nullptr);
		ModifierEffect const* PROPERTY(pop_growth, nullptr);
		ModifierEffect const* PROPERTY(prestige_gain_multiplier, nullptr);
		ModifierEffect const* PROPERTY(regular_experience_level, nullptr);
		ModifierEffect const* PROPERTY(reinforce_rate, nullptr);
		ModifierEffect const* PROPERTY(separatism, nullptr);
		ModifierEffect const* PROPERTY(shared_prestige, nullptr);
		ModifierEffect const* PROPERTY(tax_eff, nullptr);

		/* Country Modifier Effects */
		ModifierEffect const* PROPERTY(administrative_efficiency, nullptr);
		ModifierEffect const* PROPERTY(administrative_efficiency_modifier, nullptr);
		ModifierEffect const* PROPERTY(artisan_input_tech, nullptr);
		ModifierEffect const* PROPERTY(artisan_input_country, nullptr);
		ModifierEffect const* PROPERTY(artisan_output_tech, nullptr);
		ModifierEffect const* PROPERTY(artisan_output_country, nullptr);
		ModifierEffect const* PROPERTY(artisan_throughput_tech, nullptr);
		ModifierEffect const* PROPERTY(artisan_throughput_country, nullptr);
		ModifierEffect const* PROPERTY(badboy, nullptr);
		ModifierEffect const* PROPERTY(cb_generation_speed_modifier, nullptr);
		ModifierEffect const* PROPERTY(civilization_progress_modifier, nullptr);
		ModifierEffect const* PROPERTY(colonial_life_rating, nullptr);
		ModifierEffect const* PROPERTY(colonial_migration, nullptr);
		ModifierEffect const* PROPERTY(colonial_points, nullptr);
		ModifierEffect const* PROPERTY(colonial_prestige, nullptr);
		ModifierEffect const* PROPERTY(core_pop_consciousness_modifier, nullptr);
		ModifierEffect const* PROPERTY(core_pop_militancy_modifier, nullptr);
		ModifierEffect const* PROPERTY(dig_in_cap, nullptr);
		ModifierEffect const* PROPERTY(diplomatic_points, nullptr);
		ModifierEffect const* PROPERTY(diplomatic_points_modifier, nullptr);
		ModifierEffect const* PROPERTY(education_efficiency, nullptr);
		ModifierEffect const* PROPERTY(education_efficiency_modifier, nullptr);
		ModifierEffect const* PROPERTY(factory_cost_country, nullptr);
		ModifierEffect const* PROPERTY(factory_cost_tech, nullptr);
		ModifierEffect const* PROPERTY(factory_input_tech, nullptr);
		ModifierEffect const* PROPERTY(factory_input_country, nullptr);
		ModifierEffect const* PROPERTY(factory_maintenance, nullptr);
		ModifierEffect const* PROPERTY(factory_output_tech, nullptr);
		ModifierEffect const* PROPERTY(factory_output_country, nullptr);
		ModifierEffect const* PROPERTY(factory_owner_cost, nullptr);
		ModifierEffect const* PROPERTY(factory_throughput_tech, nullptr);
		ModifierEffect const* PROPERTY(factory_throughput_country, nullptr);
		ModifierEffect const* PROPERTY(global_assimilation_rate, nullptr);
		ModifierEffect const* PROPERTY(global_immigrant_attract, nullptr);
		ModifierEffect const* PROPERTY(global_pop_consciousness_modifier, nullptr);
		ModifierEffect const* PROPERTY(global_pop_militancy_modifier, nullptr);
		ModifierEffect const* PROPERTY(global_population_growth, nullptr);
		ModifierEffect const* PROPERTY(goods_demand, nullptr);
		ModifierEffect const* PROPERTY(import_cost, nullptr);
		ModifierEffect const* PROPERTY(increase_research, nullptr);
		ModifierEffect const* PROPERTY(influence, nullptr);
		ModifierEffect const* PROPERTY(influence_modifier, nullptr);
		ModifierEffect const* PROPERTY(issue_change_speed, nullptr);
		ModifierEffect const* PROPERTY(land_attack_modifier, nullptr);
		ModifierEffect const* PROPERTY(land_attrition, nullptr);
		ModifierEffect const* PROPERTY(land_defense_modifier, nullptr);
		ModifierEffect const* PROPERTY(land_organisation, nullptr);
		ModifierEffect const* PROPERTY(land_unit_start_experience, nullptr);
		ModifierEffect const* PROPERTY(leadership, nullptr);
		ModifierEffect const* PROPERTY(leadership_modifier, nullptr);
		ModifierEffect const* PROPERTY(literacy_con_impact, nullptr);
		ModifierEffect const* PROPERTY(loan_interest_base, nullptr);
		ModifierEffect const* PROPERTY(loan_interest_foreign, nullptr);
		ModifierEffect const* PROPERTY(max_loan_modifier, nullptr);
		ModifierEffect const* PROPERTY(max_military_spending, nullptr);
		ModifierEffect const* PROPERTY(max_national_focus, nullptr);
		ModifierEffect const* PROPERTY(max_social_spending, nullptr);
		ModifierEffect const* PROPERTY(max_tariff, nullptr);
		ModifierEffect const* PROPERTY(max_tax, nullptr);
		ModifierEffect const* PROPERTY(max_war_exhaustion, nullptr);
		ModifierEffect const* PROPERTY(military_tactics, nullptr);
		ModifierEffect const* PROPERTY(min_military_spending, nullptr);
		ModifierEffect const* PROPERTY(min_social_spending, nullptr);
		ModifierEffect const* PROPERTY(min_tariff, nullptr);
		ModifierEffect const* PROPERTY(min_tax, nullptr);
		ModifierEffect const* PROPERTY(minimum_wage, nullptr);
		ModifierEffect const* PROPERTY(mobilisation_economy_impact_tech, nullptr);
		ModifierEffect const* PROPERTY(mobilisation_economy_impact_country, nullptr);
		ModifierEffect const* PROPERTY(mobilisation_size_tech, nullptr);
		ModifierEffect const* PROPERTY(mobilisation_size_country, nullptr);
		ModifierEffect const* PROPERTY(mobilization_impact, nullptr);
		ModifierEffect const* PROPERTY(morale_global, nullptr);
		ModifierEffect const* PROPERTY(naval_attack_modifier, nullptr);
		ModifierEffect const* PROPERTY(naval_attrition, nullptr);
		ModifierEffect const* PROPERTY(naval_defense_modifier, nullptr);
		ModifierEffect const* PROPERTY(naval_organisation, nullptr);
		ModifierEffect const* PROPERTY(naval_unit_start_experience, nullptr);
		ModifierEffect const* PROPERTY(non_accepted_pop_consciousness_modifier, nullptr);
		ModifierEffect const* PROPERTY(non_accepted_pop_militancy_modifier, nullptr);
		ModifierEffect const* PROPERTY(org_regain, nullptr);
		ModifierEffect const* PROPERTY(pension_level, nullptr);
		ModifierEffect const* PROPERTY(permanent_prestige, nullptr);
		ModifierEffect const* PROPERTY(political_reform_desire, nullptr);
		ModifierEffect const* PROPERTY(poor_savings_modifier, nullptr);
		ModifierEffect const* PROPERTY(prestige_monthly_gain, nullptr);
		ModifierEffect const* PROPERTY(reinforce_speed, nullptr);
		ModifierEffect const* PROPERTY(research_points, nullptr);
		ModifierEffect const* PROPERTY(research_points_modifier, nullptr);
		ModifierEffect const* PROPERTY(research_points_on_conquer, nullptr);
		ModifierEffect const* PROPERTY(rgo_output_tech, nullptr);
		ModifierEffect const* PROPERTY(rgo_output_country, nullptr);
		ModifierEffect const* PROPERTY(rgo_throughput_tech, nullptr);
		ModifierEffect const* PROPERTY(rgo_throughput_country, nullptr);
		ModifierEffect const* PROPERTY(ruling_party_support, nullptr);
		ModifierEffect const* PROPERTY(self_unciv_economic_modifier, nullptr);
		ModifierEffect const* PROPERTY(self_unciv_military_modifier, nullptr);
		ModifierEffect const* PROPERTY(social_reform_desire, nullptr);
		ModifierEffect const* PROPERTY(soldier_to_pop_loss, nullptr);
		ModifierEffect const* PROPERTY(supply_consumption, nullptr);
		ModifierEffect const* PROPERTY(supply_range, nullptr);
		ModifierEffect const* PROPERTY(suppression_points_modifier_tech, nullptr);
		ModifierEffect const* PROPERTY(suppression_points_modifier_country, nullptr);
		ModifierEffect const* PROPERTY(tariff_efficiency_modifier, nullptr);
		ModifierEffect const* PROPERTY(tax_efficiency, nullptr);
		ModifierEffect const* PROPERTY(unemployment_benefit, nullptr);
		ModifierEffect const* PROPERTY(unciv_economic_modifier, nullptr);
		ModifierEffect const* PROPERTY(unciv_military_modifier, nullptr);
		ModifierEffect const* PROPERTY(unit_recruitment_time, nullptr);
		ModifierEffect const* PROPERTY(war_exhaustion_from_battles, nullptr);
		ModifierEffect const* PROPERTY(war_exhaustion_monthly, nullptr);

		/* Province Modifier Effects */
		ModifierEffect const* PROPERTY(assimilation_rate, nullptr);
		ModifierEffect const* PROPERTY(boost_strongest_party, nullptr);
		ModifierEffect const* PROPERTY(farm_rgo_throughput_and_output, nullptr);
		ModifierEffect const* PROPERTY(farm_rgo_output_global, nullptr);
		ModifierEffect const* PROPERTY(farm_rgo_output_local, nullptr);
		ModifierEffect const* PROPERTY(farm_rgo_size_fake, nullptr);
		ModifierEffect const* PROPERTY(farm_rgo_size_global, nullptr);
		ModifierEffect const* PROPERTY(farm_rgo_size_local, nullptr);
		ModifierEffect const* PROPERTY(immigrant_attract, nullptr);
		ModifierEffect const* PROPERTY(immigrant_push, nullptr);
		ModifierEffect const* PROPERTY(life_rating, nullptr);
		ModifierEffect const* PROPERTY(local_artisan_input, nullptr);
		ModifierEffect const* PROPERTY(local_artisan_output, nullptr);
		ModifierEffect const* PROPERTY(local_artisan_throughput, nullptr);
		ModifierEffect const* PROPERTY(local_factory_input, nullptr);
		ModifierEffect const* PROPERTY(local_factory_output, nullptr);
		ModifierEffect const* PROPERTY(local_factory_throughput, nullptr);
		ModifierEffect const* PROPERTY(local_repair, nullptr);
		ModifierEffect const* PROPERTY(local_rgo_output, nullptr);
		ModifierEffect const* PROPERTY(local_rgo_throughput, nullptr);
		ModifierEffect const* PROPERTY(local_ruling_party_support, nullptr);
		ModifierEffect const* PROPERTY(local_ship_build, nullptr);
		ModifierEffect const* PROPERTY(attrition_local, nullptr);
		ModifierEffect const* PROPERTY(max_attrition, nullptr);
		ModifierEffect const* PROPERTY(mine_rgo_throughput_and_output, nullptr);
		ModifierEffect const* PROPERTY(mine_rgo_output_global, nullptr);
		ModifierEffect const* PROPERTY(mine_rgo_output_local, nullptr);
		ModifierEffect const* PROPERTY(mine_rgo_size_fake, nullptr);
		ModifierEffect const* PROPERTY(mine_rgo_size_global, nullptr);
		ModifierEffect const* PROPERTY(mine_rgo_size_local, nullptr);
		ModifierEffect const* PROPERTY(movement_cost_percentage_change, nullptr);
		ModifierEffect const* PROPERTY(number_of_voters, nullptr);
		ModifierEffect const* PROPERTY(pop_consciousness_modifier, nullptr);
		ModifierEffect const* PROPERTY(pop_militancy_modifier, nullptr);
		ModifierEffect const* PROPERTY(population_growth, nullptr);
		ModifierEffect const* PROPERTY(supply_limit_global_percentage_change, nullptr);
		ModifierEffect const* PROPERTY(supply_limit_global_base, nullptr);
		ModifierEffect const* PROPERTY(supply_limit_local_base, nullptr);

		/* Military Modifier Effects */
		ModifierEffect const* PROPERTY(attack_leader, nullptr);
		ModifierEffect const* PROPERTY(attrition_leader, nullptr);
		ModifierEffect const* PROPERTY(defence_leader, nullptr);
		ModifierEffect const* PROPERTY(experience, nullptr);
		ModifierEffect const* PROPERTY(morale_leader, nullptr);
		ModifierEffect const* PROPERTY(organisation, nullptr);
		ModifierEffect const* PROPERTY(reconnaissance, nullptr);
		ModifierEffect const* PROPERTY(reliability, nullptr);
		ModifierEffect const* PROPERTY(speed, nullptr);

		/* BuildingType Effects */
	public:
		struct building_type_effects_t {
			friend struct BuildingTypeManager;

		private:
			ModifierEffect const* PROPERTY(min_level, nullptr);
			ModifierEffect const* PROPERTY(max_level, nullptr);

		public:
			constexpr building_type_effects_t() {};
		};

	private:
		OV_IFLATMAP_PROPERTY(BuildingType, building_type_effects_t, building_type_effects);

		/* GoodDefinition Effects */
	public:
		struct good_effects_t {
			friend struct GoodDefinitionManager;

		private:
			ModifierEffect const* PROPERTY(artisan_goods_input, nullptr);
			ModifierEffect const* PROPERTY(artisan_goods_output, nullptr);
			ModifierEffect const* PROPERTY(artisan_goods_throughput, nullptr);
			ModifierEffect const* PROPERTY(factory_goods_input, nullptr);
			ModifierEffect const* PROPERTY(factory_goods_output, nullptr);
			ModifierEffect const* PROPERTY(factory_goods_throughput, nullptr);
			ModifierEffect const* PROPERTY(rgo_goods_input, nullptr);
			ModifierEffect const* PROPERTY(rgo_goods_output, nullptr);
			ModifierEffect const* PROPERTY(rgo_goods_throughput, nullptr);
			ModifierEffect const* PROPERTY(rgo_size, nullptr);

		public:
			constexpr good_effects_t() {};
		};

	private:
		OV_IFLATMAP_PROPERTY(GoodDefinition, good_effects_t, good_effects);

		/* UnitType Effects */
	public:
		struct unit_type_effects_t {
			friend struct UnitTypeManager;

		private:
			ModifierEffect const* PROPERTY(attack, nullptr);
			ModifierEffect const* PROPERTY(defence, nullptr);
			ModifierEffect const* PROPERTY(default_organisation, nullptr);
			ModifierEffect const* PROPERTY(maximum_speed, nullptr);
			ModifierEffect const* PROPERTY(build_time, nullptr);
			ModifierEffect const* PROPERTY(supply_consumption, nullptr);

		protected:
			constexpr unit_type_effects_t() {};
		};

		struct regiment_type_effects_t : unit_type_effects_t {
			friend struct UnitTypeManager;

		private:
			ModifierEffect const* PROPERTY(reconnaissance, nullptr);
			ModifierEffect const* PROPERTY(discipline, nullptr);
			ModifierEffect const* PROPERTY(support, nullptr);
			ModifierEffect const* PROPERTY(maneuver, nullptr);
			ModifierEffect const* PROPERTY(siege, nullptr);

		public:
			constexpr regiment_type_effects_t() {};
		};

		struct ship_type_effects_t : unit_type_effects_t {
			friend struct UnitTypeManager;

		private:
			ModifierEffect const* PROPERTY(colonial_points, nullptr);
			ModifierEffect const* PROPERTY(supply_consumption_score, nullptr);
			ModifierEffect const* PROPERTY(hull, nullptr);
			ModifierEffect const* PROPERTY(gun_power, nullptr);
			ModifierEffect const* PROPERTY(fire_range, nullptr);
			ModifierEffect const* PROPERTY(evasion, nullptr);
			ModifierEffect const* PROPERTY(torpedo_attack, nullptr);

		public:
			constexpr ship_type_effects_t() {};
		};

	private:
		regiment_type_effects_t PROPERTY(army_base_effects);
		OV_IFLATMAP_PROPERTY(RegimentType, regiment_type_effects_t, regiment_type_effects);
		ship_type_effects_t PROPERTY(navy_base_effects);
		OV_IFLATMAP_PROPERTY(ShipType, ship_type_effects_t, ship_type_effects);

		/* Unit terrain Effects */
	public:
		struct unit_terrain_effects_t {
			friend struct TerrainTypeManager;

		private:
			ModifierEffect const* PROPERTY(attack, nullptr);
			ModifierEffect const* PROPERTY(defence, nullptr);
			ModifierEffect const* PROPERTY(attrition, nullptr);
			ModifierEffect const* PROPERTY(movement, nullptr);

		public:
			constexpr unit_terrain_effects_t() {};
		};

	private:
		OV_IFLATMAP_PROPERTY(TerrainType, unit_terrain_effects_t, unit_terrain_effects);

		/* Rebel Effects */
		ModifierEffect const* PROPERTY(rebel_org_gain_all, nullptr);
		OV_IFLATMAP_PROPERTY(RebelType, ModifierEffect const*, rebel_org_gain_effects);

		/* Pop Effects */
	public:
		struct strata_effects_t {
			friend struct PopManager;

		private:
			ModifierEffect const* PROPERTY(income_modifier, nullptr);
			ModifierEffect const* PROPERTY(vote, nullptr);
			ModifierEffect const* PROPERTY(life_needs, nullptr);
			ModifierEffect const* PROPERTY(everyday_needs, nullptr);
			ModifierEffect const* PROPERTY(luxury_needs, nullptr);

		public:
			constexpr strata_effects_t() {};
		};

	private:
		OV_IFLATMAP_PROPERTY(Strata, strata_effects_t, strata_effects);

		/* Technology Effects */
		OV_IFLATMAP_PROPERTY(TechnologyFolder, ModifierEffect const*, research_bonus_effects);
		constexpr ModifierEffectCache() :
			building_type_effects { decltype(building_type_effects)::create_empty() },
			good_effects { decltype(good_effects)::create_empty() },
			regiment_type_effects { decltype(regiment_type_effects)::create_empty() },
			ship_type_effects { decltype(ship_type_effects)::create_empty() },
			unit_terrain_effects { decltype(unit_terrain_effects)::create_empty() },
			rebel_org_gain_effects { decltype(rebel_org_gain_effects)::create_empty() },
			strata_effects { decltype(strata_effects)::create_empty() },
			research_bonus_effects { decltype(research_bonus_effects)::create_empty() }
			{}

	public:
		ModifierEffectCache(ModifierEffectCache&&) = default;
	};
}