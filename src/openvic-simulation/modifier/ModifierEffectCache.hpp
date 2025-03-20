#pragma once

#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
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
		ModifierEffect const* PROPERTY(cb_creation_speed);
		ModifierEffect const* PROPERTY(combat_width_additive);
		ModifierEffect const* PROPERTY(plurality);
		ModifierEffect const* PROPERTY(pop_growth);
		ModifierEffect const* PROPERTY(prestige_gain_multiplier);
		ModifierEffect const* PROPERTY(regular_experience_level);
		ModifierEffect const* PROPERTY(reinforce_rate);
		ModifierEffect const* PROPERTY(separatism);
		ModifierEffect const* PROPERTY(shared_prestige);
		ModifierEffect const* PROPERTY(tax_eff);

		/* Country Modifier Effects */
		ModifierEffect const* PROPERTY(administrative_efficiency);
		ModifierEffect const* PROPERTY(administrative_efficiency_modifier);
		ModifierEffect const* PROPERTY(artisan_input);
		ModifierEffect const* PROPERTY(artisan_output);
		ModifierEffect const* PROPERTY(artisan_throughput);
		ModifierEffect const* PROPERTY(badboy);
		ModifierEffect const* PROPERTY(cb_generation_speed_modifier);
		ModifierEffect const* PROPERTY(civilization_progress_modifier);
		ModifierEffect const* PROPERTY(colonial_life_rating);
		ModifierEffect const* PROPERTY(colonial_migration);
		ModifierEffect const* PROPERTY(colonial_points);
		ModifierEffect const* PROPERTY(colonial_prestige);
		ModifierEffect const* PROPERTY(core_pop_consciousness_modifier);
		ModifierEffect const* PROPERTY(core_pop_militancy_modifier);
		ModifierEffect const* PROPERTY(dig_in_cap);
		ModifierEffect const* PROPERTY(diplomatic_points);
		ModifierEffect const* PROPERTY(diplomatic_points_modifier);
		ModifierEffect const* PROPERTY(education_efficiency);
		ModifierEffect const* PROPERTY(education_efficiency_modifier);
		ModifierEffect const* PROPERTY(factory_cost);
		ModifierEffect const* PROPERTY(factory_input);
		ModifierEffect const* PROPERTY(factory_maintenance);
		ModifierEffect const* PROPERTY(factory_output);
		ModifierEffect const* PROPERTY(factory_owner_cost);
		ModifierEffect const* PROPERTY(factory_throughput);
		ModifierEffect const* PROPERTY(global_assimilation_rate);
		ModifierEffect const* PROPERTY(global_immigrant_attract);
		ModifierEffect const* PROPERTY(global_pop_consciousness_modifier);
		ModifierEffect const* PROPERTY(global_pop_militancy_modifier);
		ModifierEffect const* PROPERTY(global_population_growth);
		ModifierEffect const* PROPERTY(goods_demand);
		ModifierEffect const* PROPERTY(import_cost);
		ModifierEffect const* PROPERTY(increase_research);
		ModifierEffect const* PROPERTY(influence);
		ModifierEffect const* PROPERTY(influence_modifier);
		ModifierEffect const* PROPERTY(issue_change_speed);
		ModifierEffect const* PROPERTY(land_attack_modifier);
		ModifierEffect const* PROPERTY(land_attrition);
		ModifierEffect const* PROPERTY(land_defense_modifier);
		ModifierEffect const* PROPERTY(land_organisation);
		ModifierEffect const* PROPERTY(land_unit_start_experience);
		ModifierEffect const* PROPERTY(leadership);
		ModifierEffect const* PROPERTY(leadership_modifier);
		ModifierEffect const* PROPERTY(literacy_con_impact);
		ModifierEffect const* PROPERTY(loan_interest_base);
		ModifierEffect const* PROPERTY(loan_interest_foreign);
		ModifierEffect const* PROPERTY(max_loan_modifier);
		ModifierEffect const* PROPERTY(max_military_spending);
		ModifierEffect const* PROPERTY(max_national_focus);
		ModifierEffect const* PROPERTY(max_social_spending);
		ModifierEffect const* PROPERTY(max_tariff);
		ModifierEffect const* PROPERTY(max_tax);
		ModifierEffect const* PROPERTY(max_war_exhaustion);
		ModifierEffect const* PROPERTY(military_tactics);
		ModifierEffect const* PROPERTY(min_military_spending);
		ModifierEffect const* PROPERTY(min_social_spending);
		ModifierEffect const* PROPERTY(min_tariff);
		ModifierEffect const* PROPERTY(min_tax);
		ModifierEffect const* PROPERTY(minimum_wage);
		ModifierEffect const* PROPERTY(mobilisation_economy_impact);
		ModifierEffect const* PROPERTY(mobilisation_size);
		ModifierEffect const* PROPERTY(mobilization_impact);
		ModifierEffect const* PROPERTY(morale_global);
		ModifierEffect const* PROPERTY(naval_attack_modifier);
		ModifierEffect const* PROPERTY(naval_attrition);
		ModifierEffect const* PROPERTY(naval_defense_modifier);
		ModifierEffect const* PROPERTY(naval_organisation);
		ModifierEffect const* PROPERTY(naval_unit_start_experience);
		ModifierEffect const* PROPERTY(non_accepted_pop_consciousness_modifier);
		ModifierEffect const* PROPERTY(non_accepted_pop_militancy_modifier);
		ModifierEffect const* PROPERTY(org_regain);
		ModifierEffect const* PROPERTY(pension_level);
		ModifierEffect const* PROPERTY(permanent_prestige);
		ModifierEffect const* PROPERTY(political_reform_desire);
		ModifierEffect const* PROPERTY(poor_savings_modifier);
		ModifierEffect const* PROPERTY(prestige_monthly_gain);
		ModifierEffect const* PROPERTY(reinforce_speed);
		ModifierEffect const* PROPERTY(research_points);
		ModifierEffect const* PROPERTY(research_points_modifier);
		ModifierEffect const* PROPERTY(research_points_on_conquer);
		ModifierEffect const* PROPERTY(rgo_output);
		ModifierEffect const* PROPERTY(rgo_throughput);
		ModifierEffect const* PROPERTY(ruling_party_support);
		ModifierEffect const* PROPERTY(self_unciv_economic_modifier);
		ModifierEffect const* PROPERTY(self_unciv_military_modifier);
		ModifierEffect const* PROPERTY(social_reform_desire);
		ModifierEffect const* PROPERTY(soldier_to_pop_loss);
		ModifierEffect const* PROPERTY(supply_consumption);
		ModifierEffect const* PROPERTY(supply_range);
		ModifierEffect const* PROPERTY(suppression_points_modifier);
		ModifierEffect const* PROPERTY(tariff_efficiency_modifier);
		ModifierEffect const* PROPERTY(tax_efficiency);
		ModifierEffect const* PROPERTY(unemployment_benefit);
		ModifierEffect const* PROPERTY(unciv_economic_modifier);
		ModifierEffect const* PROPERTY(unciv_military_modifier);
		ModifierEffect const* PROPERTY(unit_recruitment_time);
		ModifierEffect const* PROPERTY(war_exhaustion_from_battles);
		ModifierEffect const* PROPERTY(war_exhaustion_monthly);

		/* Province Modifier Effects */
		ModifierEffect const* PROPERTY(assimilation_rate);
		ModifierEffect const* PROPERTY(boost_strongest_party);
		ModifierEffect const* PROPERTY(combat_width_percentage_change);
		ModifierEffect const* PROPERTY(defence_terrain);
		ModifierEffect const* PROPERTY(farm_rgo_throughput_and_output);
		ModifierEffect const* PROPERTY(farm_rgo_output_global);
		ModifierEffect const* PROPERTY(farm_rgo_output_local);
		ModifierEffect const* PROPERTY(farm_rgo_size_fake);
		ModifierEffect const* PROPERTY(farm_rgo_size_global);
		ModifierEffect const* PROPERTY(farm_rgo_size_local);
		ModifierEffect const* PROPERTY(immigrant_attract);
		ModifierEffect const* PROPERTY(immigrant_push);
		ModifierEffect const* PROPERTY(life_rating);
		ModifierEffect const* PROPERTY(local_artisan_input);
		ModifierEffect const* PROPERTY(local_artisan_output);
		ModifierEffect const* PROPERTY(local_artisan_throughput);
		ModifierEffect const* PROPERTY(local_factory_input);
		ModifierEffect const* PROPERTY(local_factory_output);
		ModifierEffect const* PROPERTY(local_factory_throughput);
		ModifierEffect const* PROPERTY(local_repair);
		ModifierEffect const* PROPERTY(local_rgo_output);
		ModifierEffect const* PROPERTY(local_rgo_throughput);
		ModifierEffect const* PROPERTY(local_ruling_party_support);
		ModifierEffect const* PROPERTY(local_ship_build);
		ModifierEffect const* PROPERTY(attrition_local);
		ModifierEffect const* PROPERTY(max_attrition);
		ModifierEffect const* PROPERTY(mine_rgo_throughput_and_output);
		ModifierEffect const* PROPERTY(mine_rgo_output_global);
		ModifierEffect const* PROPERTY(mine_rgo_output_local);
		ModifierEffect const* PROPERTY(mine_rgo_size_fake);
		ModifierEffect const* PROPERTY(mine_rgo_size_global);
		ModifierEffect const* PROPERTY(mine_rgo_size_local);
		ModifierEffect const* PROPERTY(movement_cost_percentage_change);
		ModifierEffect const* PROPERTY(number_of_voters);
		ModifierEffect const* PROPERTY(pop_consciousness_modifier);
		ModifierEffect const* PROPERTY(pop_militancy_modifier);
		ModifierEffect const* PROPERTY(population_growth);
		ModifierEffect const* PROPERTY(supply_limit_global_percentage_change);
		ModifierEffect const* PROPERTY(supply_limit_global_base);
		ModifierEffect const* PROPERTY(supply_limit_local_base);

		/* Military Modifier Effects */
		ModifierEffect const* PROPERTY(attack_leader);
		ModifierEffect const* PROPERTY(attrition_leader);
		ModifierEffect const* PROPERTY(defence_leader);
		ModifierEffect const* PROPERTY(experience);
		ModifierEffect const* PROPERTY(morale_leader);
		ModifierEffect const* PROPERTY(organisation);
		ModifierEffect const* PROPERTY(reconnaissance);
		ModifierEffect const* PROPERTY(reliability);
		ModifierEffect const* PROPERTY(speed);

		/* BuildingType Effects */
	public:
		struct building_type_effects_t {
			friend struct BuildingTypeManager;

		private:
			ModifierEffect const* PROPERTY(min_level);
			ModifierEffect const* PROPERTY(max_level);

		public:
			building_type_effects_t();
		};

	private:
		IndexedMap<BuildingType, building_type_effects_t> PROPERTY(building_type_effects);

		/* GoodDefinition Effects */
	public:
		struct good_effects_t {
			friend struct GoodDefinitionManager;

		private:
			ModifierEffect const* PROPERTY(artisan_goods_input);
			ModifierEffect const* PROPERTY(artisan_goods_output);
			ModifierEffect const* PROPERTY(artisan_goods_throughput);
			ModifierEffect const* PROPERTY(factory_goods_input);
			ModifierEffect const* PROPERTY(factory_goods_output);
			ModifierEffect const* PROPERTY(factory_goods_throughput);
			ModifierEffect const* PROPERTY(rgo_goods_output);
			ModifierEffect const* PROPERTY(rgo_goods_throughput);
			ModifierEffect const* PROPERTY(rgo_size);

		public:
			good_effects_t();
		};

	private:
		IndexedMap<GoodDefinition, good_effects_t> PROPERTY(good_effects);

		/* UnitType Effects */
	public:
		struct unit_type_effects_t {
			friend struct UnitTypeManager;

		private:
			ModifierEffect const* PROPERTY(attack);
			ModifierEffect const* PROPERTY(defence);
			ModifierEffect const* PROPERTY(default_organisation);
			ModifierEffect const* PROPERTY(maximum_speed);
			ModifierEffect const* PROPERTY(build_time);
			ModifierEffect const* PROPERTY(supply_consumption);

		protected:
			unit_type_effects_t();
		};

		struct regiment_type_effects_t : unit_type_effects_t {
			friend struct UnitTypeManager;

		private:
			ModifierEffect const* PROPERTY(reconnaissance);
			ModifierEffect const* PROPERTY(discipline);
			ModifierEffect const* PROPERTY(support);
			ModifierEffect const* PROPERTY(maneuver);
			ModifierEffect const* PROPERTY(siege);

		public:
			regiment_type_effects_t();
		};

		struct ship_type_effects_t : unit_type_effects_t {
			friend struct UnitTypeManager;

		private:
			ModifierEffect const* PROPERTY(colonial_points);
			ModifierEffect const* PROPERTY(supply_consumption_score);
			ModifierEffect const* PROPERTY(hull);
			ModifierEffect const* PROPERTY(gun_power);
			ModifierEffect const* PROPERTY(fire_range);
			ModifierEffect const* PROPERTY(evasion);
			ModifierEffect const* PROPERTY(torpedo_attack);

		public:
			ship_type_effects_t();
		};

	private:
		regiment_type_effects_t PROPERTY(army_base_effects);
		IndexedMap<RegimentType, regiment_type_effects_t> PROPERTY(regiment_type_effects);
		ship_type_effects_t PROPERTY(navy_base_effects);
		IndexedMap<ShipType, ship_type_effects_t> PROPERTY(ship_type_effects);

		/* Unit terrain Effects */
	public:
		struct unit_terrain_effects_t {
			friend struct TerrainTypeManager;

		private:
			ModifierEffect const* PROPERTY(attack);
			ModifierEffect const* PROPERTY(defence);
			ModifierEffect const* PROPERTY(attrition);
			ModifierEffect const* PROPERTY(movement);

		public:
			unit_terrain_effects_t();
		};

	private:
		IndexedMap<TerrainType, unit_terrain_effects_t> PROPERTY(unit_terrain_effects);

		/* Rebel Effects */
		ModifierEffect const* PROPERTY(rebel_org_gain_all);
		IndexedMap<RebelType, ModifierEffect const*> PROPERTY(rebel_org_gain_effects);

		/* Pop Effects */
	public:
		struct strata_effects_t {
			friend struct PopManager;

		private:
			ModifierEffect const* PROPERTY(income_modifier);
			ModifierEffect const* PROPERTY(vote);
			ModifierEffect const* PROPERTY(life_needs);
			ModifierEffect const* PROPERTY(everyday_needs);
			ModifierEffect const* PROPERTY(luxury_needs);

		public:
			strata_effects_t();
		};

	private:
		IndexedMap<Strata, strata_effects_t> PROPERTY(strata_effects);

		/* Technology Effects */
		IndexedMap<TechnologyFolder, ModifierEffect const*> PROPERTY(research_bonus_effects);

		ModifierEffectCache();

	public:
		ModifierEffectCache(ModifierEffectCache&&) = default;
	};
}
