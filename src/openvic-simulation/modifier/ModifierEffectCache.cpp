#include "ModifierEffectCache.hpp"

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/research/Technology.hpp"

using namespace OpenVic;

ModifierEffectCache::building_type_effects_t::building_type_effects_t()
  : min_level { nullptr },
	max_level { nullptr } {}

ModifierEffectCache::good_effects_t::good_effects_t()
  : artisan_goods_input { nullptr },
	artisan_goods_output { nullptr },
	artisan_goods_throughput { nullptr },
	factory_goods_input { nullptr },
	factory_goods_output { nullptr },
	factory_goods_throughput { nullptr },
	rgo_goods_output { nullptr },
	rgo_goods_throughput { nullptr },
	rgo_size { nullptr } {}

ModifierEffectCache::unit_type_effects_t::unit_type_effects_t()
  : attack { nullptr },
	defence { nullptr },
	default_organisation { nullptr },
	maximum_speed { nullptr },
	build_time { nullptr },
	supply_consumption { nullptr } {}

ModifierEffectCache::regiment_type_effects_t::regiment_type_effects_t()
  : unit_type_effects_t {},
	reconnaissance { nullptr },
	discipline { nullptr },
	support { nullptr },
	maneuver { nullptr },
	siege { nullptr } {}

ModifierEffectCache::ship_type_effects_t::ship_type_effects_t()
  : unit_type_effects_t {},
	colonial_points { nullptr },
	supply_consumption_score { nullptr },
	hull { nullptr },
	gun_power { nullptr },
	fire_range { nullptr },
	evasion { nullptr },
	torpedo_attack { nullptr } {}

ModifierEffectCache::unit_terrain_effects_t::unit_terrain_effects_t()
  : attack  { nullptr },
	defence  { nullptr },
	attrition { nullptr },
	movement { nullptr } {}

ModifierEffectCache::strata_effects_t::strata_effects_t()
  : income_modifier { nullptr },
	vote { nullptr },
	life_needs { nullptr },
	everyday_needs { nullptr },
	luxury_needs { nullptr } {}

ModifierEffectCache::ModifierEffectCache()
  : /* Tech/inventions only */
	cb_creation_speed { nullptr },
	combat_width_additive { nullptr },
	plurality { nullptr },
	pop_growth { nullptr },
	prestige_gain_multiplier { nullptr },
	regular_experience_level { nullptr },
	reinforce_rate { nullptr },
	separatism { nullptr },
	shared_prestige { nullptr },
	tax_eff { nullptr },

	/* Country Modifier Effects */
	administrative_efficiency { nullptr },
	administrative_efficiency_modifier { nullptr },
	artisan_input { nullptr },
	artisan_output { nullptr },
	artisan_throughput { nullptr },
	badboy { nullptr },
	cb_generation_speed_modifier { nullptr },
	civilization_progress_modifier { nullptr },
	colonial_life_rating { nullptr },
	colonial_migration { nullptr },
	colonial_points { nullptr },
	colonial_prestige { nullptr },
	core_pop_consciousness_modifier { nullptr },
	core_pop_militancy_modifier { nullptr },
	dig_in_cap { nullptr },
	diplomatic_points { nullptr },
	diplomatic_points_modifier { nullptr },
	education_efficiency { nullptr },
	education_efficiency_modifier { nullptr },
	factory_cost { nullptr },
	factory_input { nullptr },
	factory_maintenance { nullptr },
	factory_output { nullptr },
	factory_owner_cost { nullptr },
	factory_throughput { nullptr },
	global_assimilation_rate { nullptr },
	global_immigrant_attract { nullptr },
	global_pop_consciousness_modifier { nullptr },
	global_pop_militancy_modifier { nullptr },
	global_population_growth { nullptr },
	goods_demand { nullptr },
	import_cost { nullptr },
	increase_research { nullptr },
	influence { nullptr },
	influence_modifier { nullptr },
	issue_change_speed { nullptr },
	land_attack_modifier { nullptr },
	land_attrition { nullptr },
	land_defense_modifier { nullptr },
	land_organisation { nullptr },
	land_unit_start_experience { nullptr },
	leadership { nullptr },
	leadership_modifier { nullptr },
	literacy_con_impact { nullptr },
	loan_interest_base { nullptr },
	loan_interest_foreign { nullptr },
	max_loan_modifier { nullptr },
	max_military_spending { nullptr },
	max_national_focus { nullptr },
	max_social_spending { nullptr },
	max_tariff { nullptr },
	max_tax { nullptr },
	max_war_exhaustion { nullptr },
	military_tactics { nullptr },
	min_military_spending { nullptr },
	min_social_spending { nullptr },
	min_tariff { nullptr },
	min_tax { nullptr },
	minimum_wage { nullptr },
	mobilisation_economy_impact { nullptr },
	mobilisation_size { nullptr },
	mobilization_impact { nullptr },
	naval_attack_modifier { nullptr },
	naval_attrition { nullptr },
	naval_defense_modifier { nullptr },
	naval_organisation { nullptr },
	naval_unit_start_experience { nullptr },
	non_accepted_pop_consciousness_modifier { nullptr },
	non_accepted_pop_militancy_modifier { nullptr },
	org_regain { nullptr },
	pension_level { nullptr },
	permanent_prestige { nullptr },
	political_reform_desire { nullptr },
	poor_savings_modifier { nullptr },
	prestige_monthly_gain { nullptr },
	reinforce_speed { nullptr },
	research_points { nullptr },
	research_points_modifier { nullptr },
	research_points_on_conquer { nullptr },
	rgo_output { nullptr },
	rgo_throughput { nullptr },
	ruling_party_support { nullptr },
	self_unciv_economic_modifier { nullptr },
	self_unciv_military_modifier { nullptr },
	social_reform_desire { nullptr },
	soldier_to_pop_loss { nullptr },
	supply_consumption { nullptr },
	supply_range { nullptr },
	suppression_points_modifier { nullptr },
	tariff_efficiency_modifier { nullptr },
	tax_efficiency { nullptr },
	unemployment_benefit { nullptr },
	unciv_economic_modifier { nullptr },
	unciv_military_modifier { nullptr },
	unit_recruitment_time { nullptr },
	war_exhaustion { nullptr },

	/* Province Modifier Effects */
	assimilation_rate { nullptr },
	boost_strongest_party { nullptr },
	combat_width_percentage_change { nullptr },
	defence_terrain { nullptr },
	farm_rgo_throughput_and_output { nullptr },
	farm_rgo_output_global { nullptr },
	farm_rgo_output_local { nullptr },
	farm_rgo_size_fake { nullptr },
	farm_rgo_size_global { nullptr },
	farm_rgo_size_local { nullptr },
	immigrant_attract { nullptr },
	immigrant_push { nullptr },
	life_rating { nullptr },
	local_artisan_input { nullptr },
	local_artisan_output { nullptr },
	local_artisan_throughput { nullptr },
	local_factory_input { nullptr },
	local_factory_output { nullptr },
	local_factory_throughput { nullptr },
	local_repair { nullptr },
	local_rgo_output { nullptr },
	local_rgo_throughput { nullptr },
	local_ruling_party_support { nullptr },
	local_ship_build { nullptr },
	attrition_local { nullptr },
	max_attrition { nullptr },
	mine_rgo_throughput_and_output { nullptr },
	mine_rgo_output_global { nullptr },
	mine_rgo_output_local { nullptr },
	mine_rgo_size_fake { nullptr },
	mine_rgo_size_global { nullptr },
	mine_rgo_size_local { nullptr },
	movement_cost_percentage_change { nullptr },
	number_of_voters { nullptr },
	pop_consciousness_modifier { nullptr },
	pop_militancy_modifier { nullptr },
	population_growth { nullptr },
	supply_limit_global_percentage_change { nullptr },
	supply_limit_global_base { nullptr },
	supply_limit_local_base { nullptr },

	/* Military Modifier Effects */
	attack_leader { nullptr },
	attrition_leader { nullptr },
	defence_leader { nullptr },
	experience { nullptr },
	morale_global { nullptr },
	morale_leader { nullptr },
	organisation { nullptr },
	reconnaissance { nullptr },
	reliability { nullptr },
	speed { nullptr },

	/* BuildingType Effects */
	building_type_effects { nullptr },

	/* GoodDefinition Effects */
	good_effects { nullptr },

	/* UnitType Effects */
	army_base_effects {},
	regiment_type_effects { nullptr },
	navy_base_effects {},
	ship_type_effects { nullptr },
	unit_terrain_effects { nullptr },

	/* Rebel Effects */
	rebel_org_gain_all { nullptr },
	rebel_org_gain_effects { nullptr },

	/* Pop Effects */
	strata_effects { nullptr },

	/* Technology Effects */
	research_bonus_effects { nullptr } {}
