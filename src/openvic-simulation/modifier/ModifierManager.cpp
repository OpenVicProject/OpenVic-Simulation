#include "ModifierManager.hpp"

#include <utility>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/modifier/ModifierEffect.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using namespace OpenVic::StringUtils;

using enum ModifierEffect::target_t;

void ModifierManager::lock_all_modifier_except_base_country_effects() {
	lock_leader_modifier_effects();
	lock_unit_terrain_modifier_effects();
	lock_shared_tech_country_modifier_effects();
	lock_technology_modifier_effects();
	lock_base_province_modifier_effects();
	lock_terrain_modifier_effects();
}

bool ModifierManager::_register_modifier_effect(
	modifier_effect_registry_t& registry,
	ModifierEffect::target_t targets,
	ModifierEffect const*& effect_cache,
	const std::string_view identifier,
	const bool is_positive_good,
	const ModifierEffect::format_t format,
	const std::string_view localisation_key,
	const bool has_no_effect
) {
	using enum ModifierEffect::target_t;

	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}

	if (targets == NO_TARGETS) {
		Logger::error("Invalid targets for modifier effect \"", identifier, "\" - none!");
		return false;
	}

	if (!NumberUtils::is_power_of_two(static_cast<uint64_t>(targets))) {
		Logger::error(
			"Invalid targets for modifier effect \"", identifier, "\" - ", ModifierEffect::target_to_string(targets),
			" (can only contain one target)"
		);
		return false;
	}

	if (effect_cache != nullptr) {
		Logger::error(
			"Cache variable for modifier effect \"", identifier, "\" is already filled with modifier effect \"",
			effect_cache->get_identifier(), "\""
		);
		return false;
	}

	const bool ret = registry.add_item({
		std::move(identifier), is_positive_good, format, targets, localisation_key, has_no_effect
	});

	if (ret) {
		effect_cache = &registry.get_items().back();
	}

	return ret;
}

#define REGISTER_MODIFIER_EFFECT(MAPPING_TYPE, TARGETS) \
bool ModifierManager::register_##MAPPING_TYPE##_modifier_effect( \
	ModifierEffect const*& effect_cache, \
	const std::string_view identifier, \
	const bool is_positive_good, \
	const ModifierEffect::format_t format, \
	const std::string_view localisation_key, \
	const bool has_no_effect \
) { \
	return _register_modifier_effect( \
		MAPPING_TYPE##_modifier_effects,  \
		TARGETS, \
		effect_cache,  \
		std::move(identifier), \
		is_positive_good,  \
		format,  \
		localisation_key, \
		has_no_effect \
	); \
}

REGISTER_MODIFIER_EFFECT(leader, UNIT)
REGISTER_MODIFIER_EFFECT(unit_terrain, COUNTRY)
REGISTER_MODIFIER_EFFECT(shared_tech_country, COUNTRY)
REGISTER_MODIFIER_EFFECT(technology, COUNTRY)
REGISTER_MODIFIER_EFFECT(base_country, COUNTRY)
REGISTER_MODIFIER_EFFECT(base_province, PROVINCE)
REGISTER_MODIFIER_EFFECT(terrain, PROVINCE)

#undef REGISTER_MODIFIER_EFFECT

bool ModifierManager::setup_modifier_effects() {
	constexpr bool has_no_effect = true;
	bool ret = true;

	using enum ModifierEffect::format_t;

	/* Tech/inventions only */
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.cb_creation_speed, "cb_creation_speed", true, PROPORTION_DECIMAL, "CB_MANUFACTURE_TECH"
	);
	// When applied to countries (army tech/inventions), combat_width is an additive integer value.
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.combat_width_additive, "combat_width", false, INT
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.plurality, "plurality", true, PERCENTAGE_DECIMAL, "TECH_PLURALITY"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.pop_growth, "pop_growth", true, PROPORTION_DECIMAL, "TECH_POP_GROWTH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.prestige_gain_multiplier, "prestige", true, PROPORTION_DECIMAL,
		"PRESTIGE_MODIFIER_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.regular_experience_level, "regular_experience_level", true, RAW_DECIMAL,
		"REGULAR_EXP_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.reinforce_rate, "reinforce_rate", true, PROPORTION_DECIMAL, "REINFORCE_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.separatism, "seperatism", // paradox typo
		false, PROPORTION_DECIMAL, "SEPARATISM_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.shared_prestige, "shared_prestige", true, RAW_DECIMAL, "SHARED_PRESTIGE_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.tax_eff, "tax_eff", true, PERCENTAGE_DECIMAL, "TECH_TAX_EFF"
	);

	/* Country Modifier Effects */
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.administrative_efficiency, "administrative_efficiency", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.administrative_efficiency_modifier, "administrative_efficiency_modifier", true,
		PROPORTION_DECIMAL, ModifierEffect::make_default_modifier_effect_localisation_key("administrative_efficiency")
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.artisan_input, "artisan_input", false, PROPORTION_DECIMAL, {}, has_no_effect
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.artisan_output, "artisan_output", true, PROPORTION_DECIMAL, {}, has_no_effect
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.artisan_throughput, "artisan_throughput", true, PROPORTION_DECIMAL, {}, has_no_effect
	);
	ret &= register_base_country_modifier_effect(modifier_effect_cache.badboy, "badboy", false, RAW_DECIMAL);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.cb_generation_speed_modifier, "cb_generation_speed_modifier", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.civilization_progress_modifier, "civilization_progress_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("civilization_progress")
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.colonial_life_rating, "colonial_life_rating", false, INT, "COLONIAL_LIFE_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.colonial_migration, "colonial_migration", true, PROPORTION_DECIMAL,
		"COLONIAL_MIGRATION_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.colonial_points, "colonial_points", true, INT, "COLONIAL_POINTS_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.colonial_prestige, "colonial_prestige", true, PROPORTION_DECIMAL,
		"COLONIAL_PRESTIGE_MODIFIER_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.core_pop_consciousness_modifier, "core_pop_consciousness_modifier", false, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.core_pop_militancy_modifier, "core_pop_militancy_modifier", false, RAW_DECIMAL
	);
	ret &= register_technology_modifier_effect(modifier_effect_cache.dig_in_cap, "dig_in_cap", true, INT, "DIGIN_FROM_TECH");
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.diplomatic_points, "diplomatic_points", true, PROPORTION_DECIMAL,
		"DIPLOMATIC_POINTS_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.diplomatic_points_modifier, "diplomatic_points_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("diplopoints_gain")
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.education_efficiency, "education_efficiency", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.education_efficiency_modifier, "education_efficiency_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("education_efficiency")
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.factory_cost, "factory_cost", false, PROPORTION_DECIMAL
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.factory_input, "factory_input", false, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.factory_maintenance, "factory_maintenance", false, PROPORTION_DECIMAL
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.factory_output, "factory_output", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.factory_owner_cost, "factory_owner_cost", false, PROPORTION_DECIMAL
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.factory_throughput, "factory_throughput", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.global_assimilation_rate, "global_assimilation_rate", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("assimilation_rate")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.global_immigrant_attract, "global_immigrant_attract", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("immigant_attract")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.global_pop_consciousness_modifier, "global_pop_consciousness_modifier", false, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.global_pop_militancy_modifier, "global_pop_militancy_modifier", false, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.global_population_growth, "global_population_growth", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("population_growth")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.goods_demand, "goods_demand", false, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.import_cost, "import_cost", false, PROPORTION_DECIMAL, {}, has_no_effect
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.increase_research, "increase_research", true, PROPORTION_DECIMAL, "INC_RES_TECH"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.influence, "influence", true, PROPORTION_DECIMAL, "TECH_GP_INFLUENCE"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.influence_modifier, "influence_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("greatpower_influence_gain")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.issue_change_speed, "issue_change_speed", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.land_attack_modifier, "land_attack_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("land_attack")
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.land_attrition, "land_attrition", false, PROPORTION_DECIMAL, "LAND_ATTRITION_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.land_defense_modifier, "land_defense_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("land_defense")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.land_organisation, "land_organisation", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.land_unit_start_experience, "land_unit_start_experience", true, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.leadership, "leadership", true, RAW_DECIMAL, "LEADERSHIP"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.leadership_modifier, "leadership_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("global_leadership_modifier")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.literacy_con_impact, "literacy_con_impact", false, PROPORTION_DECIMAL
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.loan_interest_base, "loan_interest", false, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.loan_interest_foreign, "loan_interest", false, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.max_loan_modifier, "max_loan_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("max_loan_amount")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.max_military_spending, "max_military_spending", true, PROPORTION_DECIMAL
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.max_national_focus, "max_national_focus", true, INT, "TECH_MAX_FOCUS"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.max_social_spending, "max_social_spending", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(modifier_effect_cache.max_tariff, "max_tariff", true, PROPORTION_DECIMAL);
	ret &= register_base_country_modifier_effect(modifier_effect_cache.max_tax, "max_tax", true, PROPORTION_DECIMAL);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.max_war_exhaustion, "max_war_exhaustion", true, PERCENTAGE_DECIMAL, "MAX_WAR_EXHAUSTION"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.military_tactics, "military_tactics", true, PROPORTION_DECIMAL, "MIL_TACTICS_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.min_military_spending, "min_military_spending", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.min_social_spending, "min_social_spending", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(modifier_effect_cache.min_tariff, "min_tariff", true, PROPORTION_DECIMAL);
	ret &= register_base_country_modifier_effect(modifier_effect_cache.min_tax, "min_tax", true, PROPORTION_DECIMAL);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.minimum_wage, "minimum_wage", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("minimun_wage")
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.mobilisation_economy_impact, "mobilisation_economy_impact", false, PROPORTION_DECIMAL
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.mobilisation_size, "mobilisation_size", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.mobilization_impact, "mobilization_impact", false, PROPORTION_DECIMAL
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.morale_global, "morale", true, PROPORTION_DECIMAL, "MORALE_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.naval_attack_modifier, "naval_attack_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("naval_attack")
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.naval_attrition, "naval_attrition", false, PROPORTION_DECIMAL, "NAVAL_ATTRITION_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.naval_defense_modifier, "naval_defense_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("naval_defense")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.naval_organisation, "naval_organisation", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.naval_unit_start_experience, "naval_unit_start_experience", true, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.non_accepted_pop_consciousness_modifier, "non_accepted_pop_consciousness_modifier", false,
		RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.non_accepted_pop_militancy_modifier, "non_accepted_pop_militancy_modifier", false, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(modifier_effect_cache.org_regain, "org_regain", true, PROPORTION_DECIMAL);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.pension_level, "pension_level", true, PROPORTION_DECIMAL
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.permanent_prestige, "permanent_prestige", true, RAW_DECIMAL, "PERMANENT_PRESTIGE_TECH"
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.political_reform_desire, "political_reform_desire", false, PROPORTION_DECIMAL
	);

	ret &= register_technology_modifier_effect(
		modifier_effect_cache.poor_savings_modifier, "poor_savings_modifier", true, PROPORTION_DECIMAL, {}, has_no_effect
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.prestige_monthly_gain, "prestige", true, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.reinforce_speed, "reinforce_speed", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(modifier_effect_cache.research_points, "research_points", true, RAW_DECIMAL);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.research_points_modifier, "research_points_modifier", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.research_points_on_conquer, "research_points_on_conquer", true, PROPORTION_DECIMAL
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.rgo_output, "rgo_output", true, PROPORTION_DECIMAL
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.rgo_throughput, "rgo_throughput", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.ruling_party_support, "ruling_party_support", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.self_unciv_economic_modifier, "self_unciv_economic_modifier", false, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("self_unciv_economic")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.self_unciv_military_modifier, "self_unciv_military_modifier", false, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("self_unciv_military")
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.social_reform_desire, "social_reform_desire", false, PROPORTION_DECIMAL
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.soldier_to_pop_loss, "soldier_to_pop_loss", true, PROPORTION_DECIMAL,
		"SOLDIER_TO_POP_LOSS_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.supply_consumption, "supply_consumption", false, PROPORTION_DECIMAL
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.supply_range, "supply_range", true, PROPORTION_DECIMAL, "SUPPLY_RANGE_TECH"
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.suppression_points_modifier, "suppression_points_modifier", true, PROPORTION_DECIMAL,
		"SUPPRESSION_TECH"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.tariff_efficiency_modifier, "tariff_efficiency_modifier", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("tariff_efficiency")
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.tax_efficiency, "tax_efficiency", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.unemployment_benefit, "unemployment_benefit", true, PROPORTION_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.unciv_economic_modifier, "unciv_economic_modifier", false, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("unciv_economic"), has_no_effect
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.unciv_military_modifier, "unciv_military_modifier", false, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("unciv_military"), has_no_effect
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.unit_recruitment_time, "unit_recruitment_time", false, PROPORTION_DECIMAL
	);
	ret &= register_shared_tech_country_modifier_effect(
		modifier_effect_cache.war_exhaustion, "war_exhaustion", false, PROPORTION_DECIMAL, "WAR_EXHAUST_BATTLES"
	);

	/* Province Modifier Effects */
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.assimilation_rate, "assimilation_rate", true, PROPORTION_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.boost_strongest_party, "boost_strongest_party", false, PROPORTION_DECIMAL, {}, has_no_effect
	);
	// When applied to provinces (terrain), combat_width is a multiplicative proportional decimal value.
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.combat_width_percentage_change, "combat_width", false, PROPORTION_DECIMAL
	);
	ret &= register_terrain_modifier_effect(
		modifier_effect_cache.defence_terrain, "defence", true, RAW_DECIMAL, "TRAIT_DEFEND"
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.farm_rgo_throughput_and_output, "farm_rgo_eff", true, PROPORTION_DECIMAL, "TECH_FARM_OUTPUT"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.farm_rgo_output_global, "farm_rgo_eff", true, PROPORTION_DECIMAL, "MODIFIER_FARM_EFFICIENCY"
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.farm_rgo_output_local, "farm_rgo_eff", true, PROPORTION_DECIMAL, "MODIFIER_FARM_EFFICIENCY"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.farm_rgo_size_fake, "farm_rgo_size", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("farm_size"), has_no_effect
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.farm_rgo_size_global, "farm_rgo_size", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("farm_size")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.farm_rgo_size_local, "farm_rgo_size", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("farm_size")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.immigrant_attract, "immigrant_attract", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("immigant_attract")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.immigrant_push, "immigrant_push", false, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("immigant_push")
	);
	ret &= register_base_province_modifier_effect(modifier_effect_cache.life_rating, "life_rating", true, PROPORTION_DECIMAL);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_artisan_input, "local_artisan_input", false, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("artisan_input"),
		has_no_effect
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_artisan_output, "local_artisan_output", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("artisan_output"),
		has_no_effect
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_artisan_throughput, "local_artisan_throughput", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("artisan_throughput"),
		has_no_effect
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_factory_input, "local_factory_input", false, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("factory_input")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_factory_output, "local_factory_output", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("factory_output")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_factory_throughput, "local_factory_throughput", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("factory_throughput")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_repair, "local_repair", true, PROPORTION_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_rgo_output, "local_rgo_output", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("rgo_output")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_rgo_throughput, "local_rgo_throughput", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("rgo_throughput")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_ruling_party_support, "local_ruling_party_support", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("ruling_party_support")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.local_ship_build, "local_ship_build", false, PROPORTION_DECIMAL
	);
	ret &= register_terrain_modifier_effect(
		modifier_effect_cache.attrition_local, "attrition", false, PROPORTION_DECIMAL, "UA_ATTRITION"
	);
	ret &= register_base_province_modifier_effect(modifier_effect_cache.max_attrition, "max_attrition", false, RAW_DECIMAL);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.mine_rgo_throughput_and_output, "mine_rgo_eff", true, PROPORTION_DECIMAL, "TECH_MINE_OUTPUT"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.mine_rgo_output_global, "mine_rgo_eff", true, PROPORTION_DECIMAL, "MODIFIER_MINE_EFFICIENCY"
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.mine_rgo_output_local, "mine_rgo_eff", true, PROPORTION_DECIMAL, "MODIFIER_MINE_EFFICIENCY"
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.mine_rgo_size_fake, "mine_rgo_size", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("mine_size"), has_no_effect
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.mine_rgo_size_global, "mine_rgo_size", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("mine_size")
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.mine_rgo_size_local, "mine_rgo_size", true, PROPORTION_DECIMAL,
		ModifierEffect::make_default_modifier_effect_localisation_key("mine_size")
	);
	ret &= register_terrain_modifier_effect(
		modifier_effect_cache.movement_cost_base, "movement_cost", true, PROPORTION_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.movement_cost_percentage_change, "movement_cost", false, PROPORTION_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.number_of_voters, "number_of_voters", false, PROPORTION_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.pop_consciousness_modifier, "pop_consciousness_modifier", false, RAW_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.pop_militancy_modifier, "pop_militancy_modifier", false, RAW_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.population_growth, "population_growth", true, PROPORTION_DECIMAL
	);
	ret &= register_technology_modifier_effect(
		modifier_effect_cache.supply_limit_global_percentage_change, "supply_limit", true, RAW_DECIMAL
	);
	ret &= register_base_country_modifier_effect(
		modifier_effect_cache.supply_limit_global_base, "supply_limit", true, RAW_DECIMAL
	);
	ret &= register_base_province_modifier_effect(
		modifier_effect_cache.supply_limit_local_base, "supply_limit", true, RAW_DECIMAL
	);

	/* Military Modifier Effects */
	ret &= register_leader_modifier_effect(modifier_effect_cache.attack_leader, "attack", true, RAW_DECIMAL, "TRAIT_ATTACK");
	ret &= register_leader_modifier_effect(
		modifier_effect_cache.attrition_leader, "attrition", false, RAW_DECIMAL, "ATTRITION"
	);
	ret &= register_leader_modifier_effect(modifier_effect_cache.defence_leader, "defence", true, RAW_DECIMAL, "TRAIT_DEFEND");
	ret &= register_leader_modifier_effect(
		modifier_effect_cache.experience, "experience", true, PROPORTION_DECIMAL, "TRAIT_EXPERIENCE"
	);
	ret &= register_leader_modifier_effect(
		modifier_effect_cache.morale_leader, "morale", true, PROPORTION_DECIMAL, "TRAIT_MORALE"
	);
	ret &= register_leader_modifier_effect(
		modifier_effect_cache.organisation, "organisation", true, PROPORTION_DECIMAL, "TRAIT_ORGANISATION"
	);
	ret &= register_leader_modifier_effect(
		modifier_effect_cache.reconnaissance, "reconnaissance", true, PROPORTION_DECIMAL, "TRAIT_RECONAISSANCE"
	);
	ret &= register_leader_modifier_effect(
		modifier_effect_cache.reliability, "reliability", true, PROPORTION_DECIMAL, "TRAIT_RELIABILITY"
	);
	ret &= register_leader_modifier_effect(modifier_effect_cache.speed, "speed", true, PROPORTION_DECIMAL, "TRAIT_SPEED");

	return ret;
}

bool ModifierManager::register_complex_modifier(const std::string_view identifier) {
	if (complex_modifiers.emplace(identifier).second) {
		return true;
	} else {
		Logger::error("Duplicate complex modifier: ", identifier);
		return false;
	}
}

std::string ModifierManager::get_flat_identifier(
	const std::string_view complex_modifier_identifier,
	const std::string_view variant_identifier
) {
	return StringUtils::append_string_views(complex_modifier_identifier, " ", variant_identifier);
}

bool ModifierManager::add_event_modifier(
	const std::string_view identifier,
	ModifierValue&& values,
	const IconModifier::icon_t icon,
	const Modifier::modifier_type_t type
) {

	if (identifier.empty()) {
		Logger::error("Invalid event modifier effect identifier - empty!");
		return false;
	}

	return event_modifiers.add_item(
		{ identifier, std::move(values), type, icon }, duplicate_warning_callback
	);
}

bool ModifierManager::load_event_modifiers(const ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		event_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			ModifierValue modifier_value;
			IconModifier::icon_t icon = 0;

			bool ret = expect_dictionary_keys_and_default(
				expect_base_province_modifier(modifier_value),
				"icon", ZERO_OR_ONE, expect_uint(assign_variable_callback(icon))
			)(value);

			ret &= add_event_modifier(key, std::move(modifier_value), icon);

			return ret;
		}
	)(root);

	return ret;
}

bool ModifierManager::load_static_modifiers(const ast::NodeCPtr root) {
	return static_modifier_cache.load_static_modifiers(*this, root);
}

bool ModifierManager::add_triggered_modifier(
	const std::string_view identifier,
	ModifierValue&& values,
	const IconModifier::icon_t icon,
	ConditionScript&& trigger
) {
	using enum Modifier::modifier_type_t;

	if (identifier.empty()) {
		Logger::error("Invalid triggered modifier effect identifier - empty!");
		return false;
	}

	return triggered_modifiers.add_item(
		{ identifier, std::move(values), TRIGGERED, icon, std::move(trigger) },
		duplicate_warning_callback
	);
}

bool ModifierManager::load_triggered_modifiers(const ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		triggered_modifiers,
		[this](const std::string_view key, const ast::NodeCPtr value) -> bool {
			using enum scope_type_t;

			ModifierValue modifier_value {};
			IconModifier::icon_t icon = 0;
			ConditionScript trigger { COUNTRY, COUNTRY, NO_SCOPE };

			bool ret = expect_dictionary_keys_and_default(
				expect_base_country_modifier(modifier_value),
				"icon", ZERO_OR_ONE, expect_uint(assign_variable_callback(icon)),
				"trigger", ONE_EXACTLY, trigger.expect_script()
			)(value);

			ret &= add_triggered_modifier(key, std::move(modifier_value), icon, std::move(trigger));

			return ret;
		}
	)(root);

	lock_triggered_modifiers();

	return ret;
}

bool ModifierManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	for (TriggeredModifier& modifier : triggered_modifiers.get_items()) {
		ret &= modifier.parse_scripts(definition_manager);
	}

	return ret;
}

bool ModifierManager::_add_flattened_modifier_cb(
	ModifierValue& modifier_value,
	const std::string_view prefix,
	const std::string_view key,
	const ast::NodeCPtr value
) const {
	const std::string flat_identifier = get_flat_identifier(prefix, key);
	ModifierEffect const* effect = technology_modifier_effects.get_item_by_identifier(flat_identifier);
	if (effect != nullptr) {
		return _add_modifier_cb(modifier_value, effect, value);
	} else {
		Logger::error("Could not find flattened modifier: ", flat_identifier);
		return false;
	}
};

bool ModifierManager::_add_modifier_cb(
	ModifierValue& modifier_value,
	ModifierEffect const* const effect,
	const ast::NodeCPtr value
) const {
	if (effect->has_no_effect()) {
		Logger::warning("This modifier does nothing: ", effect->get_identifier());
	}
	return expect_fixed_point(map_callback(modifier_value.values, effect))(value);
}

key_value_callback_t ModifierManager::_expect_modifier_effect(
	modifier_effect_registry_t const& registry,
	ModifierValue& modifier_value
) const {
	return _expect_modifier_effect_with_fallback(registry, modifier_value, key_value_invalid_callback);
}

key_value_callback_t ModifierManager::_expect_modifier_effect_with_fallback(
	modifier_effect_registry_t const& registry,
	ModifierValue& modifier_value,
	const key_value_callback_t fallback
) const {
	return [this, &registry, &modifier_value, fallback](const std::string_view key, const ast::NodeCPtr value) -> bool {
		if (dryad::node_has_kind<ast::ListValue>(value) && complex_modifiers.contains(key)) {
			return expect_dictionary([this, &modifier_value, key](
				const std::string_view inner_key, const ast::NodeCPtr inner_value
			) -> bool {
				return _add_flattened_modifier_cb(modifier_value, key, inner_key, inner_value);
			})(value);
		}

		ModifierEffect const* effect = registry.get_item_by_identifier(key);
		if (effect == nullptr) {
			return fallback(key, value);
		}
		return _add_modifier_cb(modifier_value, effect, value);
	};
}

key_value_callback_t ModifierManager::_expect_shared_tech_country_modifier_effect(ModifierValue& modifier_value) const {
	return _expect_modifier_effect(shared_tech_country_modifier_effects, modifier_value);
}

key_value_callback_t ModifierManager::expect_leader_modifier(ModifierValue& modifier_value) const {
	return _expect_modifier_effect(leader_modifier_effects, modifier_value);
}

key_value_callback_t ModifierManager::expect_technology_modifier(ModifierValue& modifier_value) const {
	return [this, &modifier_value](const std::string_view key, const ast::NodeCPtr value) {
		if (strings_equal_case_insensitive(key, "rebel_org_gain")) { // because of course there's a special one
			std::string_view faction_identifier;
			ast::NodeCPtr value_node = nullptr;

			bool ret = expect_dictionary_keys(
				"faction", ONE_EXACTLY, expect_identifier(assign_variable_callback(faction_identifier)),
				"value", ONE_EXACTLY, assign_variable_callback(value_node)
			)(value);

			ret &= _add_flattened_modifier_cb(modifier_value, key, faction_identifier, value_node);

			return ret;
		}

		return _expect_modifier_effect_with_fallback(
			technology_modifier_effects,
			modifier_value,
			_expect_shared_tech_country_modifier_effect(modifier_value)
		)(key, value);
	};
}

key_value_callback_t ModifierManager::expect_unit_terrain_modifier(
	ModifierValue& modifier_value,
	const std::string_view terrain_type_identifier
) const {
	return [this, &modifier_value, terrain_type_identifier](const std::string_view key, const ast::NodeCPtr value) -> bool {
		const std::string flat_identifier = get_flat_identifier(key, terrain_type_identifier);
		ModifierEffect const* effect = unit_terrain_modifier_effects.get_item_by_identifier(flat_identifier);
		if (effect == nullptr) {
			return key_value_invalid_callback(flat_identifier, value);
		}
		return _add_modifier_cb(modifier_value, effect, value);
	};
}

key_value_callback_t ModifierManager::expect_base_country_modifier(ModifierValue& modifier_value) const {
	return _expect_modifier_effect_with_fallback(
		base_country_modifier_effects,
		modifier_value,
		_expect_shared_tech_country_modifier_effect(modifier_value)
	);
}

key_value_callback_t ModifierManager::expect_base_province_modifier(ModifierValue& modifier_value) const {
	return _expect_modifier_effect_with_fallback(
		base_province_modifier_effects,
		modifier_value,
		expect_base_country_modifier(modifier_value)
	);
}

key_value_callback_t ModifierManager::expect_terrain_modifier(ModifierValue& modifier_value) const {
	return _expect_modifier_effect_with_fallback(
		terrain_modifier_effects,
		modifier_value,
		expect_base_province_modifier(modifier_value)
	);
}
