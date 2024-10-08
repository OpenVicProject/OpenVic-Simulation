#include "Modifier.hpp"

#include <string>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

static std::string make_default_modifier_effect_localisation_key(std::string_view identifier) {
	return "MODIFIER_" + StringUtils::string_toupper(identifier);
}

ModifierEffect::ModifierEffect(
	std::string_view new_identifier, bool new_positive_good, format_t new_format, std::string_view new_localisation_key
) : HasIdentifier { new_identifier }, positive_good { new_positive_good }, format { new_format },
	localisation_key {
		new_localisation_key.empty() ? make_default_modifier_effect_localisation_key(new_identifier) : new_localisation_key
	} {}

ModifierValue::ModifierValue() = default;
ModifierValue::ModifierValue(effect_map_t&& new_values) : values { std::move(new_values) } {}
ModifierValue::ModifierValue(ModifierValue const&) = default;
ModifierValue::ModifierValue(ModifierValue&&) = default;

ModifierValue& ModifierValue::operator=(ModifierValue const&) = default;
ModifierValue& ModifierValue::operator=(ModifierValue&&) = default;

void ModifierValue::trim() {
	erase_if(values, [](effect_map_t::value_type const& value) -> bool {
		return value.second == fixed_point_t::_0();
	});
}

size_t ModifierValue::get_effect_count() const {
	return values.size();
}

void ModifierValue::clear() {
	values.clear();
}

bool ModifierValue::empty() const {
	return values.empty();
}

fixed_point_t ModifierValue::get_effect(ModifierEffect const& effect, bool* effect_found) const {
	const effect_map_t::const_iterator it = values.find(&effect);
	if (it != values.end()) {
		if (effect_found != nullptr) {
			*effect_found = true;
		}
		return it->second;
	}
	if (effect_found != nullptr) {
		*effect_found = false;
	}
	return fixed_point_t::_0();
}

bool ModifierValue::has_effect(ModifierEffect const& effect) const {
	return values.contains(&effect);
}

void ModifierValue::set_effect(ModifierEffect const& effect, fixed_point_t value) {
	values[&effect] = value;
}

ModifierValue& ModifierValue::operator+=(ModifierValue const& right) {
	for (effect_map_t::value_type const& value : right.values) {
		values[value.first] += value.second;
	}
	return *this;
}

ModifierValue ModifierValue::operator+(ModifierValue const& right) const {
	ModifierValue copy = *this;
	return copy += right;
}

ModifierValue ModifierValue::operator-() const {
	ModifierValue copy = *this;
	for (auto value : mutable_iterator(copy.values)) {
		value.second = -value.second;
	}
	return copy;
}

ModifierValue& ModifierValue::operator-=(ModifierValue const& right) {
	for (effect_map_t::value_type const& value : right.values) {
		values[value.first] -= value.second;
	}
	return *this;
}

ModifierValue ModifierValue::operator-(ModifierValue const& right) const {
	ModifierValue copy = *this;
	return copy -= right;
}

ModifierValue& ModifierValue::operator*=(fixed_point_t const& right) {
	for (auto value : mutable_iterator(values)) {
		value.second *= right;
	}
	return *this;
}

ModifierValue ModifierValue::operator*(fixed_point_t const& right) const {
	ModifierValue copy = *this;
	return copy *= right;
}

fixed_point_t& ModifierValue::operator[](ModifierEffect const& effect) {
	return values[&effect];
}

void ModifierValue::multiply_add(ModifierValue const& other, fixed_point_t multiplier) {
	if (multiplier == fixed_point_t::_1()) {
		*this += other;
	} else if (multiplier != fixed_point_t::_0()) {
		for (effect_map_t::value_type const& value : other.values) {
			values[value.first] += value.second * multiplier;
		}
	}
}

Modifier::Modifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type)
	: HasIdentifier { new_identifier }, ModifierValue { std::move(new_values) }, type { new_type } {}

IconModifier::IconModifier(
	std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon
) : Modifier { new_identifier, std::move(new_values), new_type }, icon { new_icon } {}

TriggeredModifier::TriggeredModifier(
	std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon,
	ConditionScript&& new_trigger
) : IconModifier { new_identifier, std::move(new_values), new_type, new_icon }, trigger { std::move(new_trigger) } {}

bool TriggeredModifier::parse_scripts(DefinitionManager const& definition_manager) {
	return trigger.parse_script(false, definition_manager);
}

ModifierInstance::ModifierInstance(Modifier const& new_modifier, Date new_expiry_date)
	: modifier { &new_modifier }, expiry_date { new_expiry_date } {}

bool ModifierManager::add_modifier_effect(
	std::string_view identifier, bool positive_good, ModifierEffect::format_t format, std::string_view localisation_key
) {
	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}
	return modifier_effects.add_item({ std::move(identifier), positive_good, format, localisation_key });
}

bool ModifierManager::setup_modifier_effects() {
	bool ret = true;

	using enum ModifierEffect::format_t;
	/* Tech/inventions only */
	ret &= add_modifier_effect("cb_creation_speed", true, PROPORTION_DECIMAL, "CB_MANUFACTURE_TECH");
	ret &= add_modifier_effect("combat_width", false);
	ret &= add_modifier_effect("plurality", true, PERCENTAGE_DECIMAL, "TECH_PLURALITY");
	ret &= add_modifier_effect("pop_growth", true, PROPORTION_DECIMAL, "TECH_POP_GROWTH");
	ret &= add_modifier_effect("regular_experience_level", true, RAW_DECIMAL, "REGULAR_EXP_TECH");
	ret &= add_modifier_effect("reinforce_rate", true, PROPORTION_DECIMAL, "REINFORCE_TECH");
	ret &= add_modifier_effect("seperatism", false, PROPORTION_DECIMAL, "SEPARATISM_TECH"); // paradox typo
	ret &= add_modifier_effect("shared_prestige", true, RAW_DECIMAL, "SHARED_PRESTIGE_TECH");
	ret &= add_modifier_effect("tax_eff", true, PROPORTION_DECIMAL, "TECH_TAX_EFF");

	/* Country Modifier Effects */
	ret &= add_modifier_effect("administrative_efficiency", true);
	ret &= add_modifier_effect(
		"administrative_efficiency_modifier", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("administrative_efficiency")
	);
	ret &= add_modifier_effect("artisan_input", false);
	ret &= add_modifier_effect("artisan_output", true);
	ret &= add_modifier_effect("artisan_throughput", true);
	ret &= add_modifier_effect("badboy", false, RAW_DECIMAL);
	ret &= add_modifier_effect("cb_generation_speed_modifier", true);
	ret &= add_modifier_effect(
		"civilization_progress_modifier", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("civilization_progress")
	);
	ret &= add_modifier_effect("colonial_life_rating", false, INT, "COLONIAL_LIFE_TECH");
	ret &= add_modifier_effect("colonial_migration", true, PROPORTION_DECIMAL, "COLONIAL_MIGRATION_TECH");
	ret &= add_modifier_effect("colonial_points", true, INT, "COLONIAL_POINTS_TECH");
	ret &= add_modifier_effect("colonial_prestige", true, PROPORTION_DECIMAL, "COLONIAL_PRESTIGE_MODIFIER_TECH");
	ret &= add_modifier_effect("core_pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("core_pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("dig_in_cap", true, INT, "DIGIN_FROM_TECH");
	ret &= add_modifier_effect("diplomatic_points", true, PROPORTION_DECIMAL, "DIPLOMATIC_POINTS_TECH");
	ret &= add_modifier_effect(
		"diplomatic_points_modifier", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("diplopoints_gain")
	);
	ret &= add_modifier_effect("education_efficiency", true);
	ret &= add_modifier_effect(
		"education_efficiency_modifier", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("education_efficiency")
	);
	ret &= add_modifier_effect("factory_cost", false);
	ret &= add_modifier_effect("factory_input", false);
	ret &= add_modifier_effect("factory_maintenance", false);
	ret &= add_modifier_effect("factory_output", true);
	ret &= add_modifier_effect("factory_owner_cost", false);
	ret &= add_modifier_effect("factory_throughput", true);
	ret &= add_modifier_effect(
		"global_assimilation_rate", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("assimilation_rate")
	);
	ret &= add_modifier_effect(
		"global_immigrant_attract", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("immigant_attract")
	);
	ret &= add_modifier_effect("global_pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("global_pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect(
		"global_population_growth", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("population_growth")
	);
	ret &= add_modifier_effect("goods_demand", false);
	ret &= add_modifier_effect("import_cost", false);
	ret &= add_modifier_effect("increase_research", true, PROPORTION_DECIMAL, "INC_RES_TECH");
	ret &= add_modifier_effect("influence", true, PROPORTION_DECIMAL, "TECH_GP_INFLUENCE");
	ret &= add_modifier_effect(
		"influence_modifier", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("greatpower_influence_gain")
	);
	ret &= add_modifier_effect("issue_change_speed", true);
	ret &= add_modifier_effect(
		"land_attack_modifier", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("land_attack")
	);
	ret &= add_modifier_effect("land_attrition", false, PROPORTION_DECIMAL, "LAND_ATTRITION_TECH");
	ret &= add_modifier_effect(
		"land_defense_modifier", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("land_defense")
	);
	ret &= add_modifier_effect("land_organisation", true);
	ret &= add_modifier_effect("land_unit_start_experience", true, RAW_DECIMAL);
	ret &= add_modifier_effect("leadership", true, RAW_DECIMAL, "LEADERSHIP");
	ret &= add_modifier_effect(
		"leadership_modifier", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("global_leadership_modifier")
	);
	ret &= add_modifier_effect("literacy_con_impact", false);
	ret &= add_modifier_effect("loan_interest", false);
	ret &= add_modifier_effect(
		"max_loan_modifier", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("max_loan_amount")
	);
	ret &= add_modifier_effect("max_military_spending", true);
	ret &= add_modifier_effect("max_national_focus", true, INT, "TECH_MAX_FOCUS");
	ret &= add_modifier_effect("max_social_spending", true);
	ret &= add_modifier_effect("max_tariff", true);
	ret &= add_modifier_effect("max_tax", true);
	ret &= add_modifier_effect("max_war_exhaustion", true, PERCENTAGE_DECIMAL, "MAX_WAR_EXHAUSTION");
	ret &= add_modifier_effect("military_tactics", true, PROPORTION_DECIMAL, "MIL_TACTICS_TECH");
	ret &= add_modifier_effect("min_military_spending", true);
	ret &= add_modifier_effect("min_social_spending", true);
	ret &= add_modifier_effect("min_tariff", true);
	ret &= add_modifier_effect("min_tax", true);
	ret &= add_modifier_effect(
		"minimum_wage", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("minimun_wage")
	);
	ret &= add_modifier_effect("mobilisation_economy_impact", false);
	ret &= add_modifier_effect("mobilisation_size", true);
	ret &= add_modifier_effect("mobilization_impact", false);
	ret &= add_modifier_effect(
		"naval_attack_modifier", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("naval_attack")
	);
	ret &= add_modifier_effect("naval_attrition", false, PROPORTION_DECIMAL, "NAVAL_ATTRITION_TECH");
	ret &= add_modifier_effect(
		"naval_defense_modifier", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("naval_defense")
	);
	ret &= add_modifier_effect("naval_organisation", true);
	ret &= add_modifier_effect("naval_unit_start_experience", true, RAW_DECIMAL);
	ret &= add_modifier_effect("non_accepted_pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("non_accepted_pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("org_regain", true);
	ret &= add_modifier_effect("pension_level", true);
	ret &= add_modifier_effect("permanent_prestige", true, RAW_DECIMAL, "PERMANENT_PRESTIGE_TECH");
	ret &= add_modifier_effect("political_reform_desire", false);
	ret &= add_modifier_effect("poor_savings_modifier", true);
	ret &= add_modifier_effect("prestige", true, RAW_DECIMAL);
	ret &= add_modifier_effect("reinforce_speed", true);
	ret &= add_modifier_effect("research_points", true, RAW_DECIMAL);
	ret &= add_modifier_effect("research_points_modifier", true);
	ret &= add_modifier_effect("research_points_on_conquer", true);
	ret &= add_modifier_effect("rgo_output", true);
	ret &= add_modifier_effect("rgo_throughput", true);
	ret &= add_modifier_effect("ruling_party_support", true);
	ret &= add_modifier_effect(
		"self_unciv_economic_modifier", false, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("self_unciv_economic")
	);
	ret &= add_modifier_effect(
		"self_unciv_military_modifier", false, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("self_unciv_military")
	);
	ret &= add_modifier_effect("social_reform_desire", false);
	ret &= add_modifier_effect("soldier_to_pop_loss", true, PROPORTION_DECIMAL, "SOLDIER_TO_POP_LOSS_TECH");
	ret &= add_modifier_effect("supply_consumption", false);
	ret &= add_modifier_effect("supply_range", true, PROPORTION_DECIMAL, "SUPPLY_RANGE_TECH");
	ret &= add_modifier_effect("suppression_points_modifier", true, PROPORTION_DECIMAL, "SUPPRESSION_TECH");
	ret &= add_modifier_effect(
		"tariff_efficiency_modifier", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("tariff_efficiency")
	);
	ret &= add_modifier_effect("tax_efficiency", true);
	ret &= add_modifier_effect("unemployment_benefit", true);
	ret &= add_modifier_effect(
		"unciv_economic_modifier", false, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("unciv_economic")
	);
	ret &= add_modifier_effect(
		"unciv_military_modifier", false, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("unciv_military")
	);
	ret &= add_modifier_effect("unit_recruitment_time", false);
	ret &= add_modifier_effect("war_exhaustion", false, PROPORTION_DECIMAL, "WAR_EXHAUST_BATTLES");

	/* Province Modifier Effects */
	ret &= add_modifier_effect("assimilation_rate", true);
	ret &= add_modifier_effect("boost_strongest_party", false);
	ret &= add_modifier_effect("farm_rgo_eff", true, PROPORTION_DECIMAL, "TECH_FARM_OUTPUT");
	ret &= add_modifier_effect(
		"farm_rgo_size", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("farm_size")
	);
	ret &= add_modifier_effect(
		"immigrant_attract", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("immigant_attract")
	);
	ret &= add_modifier_effect(
		"immigrant_push", false, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("immigant_push")
	);
	ret &= add_modifier_effect("life_rating", true);
	ret &= add_modifier_effect(
		"local_artisan_input", false, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("artisan_input")
	);
	ret &= add_modifier_effect(
		"local_artisan_output", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("artisan_output")
	);
	ret &= add_modifier_effect(
		"local_artisan_throughput", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("artisan_throughput")
	);
	ret &= add_modifier_effect(
		"local_factory_input", false, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("factory_input")
	);
	ret &= add_modifier_effect(
		"local_factory_output", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("factory_output")
	);
	ret &= add_modifier_effect(
		"local_factory_throughput", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("factory_throughput")
	);
	ret &= add_modifier_effect("local_repair", true);
	ret &= add_modifier_effect(
		"local_rgo_output", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("rgo_output")
	);
	ret &= add_modifier_effect(
		"local_rgo_throughput", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("rgo_throughput")
	);
	ret &= add_modifier_effect(
		"local_ruling_party_support", true, PROPORTION_DECIMAL,
		make_default_modifier_effect_localisation_key("ruling_party_support")
	);
	ret &= add_modifier_effect("local_ship_build", false);
	ret &= add_modifier_effect("max_attrition", false, RAW_DECIMAL);
	ret &= add_modifier_effect("mine_rgo_eff", true, PROPORTION_DECIMAL, "TECH_MINE_OUTPUT");
	ret &= add_modifier_effect(
		"mine_rgo_size", true, PROPORTION_DECIMAL, make_default_modifier_effect_localisation_key("mine_size")
	);
	ret &= add_modifier_effect("movement_cost", false);
	ret &= add_modifier_effect("number_of_voters", false);
	ret &= add_modifier_effect("pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("population_growth", true);
	ret &= add_modifier_effect("supply_limit", true, RAW_DECIMAL);

	/* Military Modifier Effects */
	ret &= add_modifier_effect("attack", true, INT, "TRAIT_ATTACK");
	ret &= add_modifier_effect("attrition", false, RAW_DECIMAL, "ATTRITION");
	ret &= add_modifier_effect("defence", true, INT, "TRAIT_DEFEND");
	ret &= add_modifier_effect("experience", true, PROPORTION_DECIMAL, "TRAIT_EXPERIENCE");
	ret &= add_modifier_effect("morale", true, PROPORTION_DECIMAL, "TRAIT_MORALE");
	ret &= add_modifier_effect("organisation", true, PROPORTION_DECIMAL, "TRAIT_ORGANISATION");
	ret &= add_modifier_effect("reconnaissance", true, PROPORTION_DECIMAL, "TRAIT_RECONAISSANCE");
	ret &= add_modifier_effect("reliability", true, RAW_DECIMAL, "TRAIT_RELIABILITY");
	ret &= add_modifier_effect("speed", true, PROPORTION_DECIMAL, "TRAIT_SPEED");

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
	if (identifier.empty()) {
		Logger::error("Invalid event modifier effect identifier - empty!");
		return false;
	}

	return event_modifiers.add_item(
		{ identifier, std::move(values), Modifier::modifier_type_t::EVENT, icon }, duplicate_warning_callback
	);
}

bool ModifierManager::load_event_modifiers(ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		event_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			ModifierValue modifier_value;
			IconModifier::icon_t icon = 0;
			bool ret = expect_modifier_value_and_keys(
				move_variable_callback(modifier_value),
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
	if (identifier.empty()) {
		Logger::error("Invalid static modifier effect identifier - empty!");
		return false;
	}

	return static_modifiers.add_item(
		{ identifier, std::move(values), Modifier::modifier_type_t::STATIC }, duplicate_warning_callback
	);
}

bool ModifierManager::load_static_modifiers(ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		static_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			ModifierValue modifier_value;
			bool ret = expect_modifier_value(move_variable_callback(modifier_value))(value);
			ret &= add_static_modifier(key, std::move(modifier_value));
			return ret;
		}
	)(root);

	lock_static_modifiers();

	return ret;
}

bool ModifierManager::add_triggered_modifier(
	std::string_view identifier, ModifierValue&& values, IconModifier::icon_t icon, ConditionScript&& trigger
) {
	if (identifier.empty()) {
		Logger::error("Invalid triggered modifier effect identifier - empty!");
		return false;
	}

	return triggered_modifiers.add_item(
		{ identifier, std::move(values), Modifier::modifier_type_t::TRIGGERED, icon, std::move(trigger) },
		duplicate_warning_callback
	);
}

bool ModifierManager::load_triggered_modifiers(ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		triggered_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			ModifierValue modifier_value;
			IconModifier::icon_t icon = 0;
			ConditionScript trigger { scope_t::COUNTRY, scope_t::COUNTRY, scope_t::NO_SCOPE };

			bool ret = expect_modifier_value_and_keys(
				move_variable_callback(modifier_value),
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
	ModifierValue& modifier, key_value_callback_t default_callback, ModifierEffectValidator auto effect_validator
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
	callback_t<ModifierValue&&> modifier_callback, key_value_callback_t default_callback,
	ModifierEffectValidator auto effect_validator
) const {
	return [this, modifier_callback, default_callback, effect_validator](ast::NodeCPtr root) -> bool {
		ModifierValue modifier;
		bool ret = expect_dictionary_reserve_length(
			modifier.values,
			_modifier_effect_callback(modifier, default_callback, effect_validator)
		)(root);
		ret &= modifier_callback(std::move(modifier));
		return ret;
	};
}

node_callback_t ModifierManager::expect_validated_modifier_value(
	callback_t<ModifierValue&&> modifier_callback, ModifierEffectValidator auto effect_validator
) const {
	return expect_validated_modifier_value_and_default(modifier_callback, key_value_invalid_callback, effect_validator);
}

node_callback_t ModifierManager::expect_modifier_value_and_default(
	callback_t<ModifierValue&&> modifier_callback, key_value_callback_t default_callback
) const {
	return expect_validated_modifier_value_and_default(modifier_callback, default_callback, [](ModifierEffect const&) -> bool {
		return true;
	});
}

node_callback_t ModifierManager::expect_modifier_value(callback_t<ModifierValue&&> modifier_callback) const {
	return expect_modifier_value_and_default(modifier_callback, key_value_invalid_callback);
}

node_callback_t ModifierManager::expect_whitelisted_modifier_value_and_default(
	callback_t<ModifierValue&&> modifier_callback, string_set_t const& whitelist, key_value_callback_t default_callback
) const {
	return expect_validated_modifier_value_and_default(
		modifier_callback, default_callback,
		[&whitelist](ModifierEffect const& effect) -> bool {
			return whitelist.contains(effect.get_identifier());
		}
	);
}

node_callback_t ModifierManager::expect_whitelisted_modifier_value(
	callback_t<ModifierValue&&> modifier_callback, string_set_t const& whitelist
) const {
	return expect_whitelisted_modifier_value_and_default(modifier_callback, whitelist, key_value_invalid_callback);
}

node_callback_t ModifierManager::expect_modifier_value_and_key_map_and_default(
	callback_t<ModifierValue&&> modifier_callback, key_value_callback_t default_callback, key_map_t&& key_map
) const {
	return [this, modifier_callback, default_callback, key_map = std::move(key_map)](ast::NodeCPtr node) mutable -> bool {
		bool ret = expect_modifier_value_and_default(
			modifier_callback,
			dictionary_keys_callback(key_map, default_callback)
		)(node);
		ret &= check_key_map_counts(key_map);
		return ret;
	};
}

node_callback_t ModifierManager::expect_modifier_value_and_key_map(
	callback_t<ModifierValue&&> modifier_callback, key_map_t&& key_map
) const {
	return expect_modifier_value_and_key_map_and_default(modifier_callback, key_value_invalid_callback, std::move(key_map));
}

namespace OpenVic { // so the compiler shuts up
	std::ostream& operator<<(std::ostream& stream, ModifierValue const& value) {
		for (ModifierValue::effect_map_t::value_type const& effect : value.values) {
			stream << effect.first << ": " << effect.second << "\n";
		}
		return stream;
	}
}
