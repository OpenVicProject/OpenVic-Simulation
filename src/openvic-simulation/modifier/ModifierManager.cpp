#include "ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

bool ModifierManager::add_modifier_effect(
	ModifierEffect const*& effect_cache, std::string_view identifier, bool positive_good, ModifierEffect::format_t format,
	ModifierEffect::target_t targets, std::string_view localisation_key
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

	if (effect_cache != nullptr) {
		Logger::error(
			"Cache variable for modifier effect \"", identifier, "\" is already filled with modifier effect \"",
			effect_cache->get_identifier(), "\""
		);
		return false;
	}

	const bool ret = modifier_effects.add_item({ std::move(identifier), positive_good, format, targets, localisation_key });

	if (ret) {
		effect_cache = &modifier_effects.get_items().back();
	}

	return ret;
}

bool ModifierManager::setup_modifier_effects() {
	bool ret = true;

	using enum ModifierEffect::format_t;
	using enum ModifierEffect::target_t;

	/* Tech/inventions only */
	ret &= add_modifier_effect(
		modifier_effect_cache.cb_creation_speed, "cb_creation_speed", true, PROPORTION_DECIMAL, COUNTRY, "CB_MANUFACTURE_TECH"
	);
	ret &= add_modifier_effect(modifier_effect_cache.combat_width, "combat_width", false, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.plurality, "plurality", true, PERCENTAGE_DECIMAL, COUNTRY, "TECH_PLURALITY"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.pop_growth, "pop_growth", true, PROPORTION_DECIMAL, COUNTRY, "TECH_POP_GROWTH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.regular_experience_level, "regular_experience_level", true, RAW_DECIMAL, COUNTRY,
		"REGULAR_EXP_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.reinforce_rate, "reinforce_rate", true, PROPORTION_DECIMAL, COUNTRY, "REINFORCE_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.separatism, "seperatism", // paradox typo
		false, PROPORTION_DECIMAL, COUNTRY, "SEPARATISM_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.shared_prestige, "shared_prestige", true, RAW_DECIMAL, COUNTRY, "SHARED_PRESTIGE_TECH"
	);
	ret &= add_modifier_effect(modifier_effect_cache.tax_eff, "tax_eff", true, PERCENTAGE_DECIMAL, COUNTRY, "TECH_TAX_EFF");

	/* Country Modifier Effects */
	ret &= add_modifier_effect(
		modifier_effect_cache.administrative_efficiency, "administrative_efficiency", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.administrative_efficiency_modifier, "administrative_efficiency_modifier", true,
		PROPORTION_DECIMAL, COUNTRY, ModifierEffect::make_default_modifier_effect_localisation_key("administrative_efficiency")
	);
	ret &= add_modifier_effect(modifier_effect_cache.artisan_input, "artisan_input", false, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.artisan_output, "artisan_output", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.artisan_throughput, "artisan_throughput", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.badboy, "badboy", false, RAW_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.cb_generation_speed_modifier, "cb_generation_speed_modifier", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.civilization_progress_modifier, "civilization_progress_modifier", true, PROPORTION_DECIMAL,
		COUNTRY, ModifierEffect::make_default_modifier_effect_localisation_key("civilization_progress")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.colonial_life_rating, "colonial_life_rating", false, INT, COUNTRY, "COLONIAL_LIFE_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.colonial_migration, "colonial_migration", true, PROPORTION_DECIMAL, COUNTRY,
		"COLONIAL_MIGRATION_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.colonial_points, "colonial_points", true, INT, COUNTRY, "COLONIAL_POINTS_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.colonial_prestige, "colonial_prestige", true, PROPORTION_DECIMAL, COUNTRY,
		"COLONIAL_PRESTIGE_MODIFIER_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.core_pop_consciousness_modifier, "core_pop_consciousness_modifier", false, RAW_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.core_pop_militancy_modifier, "core_pop_militancy_modifier", false, RAW_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.dig_in_cap, "dig_in_cap", true, INT, COUNTRY, "DIGIN_FROM_TECH");
	ret &= add_modifier_effect(
		modifier_effect_cache.diplomatic_points, "diplomatic_points", true, PROPORTION_DECIMAL, COUNTRY,
		"DIPLOMATIC_POINTS_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.diplomatic_points_modifier, "diplomatic_points_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("diplopoints_gain")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.education_efficiency, "education_efficiency", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.education_efficiency_modifier, "education_efficiency_modifier", true, PROPORTION_DECIMAL,
		COUNTRY, ModifierEffect::make_default_modifier_effect_localisation_key("education_efficiency")
	);
	ret &= add_modifier_effect(modifier_effect_cache.factory_cost, "factory_cost", false, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.factory_input, "factory_input", false, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.factory_maintenance, "factory_maintenance", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.factory_output, "factory_output", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.factory_owner_cost, "factory_owner_cost", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.factory_throughput, "factory_throughput", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.global_assimilation_rate, "global_assimilation_rate", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("assimilation_rate")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.global_immigrant_attract, "global_immigrant_attract", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("immigant_attract")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.global_pop_consciousness_modifier, "global_pop_consciousness_modifier", false, RAW_DECIMAL,
		COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.global_pop_militancy_modifier, "global_pop_militancy_modifier", false, RAW_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.global_population_growth, "global_population_growth", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("population_growth")
	);
	ret &= add_modifier_effect(modifier_effect_cache.goods_demand, "goods_demand", false, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.import_cost, "import_cost", false, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.increase_research, "increase_research", true, PROPORTION_DECIMAL, COUNTRY, "INC_RES_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.influence, "influence", true, PROPORTION_DECIMAL, COUNTRY, "TECH_GP_INFLUENCE"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.influence_modifier, "influence_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("greatpower_influence_gain")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.issue_change_speed, "issue_change_speed", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.land_attack_modifier, "land_attack_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("land_attack")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.land_attrition, "land_attrition", false, PROPORTION_DECIMAL, COUNTRY, "LAND_ATTRITION_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.land_defense_modifier, "land_defense_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("land_defense")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.land_organisation, "land_organisation", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.land_unit_start_experience, "land_unit_start_experience", true, RAW_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.leadership, "leadership", true, RAW_DECIMAL, COUNTRY, "LEADERSHIP");
	ret &= add_modifier_effect(
		modifier_effect_cache.leadership_modifier, "leadership_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("global_leadership_modifier")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.literacy_con_impact, "literacy_con_impact", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.loan_interest, "loan_interest", false, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.max_loan_modifier, "max_loan_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("max_loan_amount")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.max_military_spending, "max_military_spending", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.max_national_focus, "max_national_focus", true, INT, COUNTRY, "TECH_MAX_FOCUS"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.max_social_spending, "max_social_spending", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.max_tariff, "max_tariff", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.max_tax, "max_tax", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.max_war_exhaustion, "max_war_exhaustion", true, PERCENTAGE_DECIMAL, COUNTRY, "MAX_WAR_EXHAUSTION"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.military_tactics, "military_tactics", true, PROPORTION_DECIMAL, COUNTRY, "MIL_TACTICS_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.min_military_spending, "min_military_spending", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.min_social_spending, "min_social_spending", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.min_tariff, "min_tariff", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.min_tax, "min_tax", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.minimum_wage, "minimum_wage", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("minimun_wage")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.mobilisation_economy_impact, "mobilisation_economy_impact", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.mobilisation_size, "mobilisation_size", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.mobilization_impact, "mobilization_impact", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.naval_attack_modifier, "naval_attack_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("naval_attack")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.naval_attrition, "naval_attrition", false, PROPORTION_DECIMAL, COUNTRY, "NAVAL_ATTRITION_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.naval_defense_modifier, "naval_defense_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("naval_defense")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.naval_organisation, "naval_organisation", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.naval_unit_start_experience, "naval_unit_start_experience", true, RAW_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.non_accepted_pop_consciousness_modifier, "non_accepted_pop_consciousness_modifier", false,
		RAW_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.non_accepted_pop_militancy_modifier, "non_accepted_pop_militancy_modifier", false, RAW_DECIMAL,
		COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.org_regain, "org_regain", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.pension_level, "pension_level", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.permanent_prestige, "permanent_prestige", true, RAW_DECIMAL, COUNTRY, "PERMANENT_PRESTIGE_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.political_reform_desire, "political_reform_desire", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.poor_savings_modifier, "poor_savings_modifier", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.prestige, "prestige", true, RAW_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.reinforce_speed, "reinforce_speed", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.research_points, "research_points", true, RAW_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.research_points_modifier, "research_points_modifier", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.research_points_on_conquer, "research_points_on_conquer", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(modifier_effect_cache.rgo_output, "rgo_output", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(modifier_effect_cache.rgo_throughput, "rgo_throughput", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.ruling_party_support, "ruling_party_support", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.self_unciv_economic_modifier, "self_unciv_economic_modifier", false, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("self_unciv_economic")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.self_unciv_military_modifier, "self_unciv_military_modifier", false, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("self_unciv_military")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.social_reform_desire, "social_reform_desire", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.soldier_to_pop_loss, "soldier_to_pop_loss", true, PROPORTION_DECIMAL, COUNTRY,
		"SOLDIER_TO_POP_LOSS_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.supply_consumption, "supply_consumption", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.supply_range, "supply_range", true, PROPORTION_DECIMAL, COUNTRY, "SUPPLY_RANGE_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.suppression_points_modifier, "suppression_points_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		"SUPPRESSION_TECH"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.tariff_efficiency_modifier, "tariff_efficiency_modifier", true, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("tariff_efficiency")
	);
	ret &= add_modifier_effect(modifier_effect_cache.tax_efficiency, "tax_efficiency", true, PROPORTION_DECIMAL, COUNTRY);
	ret &= add_modifier_effect(
		modifier_effect_cache.unemployment_benefit, "unemployment_benefit", true, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.unciv_economic_modifier, "unciv_economic_modifier", false, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("unciv_economic")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.unciv_military_modifier, "unciv_military_modifier", false, PROPORTION_DECIMAL, COUNTRY,
		ModifierEffect::make_default_modifier_effect_localisation_key("unciv_military")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.unit_recruitment_time, "unit_recruitment_time", false, PROPORTION_DECIMAL, COUNTRY
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.war_exhaustion, "war_exhaustion", false, PROPORTION_DECIMAL, COUNTRY, "WAR_EXHAUST_BATTLES"
	);

	/* Province Modifier Effects */
	ret &= add_modifier_effect(
		modifier_effect_cache.assimilation_rate, "assimilation_rate", true, PROPORTION_DECIMAL, PROVINCE
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.boost_strongest_party, "boost_strongest_party", false, PROPORTION_DECIMAL, PROVINCE
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.farm_rgo_eff, "farm_rgo_eff", true, PROPORTION_DECIMAL, PROVINCE, "TECH_FARM_OUTPUT"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.farm_rgo_size, "farm_rgo_size", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("farm_size")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.immigrant_attract, "immigrant_attract", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("immigant_attract")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.immigrant_push, "immigrant_push", false, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("immigant_push")
	);
	ret &= add_modifier_effect(modifier_effect_cache.life_rating, "life_rating", true, PROPORTION_DECIMAL, PROVINCE);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_artisan_input, "local_artisan_input", false, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("artisan_input")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_artisan_output, "local_artisan_output", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("artisan_output")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_artisan_throughput, "local_artisan_throughput", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("artisan_throughput")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_factory_input, "local_factory_input", false, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("factory_input")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_factory_output, "local_factory_output", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("factory_output")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_factory_throughput, "local_factory_throughput", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("factory_throughput")
	);
	ret &= add_modifier_effect(modifier_effect_cache.local_repair, "local_repair", true, PROPORTION_DECIMAL, PROVINCE);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_rgo_output, "local_rgo_output", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("rgo_output")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_rgo_throughput, "local_rgo_throughput", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("rgo_throughput")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_ruling_party_support, "local_ruling_party_support", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("ruling_party_support")
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.local_ship_build, "local_ship_build", false, PROPORTION_DECIMAL, PROVINCE
	);
	ret &= add_modifier_effect(modifier_effect_cache.max_attrition, "max_attrition", false, RAW_DECIMAL, PROVINCE);
	ret &= add_modifier_effect(
		modifier_effect_cache.mine_rgo_eff, "mine_rgo_eff", true, PROPORTION_DECIMAL, PROVINCE, "TECH_MINE_OUTPUT"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.mine_rgo_size, "mine_rgo_size", true, PROPORTION_DECIMAL, PROVINCE,
		ModifierEffect::make_default_modifier_effect_localisation_key("mine_size")
	);
	ret &= add_modifier_effect(modifier_effect_cache.movement_cost, "movement_cost", false, PROPORTION_DECIMAL, PROVINCE);
	ret &= add_modifier_effect(
		modifier_effect_cache.number_of_voters, "number_of_voters", false, PROPORTION_DECIMAL, PROVINCE
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.pop_consciousness_modifier, "pop_consciousness_modifier", false, RAW_DECIMAL, PROVINCE
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.pop_militancy_modifier, "pop_militancy_modifier", false, RAW_DECIMAL, PROVINCE
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.population_growth, "population_growth", true, PROPORTION_DECIMAL, PROVINCE
	);
	ret &= add_modifier_effect(modifier_effect_cache.supply_limit, "supply_limit", true, RAW_DECIMAL, PROVINCE);

	/* Military Modifier Effects */
	ret &= add_modifier_effect(modifier_effect_cache.attack, "attack", true, INT, UNIT, "TRAIT_ATTACK");
	ret &= add_modifier_effect(modifier_effect_cache.attrition, "attrition", false, RAW_DECIMAL, UNIT, "ATTRITION");
	ret &= add_modifier_effect(modifier_effect_cache.defence, "defence", true, INT, UNIT, "TRAIT_DEFEND");
	ret &= add_modifier_effect(
		modifier_effect_cache.experience, "experience", true, PROPORTION_DECIMAL, UNIT, "TRAIT_EXPERIENCE"
	);
	ret &= add_modifier_effect(modifier_effect_cache.morale, "morale", true, PROPORTION_DECIMAL, UNIT, "TRAIT_MORALE");
	ret &= add_modifier_effect(
		modifier_effect_cache.organisation, "organisation", true, PROPORTION_DECIMAL, UNIT, "TRAIT_ORGANISATION"
	);
	ret &= add_modifier_effect(
		modifier_effect_cache.reconnaissance, "reconnaissance", true, PROPORTION_DECIMAL, UNIT, "TRAIT_RECONAISSANCE"
	);
	ret &= add_modifier_effect(modifier_effect_cache.reliability, "reliability", true, RAW_DECIMAL, UNIT, "TRAIT_RELIABILITY");
	ret &= add_modifier_effect(modifier_effect_cache.speed, "speed", true, PROPORTION_DECIMAL, UNIT, "TRAIT_SPEED");

	return ret;
}

bool ModifierManager::register_complex_modifier(std::string_view identifier) {
	if (complex_modifiers.emplace(identifier).second) {
		return true;
	} else {
		Logger::error("Duplicate complex modifier: ", identifier);
		return false;
	}
}

std::string ModifierManager::get_flat_identifier(
	std::string_view complex_modifier_identifier, std::string_view variant_identifier
) {
	return StringUtils::append_string_views(complex_modifier_identifier, " ", variant_identifier);
}

bool ModifierManager::add_event_modifier(std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon) {
	using enum Modifier::modifier_type_t;

	if (identifier.empty()) {
		Logger::error("Invalid event modifier effect identifier - empty!");
		return false;
	}

	return event_modifiers.add_item(
		{ identifier, std::move(values), EVENT, icon }, duplicate_warning_callback
	);
}

bool ModifierManager::load_event_modifiers(ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		event_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			using enum Modifier::modifier_type_t;

			ModifierValue modifier_value;
			IconModifier::icon_t icon = 0;

			bool ret = expect_modifier_value_and_keys(
				move_variable_callback(modifier_value),
				EVENT,
				"icon", ZERO_OR_ONE, expect_uint(assign_variable_callback(icon))
			)(value);

			ret &= add_event_modifier(key, std::move(modifier_value), icon);

			return ret;
		}
	)(root);

	lock_event_modifiers();

	return ret;
}

bool ModifierManager::add_static_modifier(std::string_view identifier, ModifierValue&& values) {
	using enum Modifier::modifier_type_t;

	if (identifier.empty()) {
		Logger::error("Invalid static modifier effect identifier - empty!");
		return false;
	}

	return static_modifiers.add_item(
		{ identifier, std::move(values), STATIC }, duplicate_warning_callback
	);
}

bool ModifierManager::load_static_modifiers(ast::NodeCPtr root) {
	bool ret = expect_dictionary_reserve_length(
		static_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			using enum Modifier::modifier_type_t;

			ModifierValue modifier_value;

			bool ret = expect_modifier_value(move_variable_callback(modifier_value), STATIC)(value);

			ret &= add_static_modifier(key, std::move(modifier_value));

			return ret;
		}
	)(root);

	lock_static_modifiers();

	ret &= static_modifier_cache.load_static_modifiers(*this);

	return ret;
}

bool ModifierManager::add_triggered_modifier(
	std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon, ConditionScript&& trigger
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

bool ModifierManager::load_triggered_modifiers(ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		triggered_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			using enum Modifier::modifier_type_t;

			ModifierValue modifier_value;
			IconModifier::icon_t icon = 0;
			ConditionScript trigger { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };

			bool ret = expect_modifier_value_and_keys(
				move_variable_callback(modifier_value),
				TRIGGERED,
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

key_value_callback_t ModifierManager::_modifier_effect_callback(
	ModifierValue& modifier, Modifier::modifier_type_t type, key_value_callback_t default_callback,
	ModifierEffectValidator auto effect_validator
) const {
	const auto add_modifier_cb = [this, &modifier, effect_validator](
		ModifierEffect const* effect, ast::NodeCPtr value
	) -> bool {
		if (effect_validator(*effect)) {
			static const case_insensitive_string_set_t no_effect_modifiers {
				"boost_strongest_party", "poor_savings_modifier",   "local_artisan_input",     "local_artisan_throughput",
				"local_artisan_output",  "artisan_input",           "artisan_throughput",      "artisan_output",
				"import_cost",           "unciv_economic_modifier", "unciv_military_modifier"
			};

			if (no_effect_modifiers.contains(effect->get_identifier())) {
				Logger::warning("This modifier does nothing: ", effect->get_identifier());
			}

			return expect_fixed_point(map_callback(modifier.values, effect))(value);
		} else {
			Logger::error("Failed to validate modifier effect: ", effect->get_identifier());
			return false;
		}
	};

	const auto add_flattened_modifier_cb = [this, add_modifier_cb](
		std::string_view prefix, std::string_view key, ast::NodeCPtr value
	) -> bool {
		const std::string flat_identifier = get_flat_identifier(prefix, key);
		ModifierEffect const* effect = get_modifier_effect_by_identifier(flat_identifier);
		if (effect != nullptr) {
			return add_modifier_cb(effect, value);
		} else {
			Logger::error("Could not find flattened modifier: ", flat_identifier);
			return false;
		}
	};

	return [this, default_callback, add_modifier_cb, add_flattened_modifier_cb](
		std::string_view key, ast::NodeCPtr value
	) -> bool {
		ModifierEffect const* effect = get_modifier_effect_by_identifier(key);
		if (effect != nullptr && dryad::node_has_kind<ast::IdentifierValue>(value)) {
			return add_modifier_cb(effect, value);
		} else if (complex_modifiers.contains(key) && dryad::node_has_kind<ast::ListValue>(value)) {
			if (key == "rebel_org_gain") { //because of course there's a special one
				std::string_view faction_identifier;
				ast::NodeCPtr value_node = nullptr;
				bool ret = expect_dictionary_keys(
					"faction", ONE_EXACTLY, expect_identifier(assign_variable_callback(faction_identifier)),
					"value", ONE_EXACTLY, assign_variable_callback(value_node)
				)(value);
				ret &= add_flattened_modifier_cb(key, faction_identifier, value_node);
				return ret;
			} else {
				return expect_dictionary(std::bind_front(add_flattened_modifier_cb, key))(value);
			}
		} else if (key == "war_exhaustion_effect") {
			Logger::warning("war_exhaustion_effect does nothing (vanilla issues have it).");
			return true;
		} else {
			return default_callback(key, value);
		}
	};
}

node_callback_t ModifierManager::expect_validated_modifier_value_and_default(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type, key_value_callback_t default_callback,
	ModifierEffectValidator auto effect_validator
) const {
	return [this, modifier_callback, type, default_callback, effect_validator](ast::NodeCPtr root) -> bool {
		ModifierValue modifier;

		bool ret = expect_dictionary_reserve_length(
			modifier.values,
			_modifier_effect_callback(modifier, type, default_callback, effect_validator)
		)(root);

		ret &= modifier_callback(std::move(modifier));

		return ret;
	};
}

node_callback_t ModifierManager::expect_validated_modifier_value(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type,
	ModifierEffectValidator auto effect_validator
) const {
	return expect_validated_modifier_value_and_default(modifier_callback, type, key_value_invalid_callback, effect_validator);
}

node_callback_t ModifierManager::expect_modifier_value_and_default(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type, key_value_callback_t default_callback
) const {
	return expect_validated_modifier_value_and_default(
		modifier_callback, type, default_callback, [](ModifierEffect const&) -> bool {
			return true;
		}
	);
}

node_callback_t ModifierManager::expect_modifier_value(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type
) const {
	return expect_modifier_value_and_default(modifier_callback, type, key_value_invalid_callback);
}

node_callback_t ModifierManager::expect_whitelisted_modifier_value_and_default(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type, string_set_t const& whitelist,
	key_value_callback_t default_callback
) const {
	return expect_validated_modifier_value_and_default(
		modifier_callback, type, default_callback,
		[&whitelist](ModifierEffect const& effect) -> bool {
			return whitelist.contains(effect.get_identifier());
		}
	);
}

node_callback_t ModifierManager::expect_whitelisted_modifier_value(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type, string_set_t const& whitelist
) const {
	return expect_whitelisted_modifier_value_and_default(modifier_callback, type, whitelist, key_value_invalid_callback);
}

node_callback_t ModifierManager::expect_modifier_value_and_key_map_and_default(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type, key_value_callback_t default_callback,
	key_map_t&& key_map
) const {
	return [this, modifier_callback, type, default_callback, key_map = std::move(key_map)](
		ast::NodeCPtr node
	) mutable -> bool {
		bool ret = expect_modifier_value_and_default(
			modifier_callback, type, dictionary_keys_callback(key_map, default_callback)
		)(node);

		ret &= check_key_map_counts(key_map);

		return ret;
	};
}

node_callback_t ModifierManager::expect_modifier_value_and_key_map(
	callback_t<ModifierValue&&> modifier_callback, Modifier::modifier_type_t type, key_map_t&& key_map
) const {
	return expect_modifier_value_and_key_map_and_default(
		modifier_callback, type, key_value_invalid_callback, std::move(key_map)
	);
}
