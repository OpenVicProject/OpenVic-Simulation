#include "Condition.hpp"

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/DefinitionManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

using enum value_type_t;
using enum scope_type_t;
using enum identifier_type_t;

Condition::Condition(
	std::string_view new_identifier, value_type_t new_value_type, scope_type_t new_scope,
	scope_type_t new_scope_change, identifier_type_t new_key_identifier_type,
	identifier_type_t new_value_identifier_type
) : HasIdentifier { new_identifier }, value_type { new_value_type }, scope { new_scope },
	scope_change { new_scope_change }, key_identifier_type { new_key_identifier_type },
	value_identifier_type { new_value_identifier_type } {}

ConditionNode::ConditionNode(
	Condition const* new_condition, value_t&& new_value, bool new_valid,
	HasIdentifier const* new_condition_key_item,
	HasIdentifier const* new_condition_value_item
) : condition { new_condition }, value { std::move(new_value) }, valid { new_valid },
	condition_key_item { new_condition_key_item }, condition_value_item { new_condition_key_item } {}

bool ConditionManager::add_condition(
	std::string_view identifier, value_type_t value_type, scope_type_t scope, scope_type_t scope_change,
	identifier_type_t key_identifier_type, identifier_type_t value_identifier_type
) {
	if (identifier.empty()) {
		Logger::error("Invalid condition identifier - empty!");
		return false;
	}

	if (value_type == NO_TYPE || value_type > MAX_VALUE) {
		Logger::error("Condition ", identifier, " has invalid value type: ", static_cast<uint64_t>(value_type));
		return false;
	}
	if (scope == NO_SCOPE || scope > MAX_SCOPE) {
		Logger::error("Condition ", identifier, " has invalid scope: ", static_cast<uint64_t>(scope));
		return false;
	}

	if (share_value_type(value_type, IDENTIFIER) && value_identifier_type == NO_IDENTIFIER) {
		Logger::error("Condition ", identifier, " has no identifier type!");
		return false;
	}

	// don't perform the check for complex types
	if (!share_value_type(value_type, COMPLEX)) {
		if (!share_value_type(value_type, IDENTIFIER) && value_identifier_type != NO_IDENTIFIER) {
			Logger::warning("Condition ", identifier, " specified an identifier type, but doesn't have an identifier!");
		}
	}

	return conditions.add_item({
		identifier,
		value_type,
		scope,
		scope_change,
		key_identifier_type,
		value_identifier_type
	});
}

bool ConditionManager::setup_conditions(DefinitionManager const& definition_manager) {
	bool ret = true;

	/* Special Scopes */
	ret &= add_condition("THIS", GROUP, COUNTRY, THIS);
	ret &= add_condition("FROM", GROUP, COUNTRY, FROM);
	ret &= add_condition("independence", GROUP, COUNTRY, COUNTRY); //only from rebels!

	/* Trigger Country Scopes */
	ret &= add_condition("all_core", GROUP, COUNTRY, PROVINCE);
	ret &= add_condition("any_core", GROUP, COUNTRY, PROVINCE);
	ret &= add_condition("any_greater_power", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("any_neighbor_country", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("any_owned_province", GROUP, COUNTRY, PROVINCE);
	ret &= add_condition("any_pop", GROUP, COUNTRY, POP);
	ret &= add_condition("any_sphere_member", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("any_state", GROUP, COUNTRY, STATE);
	ret &= add_condition("any_substate", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("capital_scope", GROUP, COUNTRY, PROVINCE);
	ret &= add_condition("country", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("cultural_union", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("overlord", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("owner", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("sphere_owner", GROUP, COUNTRY, COUNTRY);
	ret &= add_condition("war_countries", GROUP, COUNTRY, COUNTRY);

	/* Trigger State Scopes */
	ret &= add_condition("any_neighbor_province", GROUP, STATE, PROVINCE);

	/* Trigger Province Scopes */
	ret &= add_condition("controller", GROUP, PROVINCE, COUNTRY);
	ret &= add_condition("state_scope", GROUP, PROVINCE, STATE);

	/* Trigger Pop Scopes */
	ret &= add_condition("location", GROUP, POP, PROVINCE);

	/* Special Conditions */
	ret &= add_condition("AND", GROUP, COUNTRY);
	ret &= add_condition("OR", GROUP, COUNTRY);
	ret &= add_condition("NOT", GROUP, COUNTRY);

	/* Global Conditions */
	ret &= add_condition("year", INTEGER, COUNTRY);
	ret &= add_condition("month", INTEGER, COUNTRY);
	ret &= add_condition("has_global_flag", IDENTIFIER, COUNTRY,  NO_SCOPE, NO_IDENTIFIER, GLOBAL_FLAG);
	ret &= add_condition("is_canal_enabled", INTEGER, COUNTRY);
	ret &= add_condition("always", BOOLEAN, COUNTRY);
	ret &= add_condition("world_wars_enabled", BOOLEAN, COUNTRY);

	/* Country Scope Conditions */
	ret &= add_condition("administration_spending", REAL, COUNTRY);
	ret &= add_condition("ai", BOOLEAN, COUNTRY);
	ret &= add_condition("alliance_with", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("average_consciousness", REAL, COUNTRY);
	ret &= add_condition("average_militancy", REAL, COUNTRY);
	ret &= add_condition("badboy", REAL, COUNTRY);
	ret &= add_condition("big_producer", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, TRADE_GOOD);
	ret &= add_condition("blockade", REAL, COUNTRY);
	ret &= add_condition("brigades_compare", REAL, COUNTRY);
	ret &= add_condition("can_build_factory_in_capital_state", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, BUILDING);
	ret &= add_condition("can_build_fort_in_capital", COMPLEX, COUNTRY);
	ret &= add_condition("can_build_railway_in_capital", COMPLEX, COUNTRY);
	ret &= add_condition("can_nationalize", BOOLEAN, COUNTRY);
	ret &= add_condition("can_create_vassals", BOOLEAN, COUNTRY);
	ret &= add_condition("capital", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	ret &= add_condition("casus_belli", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("check_variable", COMPLEX, COUNTRY, NO_SCOPE, NO_IDENTIFIER, VARIABLE);
	ret &= add_condition("citizenship_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	ret &= add_condition("civilization_progress", REAL, COUNTRY);
	ret &= add_condition("civilized", BOOLEAN, COUNTRY);
	ret &= add_condition("colonial_nation", BOOLEAN, COUNTRY);
	ret &= add_condition("consciousness", REAL, COUNTRY);
	ret &= add_condition("constructing_cb_progress", REAL, COUNTRY);
	ret &= add_condition("constructing_cb_type", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CASUS_BELLI);
	ret &= add_condition("continent", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CONTINENT);
	ret &= add_condition("controls", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	ret &= add_condition("crime_fighting", REAL, COUNTRY);
	ret &= add_condition("crime_higher_than_education", BOOLEAN, COUNTRY);
	ret &= add_condition("crisis_exist", BOOLEAN, COUNTRY);
	ret &= add_condition("culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	ret &= add_condition("culture_has_union_tag", BOOLEAN, COUNTRY);
	ret &= add_condition("diplomatic_influence", COMPLEX, COUNTRY);
	ret &= add_condition("economic_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	ret &= add_condition("education_spending", REAL, COUNTRY);
	ret &= add_condition("election", BOOLEAN, COUNTRY);
	ret &= add_condition("exists", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("government", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, GOVERNMENT_TYPE);
	ret &= add_condition("great_wars_enabled", BOOLEAN, COUNTRY);
	ret &= add_condition("have_core_in", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("has_country_flag", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_FLAG);
	ret &= add_condition("has_country_modifier", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_EVENT_MODIFIER);
	ret &= add_condition("has_cultural_sphere", BOOLEAN, COUNTRY);
	ret &= add_condition("has_leader", STRING, COUNTRY);
	ret &= add_condition("has_pop_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	ret &= add_condition("has_pop_religion", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, RELIGION);
	ret &= add_condition("has_pop_type", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, POP_TYPE);
	ret &= add_condition("has_recently_lost_war", BOOLEAN, COUNTRY);
	ret &= add_condition("has_unclaimed_cores", BOOLEAN, COUNTRY);
	ret &= add_condition("ideology", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	ret &= add_condition("industrial_score", REAL | IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("in_sphere", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("in_default", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("invention", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, INVENTION);
	ret &= add_condition("involved_in_crisis", BOOLEAN, COUNTRY);
	ret &= add_condition("is_claim_crisis", BOOLEAN, COUNTRY);
	ret &= add_condition("is_colonial_crisis", BOOLEAN, COUNTRY);
	ret &= add_condition("is_cultural_union", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("is_disarmed", BOOLEAN, COUNTRY);
	ret &= add_condition("is_greater_power", BOOLEAN, COUNTRY);
	ret &= add_condition("is_colonial", BOOLEAN, STATE);
	ret &= add_condition("is_core", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG | PROVINCE_ID);
	ret &= add_condition("is_culture_group", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG | CULTURE_GROUP);
	ret &= add_condition("is_ideology_enabled", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	ret &= add_condition("is_independant", BOOLEAN, COUNTRY); // paradox typo
	ret &= add_condition("is_liberation_crisis", BOOLEAN, COUNTRY);
	ret &= add_condition("is_mobilised", BOOLEAN, COUNTRY);
	ret &= add_condition("is_next_reform", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, REFORM);
	ret &= add_condition("is_our_vassal", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("is_possible_vassal", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("is_releasable_vassal", IDENTIFIER | BOOLEAN, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("is_secondary_power", BOOLEAN, COUNTRY);
	ret &= add_condition("is_sphere_leader_of", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("is_substate", BOOLEAN, COUNTRY);
	ret &= add_condition("is_vassal", BOOLEAN, COUNTRY);
	ret &= add_condition("literacy", REAL, COUNTRY);
	ret &= add_condition("lost_national", REAL, COUNTRY);
	ret &= add_condition("middle_strata_militancy", REAL, COUNTRY);
	ret &= add_condition("middle_strata_everyday_needs", REAL, COUNTRY);
	ret &= add_condition("middle_strata_life_needs", REAL, COUNTRY);
	ret &= add_condition("middle_strata_luxury_needs", REAL, COUNTRY);
	ret &= add_condition("middle_tax", REAL, COUNTRY);
	ret &= add_condition("military_access", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("military_score", REAL | IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("militancy", REAL, COUNTRY);
	ret &= add_condition("military_spending", REAL, COUNTRY);
	ret &= add_condition("money", REAL, COUNTRY);
	ret &= add_condition("nationalvalue", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, NATIONAL_VALUE);
	ret &= add_condition("national_provinces_occupied", REAL, COUNTRY);
	ret &= add_condition("neighbour", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("num_of_allies", INTEGER, COUNTRY);
	ret &= add_condition("num_of_cities", INTEGER, COUNTRY);
	ret &= add_condition("num_of_ports", INTEGER, COUNTRY);
	ret &= add_condition("num_of_revolts", INTEGER, COUNTRY);
	ret &= add_condition("number_of_states", INTEGER, COUNTRY);
	ret &= add_condition("num_of_substates", INTEGER, COUNTRY);
	ret &= add_condition("num_of_vassals", INTEGER, COUNTRY);
	ret &= add_condition("num_of_vassals_no_substates", INTEGER, COUNTRY);
	ret &= add_condition("owns", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	ret &= add_condition("part_of_sphere", BOOLEAN, COUNTRY);
	ret &= add_condition("plurality", REAL, COUNTRY);
	ret &= add_condition("political_movement_strength", REAL, COUNTRY);
	ret &= add_condition("political_reform_want", REAL, COUNTRY);
	ret &= add_condition("pop_majority_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	ret &= add_condition("pop_majority_ideology", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	ret &= add_condition("pop_majority_religion", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, RELIGION);
	ret &= add_condition("pop_militancy", REAL, COUNTRY);
	ret &= add_condition("poor_strata_militancy", REAL, COUNTRY);
	ret &= add_condition("poor_strata_everyday_needs", REAL, COUNTRY);
	ret &= add_condition("poor_strata_life_needs", REAL, COUNTRY);
	ret &= add_condition("poor_strata_luxury_needs", REAL, COUNTRY);
	ret &= add_condition("poor_tax", REAL, COUNTRY);
	ret &= add_condition("prestige", REAL, COUNTRY);
	ret &= add_condition("primary_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	ret &= add_condition("accepted_culture", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE);
	ret &= add_condition("produces", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, TRADE_GOOD);
	ret &= add_condition("rank", INTEGER, COUNTRY);
	ret &= add_condition("rebel_power_fraction", REAL, COUNTRY);
	ret &= add_condition("recruited_percentage", REAL, COUNTRY);
	ret &= add_condition("relation", COMPLEX, COUNTRY);
	ret &= add_condition("religion", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, RELIGION);
	ret &= add_condition("religious_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	ret &= add_condition("revanchism", REAL, COUNTRY);
	ret &= add_condition("revolt_percentage", REAL, COUNTRY);
	ret &= add_condition("rich_strata_militancy", REAL, COUNTRY);
	ret &= add_condition("rich_strata_everyday_needs", REAL, COUNTRY);
	ret &= add_condition("rich_strata_life_needs", REAL, COUNTRY);
	ret &= add_condition("rich_strata_luxury_needs", REAL, COUNTRY);
	ret &= add_condition("rich_tax", REAL, COUNTRY);
	ret &= add_condition("rich_tax_above_poor", BOOLEAN, COUNTRY);
	ret &= add_condition("ruling_party", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("ruling_party_ideology", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, IDEOLOGY);
	ret &= add_condition("social_movement_strength", REAL, COUNTRY);
	ret &= add_condition("social_reform_want", REAL, COUNTRY);
	ret &= add_condition("social_spending", REAL, COUNTRY);
	ret &= add_condition("stronger_army_than", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("substate_of", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("tag", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("tech_school", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, TECH_SCHOOL);
	ret &= add_condition("this_culture_union", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, CULTURE_UNION);
	ret &= add_condition("total_amount_of_divisions", INTEGER, COUNTRY);
	ret &= add_condition("total_amount_of_ships", INTEGER, COUNTRY);
	ret &= add_condition("total_defensives", INTEGER, COUNTRY);
	ret &= add_condition("total_num_of_ports", INTEGER, COUNTRY);
	ret &= add_condition("total_of_ours_sunk", INTEGER, COUNTRY);
	ret &= add_condition("total_pops", INTEGER, COUNTRY);
	ret &= add_condition("total_sea_battles", INTEGER, COUNTRY);
	ret &= add_condition("total_sunk_by_us", INTEGER, COUNTRY);
	ret &= add_condition("trade_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	ret &= add_condition("treasury", REAL, COUNTRY);
	ret &= add_condition("truce_with", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("unemployment", REAL, COUNTRY);
	ret &= add_condition("unit_has_leader", BOOLEAN, COUNTRY);
	ret &= add_condition("unit_in_battle", BOOLEAN, COUNTRY);
	ret &= add_condition("upper_house", COMPLEX, COUNTRY);
	ret &= add_condition("vassal_of", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("war", BOOLEAN, COUNTRY);
	ret &= add_condition("war_exhaustion", REAL, COUNTRY);
	ret &= add_condition("war_policy", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	ret &= add_condition("war_score", REAL, COUNTRY);
	ret &= add_condition("war_with", IDENTIFIER, COUNTRY, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);

	/* State Scope Conditions */
	ret &= add_condition("controlled_by", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("empty", BOOLEAN, STATE);
	ret &= add_condition("flashpoint_tension", REAL, STATE);
	ret &= add_condition("has_building", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, BUILDING);
	ret &= add_condition("has_factories", BOOLEAN, STATE);
	ret &= add_condition("has_flashpoint", BOOLEAN, STATE);
	ret &= add_condition("is_slave", BOOLEAN, STATE);
	ret &= add_condition("owned_by", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("trade_goods_in_state", IDENTIFIER, STATE, NO_SCOPE, NO_IDENTIFIER, TRADE_GOOD);
	ret &= add_condition("work_available", COMPLEX, STATE);

	/* Province Scope Conditions */
	ret &= add_condition("can_build_factory", BOOLEAN, PROVINCE);
	ret &= add_condition("controlled_by_rebels", BOOLEAN, PROVINCE);
	ret &= add_condition("country_units_in_province", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("country_units_in_state", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("has_crime", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, CRIME);
	ret &= add_condition("has_culture_core", BOOLEAN, PROVINCE);
	ret &= add_condition("has_empty_adjacent_province", BOOLEAN, PROVINCE);
	ret &= add_condition("has_empty_adjacent_state", BOOLEAN, PROVINCE);
	ret &= add_condition("has_national_minority", BOOLEAN, PROVINCE);
	ret &= add_condition("has_province_flag", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_FLAG);
	ret &= add_condition("has_province_modifier", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_EVENT_MODIFIER);
	ret &= add_condition("has_recent_imigration", INTEGER, PROVINCE); // paradox typo
	ret &= add_condition("is_blockaded", BOOLEAN, PROVINCE);
	ret &= add_condition("is_accepted_culture", IDENTIFIER | BOOLEAN, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("is_capital", BOOLEAN, PROVINCE);
	ret &= add_condition("is_coastal", BOOLEAN, PROVINCE);
	ret &= add_condition("is_overseas", BOOLEAN, PROVINCE);
	ret &= add_condition("is_primary_culture", IDENTIFIER | BOOLEAN, PROVINCE, NO_SCOPE, NO_IDENTIFIER, COUNTRY_TAG);
	ret &= add_condition("is_state_capital", BOOLEAN, PROVINCE);
	ret &= add_condition("is_state_religion", BOOLEAN, PROVINCE);
	ret &= add_condition("life_rating", REAL, PROVINCE);
	ret &= add_condition("minorities", BOOLEAN, PROVINCE);
	ret &= add_condition("port", BOOLEAN, PROVINCE);
	ret &= add_condition("province_control_days", INTEGER, PROVINCE);
	ret &= add_condition("province_id", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	ret &= add_condition("region", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, REGION);
	ret &= add_condition("state_id", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, PROVINCE_ID);
	ret &= add_condition("terrain", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, TERRAIN);
	ret &= add_condition("trade_goods", IDENTIFIER, PROVINCE, NO_SCOPE, NO_IDENTIFIER, TRADE_GOOD);
	ret &= add_condition("unemployment_by_type", COMPLEX, PROVINCE);
	ret &= add_condition("units_in_province", INTEGER, PROVINCE);

	/* Pop Scope Conditions */
	ret &= add_condition("agree_with_ruling_party", REAL, POP);
	ret &= add_condition("cash_reserves", REAL, POP);
	ret &= add_condition("everyday_needs", REAL, POP);
	ret &= add_condition("life_needs", REAL, POP);
	ret &= add_condition("luxury_needs", REAL, POP);
	ret &= add_condition("political_movement", BOOLEAN, POP);
	ret &= add_condition("pop_majority_issue", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, ISSUE);
	ret &= add_condition("pop_type", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, POP_TYPE);
	ret &= add_condition("social_movement", BOOLEAN, POP);
	ret &= add_condition("strata", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, POP_STRATA);
	ret &= add_condition("type", IDENTIFIER, POP, NO_SCOPE, NO_IDENTIFIER, POP_TYPE);

	const auto import_identifiers = [this, &ret](
		std::vector<std::string_view> const& identifiers,
		value_type_t value_type,
		scope_type_t scope,
		scope_type_t scope_change = NO_SCOPE,
		identifier_type_t key_identifier_type = NO_IDENTIFIER,
		identifier_type_t value_identifier_type = NO_IDENTIFIER
	) -> void {
		for (std::string_view const& identifier : identifiers) {
			ret &= add_condition(
				identifier, value_type, scope, scope_change,
				key_identifier_type, value_identifier_type
			);
		}
	};

	/* Scopes from other registries */
	import_identifiers(
		definition_manager.get_country_definition_manager().get_country_definition_identifiers(),
		GROUP,
		COUNTRY,
		COUNTRY,
		COUNTRY_TAG,
		NO_IDENTIFIER
	);

	import_identifiers(
		definition_manager.get_map_definition().get_region_identifiers(),
		GROUP,
		COUNTRY,
		STATE,
		REGION,
		NO_IDENTIFIER
	);

	import_identifiers(
		definition_manager.get_map_definition().get_province_definition_identifiers(),
		GROUP,
		COUNTRY,
		PROVINCE,
		PROVINCE_ID,
		NO_IDENTIFIER
	);

	/* Conditions from other registries */
	import_identifiers(
		definition_manager.get_politics_manager().get_ideology_manager().get_ideology_identifiers(),
		REAL,
		COUNTRY,
		NO_SCOPE,
		IDEOLOGY,
		NO_IDENTIFIER
	);

	import_identifiers(
		definition_manager.get_politics_manager().get_issue_manager().get_reform_group_identifiers(),
		IDENTIFIER,
		COUNTRY,
		NO_SCOPE,
		REFORM_GROUP,
		REFORM
	);

	import_identifiers(
		definition_manager.get_politics_manager().get_issue_manager().get_reform_identifiers(),
		REAL,
		COUNTRY,
		NO_SCOPE,
		REFORM,
		NO_IDENTIFIER
	);

	import_identifiers(
		definition_manager.get_politics_manager().get_issue_manager().get_issue_identifiers(),
		REAL,
		COUNTRY,
		NO_SCOPE,
		ISSUE,
		NO_IDENTIFIER
	);

	import_identifiers(
		definition_manager.get_pop_manager().get_pop_type_identifiers(),
		REAL,
		COUNTRY,
		NO_SCOPE,
		POP_TYPE,
		NO_IDENTIFIER
	);

	import_identifiers(
		definition_manager.get_research_manager().get_technology_manager().get_technology_identifiers(),
		BOOLEAN_INT,
		COUNTRY,
		NO_SCOPE,
		TECHNOLOGY,
		NO_IDENTIFIER
	);

	import_identifiers(
		definition_manager.get_economy_manager().get_good_definition_manager().get_good_definition_identifiers(),
		INTEGER,
		COUNTRY,
		NO_SCOPE,
		TRADE_GOOD,
		NO_IDENTIFIER
	);

	lock_conditions();

	static constexpr std::string_view condition_root_identifier = "AND";
	root_condition = get_condition_by_identifier(condition_root_identifier);

	if (root_condition == nullptr) {
		Logger::error("Failed to find root condition: ", condition_root_identifier);
		ret = false;
	}

	return ret;
}

callback_t<std::string_view> ConditionManager::expect_parse_identifier(
	DefinitionManager const& definition_manager, identifier_type_t identifier_type,
	callback_t<HasIdentifier const*> callback
) const {
	return [this, &definition_manager, identifier_type, callback](std::string_view identifier) mutable -> bool {
		HasIdentifier const* identified = nullptr;

		#define EXPECT_CALL(type, name, manager, ...) \
			if (share_identifier_type(identifier_type, type)) { \
				identified = manager.get_##name##_by_identifier(identifier); \
				if (identified != nullptr) { \
					return callback(identified); \
				} __VA_OPT__(else { \
					/* TODO: the set is just a placeholder for actual logic */ \
					static const case_insensitive_string_set_t chances { __VA_ARGS__ }; \
					if (chances.contains(identifier)) { \
						return true; \
					} \
				}) \
			}

		//TODO: placeholder for not implemented stuff
		#define EXPECT_CALL_PLACEHOLDER(type) \
			if (share_identifier_type(identifier_type, type)) { return true; }

		EXPECT_CALL_PLACEHOLDER(VARIABLE);
		EXPECT_CALL_PLACEHOLDER(GLOBAL_FLAG);
		EXPECT_CALL_PLACEHOLDER(COUNTRY_FLAG);
		EXPECT_CALL_PLACEHOLDER(PROVINCE_FLAG);
		EXPECT_CALL(
			COUNTRY_TAG, country_definition, definition_manager.get_country_definition_manager(), "THIS", "FROM", "OWNER"
		);
		EXPECT_CALL(PROVINCE_ID, province_definition, definition_manager.get_map_definition(), "THIS", "FROM");
		EXPECT_CALL(REGION, region, definition_manager.get_map_definition());
		EXPECT_CALL(IDEOLOGY, ideology, definition_manager.get_politics_manager().get_ideology_manager());
		EXPECT_CALL(REFORM_GROUP, reform_group, definition_manager.get_politics_manager().get_issue_manager());
		EXPECT_CALL(REFORM, reform, definition_manager.get_politics_manager().get_issue_manager());
		EXPECT_CALL(ISSUE, issue, definition_manager.get_politics_manager().get_issue_manager());
		EXPECT_CALL(POP_TYPE, pop_type, definition_manager.get_pop_manager());
		EXPECT_CALL(POP_STRATA, strata, definition_manager.get_pop_manager());
		EXPECT_CALL(TECHNOLOGY, technology, definition_manager.get_research_manager().get_technology_manager());
		EXPECT_CALL(INVENTION, invention, definition_manager.get_research_manager().get_invention_manager());
		EXPECT_CALL(TECH_SCHOOL, technology_school, definition_manager.get_research_manager().get_technology_manager());
		EXPECT_CALL(CULTURE, culture, definition_manager.get_pop_manager().get_culture_manager(), "THIS", "FROM");
		EXPECT_CALL(CULTURE_GROUP, culture_group, definition_manager.get_pop_manager().get_culture_manager());
		EXPECT_CALL(RELIGION, religion, definition_manager.get_pop_manager().get_religion_manager(), "THIS", "FROM");
		EXPECT_CALL(TRADE_GOOD, good_definition, definition_manager.get_economy_manager().get_good_definition_manager());
		EXPECT_CALL(BUILDING, building_type, definition_manager.get_economy_manager().get_building_type_manager(), "FACTORY");
		EXPECT_CALL(CASUS_BELLI, wargoal_type, definition_manager.get_military_manager().get_wargoal_type_manager());
		EXPECT_CALL(GOVERNMENT_TYPE, government_type, definition_manager.get_politics_manager().get_government_type_manager());
		EXPECT_CALL(COUNTRY_EVENT_MODIFIER | PROVINCE_EVENT_MODIFIER, event_modifier, definition_manager.get_modifier_manager());
		EXPECT_CALL(COUNTRY_EVENT_MODIFIER, triggered_modifier, definition_manager.get_modifier_manager());
		EXPECT_CALL(NATIONAL_VALUE, national_value, definition_manager.get_politics_manager().get_national_value_manager());
		EXPECT_CALL(
			CULTURE_UNION, country_definition, definition_manager.get_country_definition_manager(), "THIS", "FROM", "THIS_UNION"
		);
		EXPECT_CALL(CONTINENT, continent, definition_manager.get_map_definition());
		EXPECT_CALL(CRIME, crime_modifier, definition_manager.get_crime_manager());
		EXPECT_CALL(TERRAIN, terrain_type, definition_manager.get_map_definition().get_terrain_type_manager());

		#undef EXPECT_CALL
		#undef EXPECT_CALL_PLACEHOLDER

		return false;
	};
}

node_callback_t ConditionManager::expect_condition_node(
	DefinitionManager const& definition_manager, Condition const& condition, scope_type_t current_scope,
	scope_type_t this_scope, scope_type_t from_scope, callback_t<ConditionNode&&> callback
) const {
	return [this, &definition_manager, &condition, callback, current_scope, this_scope, from_scope](
		ast::NodeCPtr node
	) mutable -> bool {
		bool ret = false;
		ConditionNode::value_t value;

		const std::string_view identifier = condition.get_identifier();
		const value_type_t value_type = condition.get_value_type();
		const scope_type_t scope = condition.get_scope();
		const scope_type_t scope_change = condition.get_scope_change();
		const identifier_type_t key_identifier_type = condition.get_key_identifier_type();
		const identifier_type_t value_identifier_type = condition.get_value_identifier_type();

		HasIdentifier const* value_item = nullptr;

		const auto get_identifiable = [this, &definition_manager](
			identifier_type_t item_type, std::string_view id, bool log
		) -> HasIdentifier const* {
			HasIdentifier const* keyval = nullptr;
			bool ret = expect_parse_identifier(
				definition_manager,
				item_type,
				assign_variable_callback(keyval)
			)(id);
			if (log && !ret) {
				Logger::error(
					"Invalid identifier ", id, " expected to have type ", item_type, " found during condition node parsing!"
				);
			}
			return keyval;
		};

		if (!ret && share_value_type(value_type, IDENTIFIER)) {
			std::string_view value_identifier {};
			ret |= expect_identifier_or_string(assign_variable_callback(value_identifier))(node);
			if (ret) {
				value = ConditionNode::string_t { value_identifier };
				value_item = get_identifiable(
					value_identifier_type,
					value_identifier,
					value_type == IDENTIFIER // don't log if there's a fallback
				);
				ret |= value_item != nullptr;
			}
		}

		if (!ret && share_value_type(value_type, STRING)) {
			std::string_view value_identifier {};
			bool local_ret = expect_identifier_or_string(
				assign_variable_callback(value_identifier)
			)(node);
			ret |= local_ret;
			if (local_ret) {
				value = ConditionNode::string_t { value_identifier };
			}
		}

		ret |= (!ret && share_value_type(value_type, BOOLEAN))
			&& expect_bool(assign_variable_callback(value))(node);
		ret |= (!ret && share_value_type(value_type, BOOLEAN_INT))
			&& expect_int_bool(assign_variable_callback(value))(node);
		ret |= (!ret && share_value_type(value_type, INTEGER))
			&& expect_uint64(assign_variable_callback(value))(node);
		ret |= (!ret && share_value_type(value_type, REAL))
			&& expect_fixed_point(assign_variable_callback(value))(node);

		//entries with magic syntax, thanks paradox!
		if (!ret && share_value_type(value_type, COMPLEX)) {
			const auto expect_pair = [&ret, &value, node](
				std::string_view identifier_key, std::string_view value_key
			) -> void {
				std::string_view pair_identifier {};
				fixed_point_t pair_value = 0;
				ret |= expect_dictionary_keys(
					identifier_key, ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(pair_identifier)),
					value_key, ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pair_value))
				)(node);
				if (ret) {
					value = ConditionNode::identifier_real_t { pair_identifier, pair_value };
				}
			};

			if (identifier == "can_build_railway_in_capital" || identifier == "can_build_fort_in_capital") {
				bool in_whole_capital_state = false, limit_to_world_greatest_level = false;
				ret |= expect_dictionary_keys(
					"in_whole_capital_state", ONE_EXACTLY, expect_bool(assign_variable_callback(in_whole_capital_state)),
					"limit_to_world_greatest_level", ONE_EXACTLY,
						expect_bool(assign_variable_callback(limit_to_world_greatest_level))
				)(node);
				if (ret) {
					value = ConditionNode::double_boolean_t { in_whole_capital_state, limit_to_world_greatest_level };
				}
			} else if (identifier == "check_variable") {
				expect_pair("which", "value"); // { which = [name of variable] value = x }
			} else if (identifier == "diplomatic_influence") {
				expect_pair("who", "value"); // { who = [THIS/FROM/TAG] value = x }
			} else if (identifier == "relation") {
				expect_pair("who", "value"); // { who = [tag/this/from] value = x }
			} else if (identifier == "unemployment_by_type") {
				expect_pair("type", "value"); // { type = [poptype] value = x }
			} else if (identifier == "upper_house") {
				expect_pair("ideology", "value"); // { ideology = name value = 0.x }
			} else if (identifier == "work_available") {
				// { worker = [type] }
				std::string_view worker {};
				ret |= expect_dictionary_keys(
					"worker", ONE_EXACTLY, expect_identifier_or_string(assign_variable_callback(worker))
				)(node);
				if (ret) {
					value = ConditionNode::string_t { worker };
				}
			} else {
				Logger::error("Attempted to parse unknown complex condition ", identifier, "!");
			}

			#undef EXPECT_PAIR
		}

		if (!ret && share_value_type(value_type, GROUP)) {
			ConditionNode::condition_list_t node_list;
			ret |= expect_condition_node_list(
				definition_manager,
				scope_change == NO_SCOPE ? current_scope : scope_change,
				this_scope,
				from_scope,
				vector_callback(node_list)
			)(node);
			value = std::move(node_list);
		}

		// scope validation
		scope_type_t effective_current_scope = current_scope;
		if (share_scope_type(effective_current_scope, THIS)) {
			effective_current_scope = this_scope;
		} else if (share_scope_type(effective_current_scope, FROM)) {
			effective_current_scope = from_scope;
		}

		if (!share_scope_type(scope, effective_current_scope) && effective_current_scope > scope) {
			Logger::warning(
				"Condition or scope ", identifier, " was found in wrong scope ", effective_current_scope, ", expected ",
				scope, "!"
			);
			ret = false;
		}

		// key parsing
		HasIdentifier const* key_item = nullptr;
		if (condition.key_identifier_type != NO_IDENTIFIER) {
			key_item = get_identifiable(key_identifier_type, identifier, true);
			ret &= key_item != nullptr;
		}

		if (!ret) {
			Logger::error("Could not parse condition node ", identifier);
		}

		ret &= callback({
			&condition,
			std::move(value),
			ret,
			key_item,
			value_item
		});

		return ret;
	};
}

/* Default callback for top condition scope. */
static bool top_scope_fallback(std::string_view id, ast::NodeCPtr node) {
	/* This is a non-condition key, and so not case-insensitive. */
	if (id == "factor") {
		return true;
	} else {
		Logger::error("Unknown node \"", id, "\" found while parsing conditions!");
		return false;
	}
};

node_callback_t ConditionManager::expect_condition_node_list(
	DefinitionManager const& definition_manager, scope_type_t current_scope, scope_type_t this_scope, scope_type_t from_scope,
	callback_t<ConditionNode&&> callback, bool top_scope
) const {
	return [this, &definition_manager, callback, current_scope, this_scope, from_scope, top_scope](ast::NodeCPtr node) -> bool {
		const auto expect_node = [
			this, &definition_manager, callback, current_scope, this_scope, from_scope
		](Condition const& condition, ast::NodeCPtr node) -> bool {
			return expect_condition_node(
				definition_manager, condition, current_scope, this_scope, from_scope, callback
			)(node);
		};

		bool ret = conditions.expect_item_dictionary_and_default(
			top_scope ? top_scope_fallback : key_value_invalid_callback, expect_node
		)(node);
		if (!ret) {
			Logger::error("Error parsing condition node:\n", node);
		}
		return ret;
	};
}

node_callback_t ConditionManager::expect_condition_script(
	DefinitionManager const& definition_manager, scope_type_t initial_scope, scope_type_t this_scope,
	scope_type_t from_scope, callback_t<ConditionNode&&> callback
) const {
	return [this, &definition_manager, initial_scope, this_scope, from_scope, callback](ast::NodeCPtr node) mutable -> bool {

		ConditionNode::condition_list_t conds;
		bool ret = expect_condition_node_list(
			definition_manager,
			initial_scope,
			this_scope,
			from_scope,
			NodeTools::vector_callback(conds),
			true
		)(node);

		ret &= callback({ root_condition, std::move(conds), true });

		return ret;
	};
}
