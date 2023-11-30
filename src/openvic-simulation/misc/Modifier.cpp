#include "Modifier.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ModifierEffect::ModifierEffect(std::string_view new_identifier, bool new_positive_good, format_t new_format)
	: HasIdentifier { new_identifier }, positive_good { new_positive_good }, format { new_format } {}

ModifierValue::ModifierValue() = default;
ModifierValue::ModifierValue(effect_map_t&& new_values) : values { std::move(new_values) } {}
ModifierValue::ModifierValue(ModifierValue const&) = default;
ModifierValue::ModifierValue(ModifierValue&&) = default;

ModifierValue& ModifierValue::operator=(ModifierValue const&) = default;
ModifierValue& ModifierValue::operator=(ModifierValue&&) = default;

void ModifierValue::trim() {
	std::erase_if(values, [](effect_map_t::value_type const& value) -> bool {
		return value.second == fixed_point_t::_0();
	});
}

size_t ModifierValue::get_effect_count() const {
	return values.size();
}

fixed_point_t ModifierValue::get_effect(ModifierEffect const* effect, bool* successful) {
	const effect_map_t::const_iterator it = values.find(effect);
	if (it != values.end()) {
		if (successful != nullptr) {
			*successful = true;
		}
		return it->second;
	}
	if (successful != nullptr) {
		*successful = false;
	}
	return fixed_point_t::_0();
}

bool ModifierValue::has_effect(ModifierEffect const* effect) const {
	return values.contains(effect);
}

ModifierValue& ModifierValue::operator+=(ModifierValue const& right) {
	for (effect_map_t::value_type const& value : right.values) {
		values[value.first] += value.second;
	}
	return *this;
}

ModifierValue ModifierValue::operator+(ModifierValue const& right) const {
	ModifierValue ret = *this;
	return ret += right;
}

ModifierValue ModifierValue::operator-() const {
	ModifierValue ret = *this;
	for (effect_map_t::value_type& value : ret.values) {
		value.second = -value.second;
	}
	return ret;
}

ModifierValue& ModifierValue::operator-=(ModifierValue const& right) {
	for (effect_map_t::value_type const& value : right.values) {
		values[value.first] -= value.second;
	}
	return *this;
}

ModifierValue ModifierValue::operator-(ModifierValue const& right) const {
	ModifierValue ret = *this;
	return ret -= right;
}

Modifier::Modifier(std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon)
	: HasIdentifier { new_identifier }, ModifierValue { std::move(new_values) }, icon { new_icon } {}

ModifierInstance::ModifierInstance(Modifier const& modifier, Date expiry_date)
	: modifier { modifier }, expiry_date { expiry_date } {}

ModifierManager::ModifierManager() : modifier_effects { "modifier effects" }, event_modifiers { "event modifiers" } {}

bool ModifierManager::add_modifier_effect(std::string_view identifier, bool positive_good, ModifierEffect::format_t format) {
	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}
	return modifier_effects.add_item(
		std::make_unique<ModifierEffect>(std::move(identifier), std::move(positive_good), std::move(format))
	);
}

bool ModifierManager::add_event_modifier(std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon) {
	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}
	if (icon <= 0) {
		Logger::error("Invalid modifier icon for ", identifier, ": ", icon);
		return false;
	}
	return event_modifiers.add_item({ identifier, std::move(values), icon }, duplicate_warning_callback);
}

bool ModifierManager::setup_modifier_effects() {
	bool ret = true;

	using enum ModifierEffect::format_t;

	/* Country Modifier Effects */
	ret &= add_modifier_effect("administrative_efficiency_modifier", true);
	ret &= add_modifier_effect("badboy", false, RAW_DECIMAL);
	ret &= add_modifier_effect("cb_generation_speed_modifier", true);
	ret &= add_modifier_effect("core_pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("core_pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("diplomatic_points_modifier", true);
	ret &= add_modifier_effect("education_efficiency_modifier", true);
	ret &= add_modifier_effect("factory_cost", false);
	ret &= add_modifier_effect("factory_input", false);
	ret &= add_modifier_effect("factory_output", true);
	ret &= add_modifier_effect("factory_owner_cost", false);
	ret &= add_modifier_effect("factory_throughput", true);
	ret &= add_modifier_effect("global_assimilation_rate", true);
	ret &= add_modifier_effect("global_immigrant_attract", true);
	ret &= add_modifier_effect("global_pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("global_pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("global_population_growth", true);
	ret &= add_modifier_effect("goods_demand", false);
	ret &= add_modifier_effect("import_cost", false);
	ret &= add_modifier_effect("influence_modifier", true);
	ret &= add_modifier_effect("issue_change_speed", true);
	ret &= add_modifier_effect("land_organisation", true);
	ret &= add_modifier_effect("land_unit_start_experience", true, RAW_DECIMAL);
	ret &= add_modifier_effect("leadership_modifier", true);
	ret &= add_modifier_effect("loan_interest", false);
	ret &= add_modifier_effect("max_loan_modifier", true);
	ret &= add_modifier_effect("max_military_spending", true);
	ret &= add_modifier_effect("max_social_spending", true);
	ret &= add_modifier_effect("max_tariff", true);
	ret &= add_modifier_effect("max_tax", true);
	ret &= add_modifier_effect("middle_income_modifier", true);
	ret &= add_modifier_effect("middle_life_needs", true);
	ret &= add_modifier_effect("middle_everyday_needs", true);
	ret &= add_modifier_effect("middle_luxury_needs", true);
	ret &= add_modifier_effect("middle_vote", true);
	ret &= add_modifier_effect("min_military_spending", true);
	ret &= add_modifier_effect("min_social_spending", true);
	ret &= add_modifier_effect("min_tariff", true);
	ret &= add_modifier_effect("min_tax", true);
	ret &= add_modifier_effect("mobilisation_economy_impact", false);
	ret &= add_modifier_effect("mobilisation_impact", false);
	ret &= add_modifier_effect("mobilisation_size", true);
	ret &= add_modifier_effect("naval_organisation", true);
	ret &= add_modifier_effect("naval_unit_start_experience", true, RAW_DECIMAL);
	ret &= add_modifier_effect("non_accepted_pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("non_accepted_pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("org_regain", true);
	ret &= add_modifier_effect("political_reform_desire", false);
	ret &= add_modifier_effect("poor_income_modifier", true);
	ret &= add_modifier_effect("poor_life_needs", true);
	ret &= add_modifier_effect("poor_everyday_needs", true);
	ret &= add_modifier_effect("poor_luxury_needs", true);
	ret &= add_modifier_effect("poor_vote", true);
	ret &= add_modifier_effect("prestige", true, RAW_DECIMAL);
	ret &= add_modifier_effect("research_points", true, RAW_DECIMAL);
	ret &= add_modifier_effect("research_points_modifier", true);
	ret &= add_modifier_effect("research_points_on_conquer", true);
	ret &= add_modifier_effect("rgo_output", true);
	ret &= add_modifier_effect("RGO_output", true);
	ret &= add_modifier_effect("rgo_throughput", true);
	ret &= add_modifier_effect("RGO_throughput", true);
	ret &= add_modifier_effect("rich_income_modifier", true);
	ret &= add_modifier_effect("rich_life_needs", true);
	ret &= add_modifier_effect("rich_everyday_needs", true);
	ret &= add_modifier_effect("rich_luxury_needs", true);
	ret &= add_modifier_effect("rich_vote", true);
	ret &= add_modifier_effect("ruling_party_support", true);
	ret &= add_modifier_effect("social_reform_desire", false);
	ret &= add_modifier_effect("supply_consumption", false);
	ret &= add_modifier_effect("tax_efficiency", true);
	ret &= add_modifier_effect("unit_start_experience", true, RAW_DECIMAL);
	ret &= add_modifier_effect("war_exhaustion", false);
	ret &= add_modifier_effect("reinforce_rate", true);
	ret &= add_modifier_effect("colonial_migration", true);
	ret &= add_modifier_effect("supply_range", true);
	ret &= add_modifier_effect("colonial_points", true, INT);
	ret &= add_modifier_effect("diplomatic_points", true);
	ret &= add_modifier_effect("cb_creation_speed", true); //seemingly works the same way as cb_generation_speed_modifier
	ret &= add_modifier_effect("education_efficiency", true);
	ret &= add_modifier_effect("increase_research", true);
	ret &= add_modifier_effect("influence", true);
	ret &= add_modifier_effect("administrative_efficiency", true);
	ret &= add_modifier_effect("tax_eff", true);	
	ret &= add_modifier_effect("military_tactics", true);
	ret &= add_modifier_effect("dig_in_cap", true, INT);
	ret &= add_modifier_effect("max_national_focus", true, INT);
	ret &= add_modifier_effect("regular_experience_level", true, RAW_DECIMAL);

	/* Province Modifier Effects */
	ret &= add_modifier_effect("assimilation_rate", true);
	ret &= add_modifier_effect("immigrant_attract", true);
	ret &= add_modifier_effect("immigrant_push", false);
	ret &= add_modifier_effect("life_rating", true);
	ret &= add_modifier_effect("local_factory_input", true);
	ret &= add_modifier_effect("local_factory_output", true);
	ret &= add_modifier_effect("local_factory_throughput", true);
	ret &= add_modifier_effect("local_repair", true);
	ret &= add_modifier_effect("local_rgo_output", true);
	ret &= add_modifier_effect("local_RGO_output", true);
	ret &= add_modifier_effect("local_RGO_throughput", true);
	ret &= add_modifier_effect("local_ruling_party_support", true);
	ret &= add_modifier_effect("local_ship_build", false);
	ret &= add_modifier_effect("pop_consciousness_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("pop_militancy_modifier", false, RAW_DECIMAL);
	ret &= add_modifier_effect("population_growth", true);
	ret &= add_modifier_effect("flashpoint_tension", false);
	ret &= add_modifier_effect("farm_rgo_eff", true);
	ret &= add_modifier_effect("farm_RGO_eff", true);
	ret &= add_modifier_effect("farm_rgo_size", true);
	ret &= add_modifier_effect("farm_RGO_size", true);
	ret &= add_modifier_effect("mine_rgo_eff", true);
	ret &= add_modifier_effect("mine_RGO_eff", true);
	ret &= add_modifier_effect("mine_rgo_size", true);
	ret &= add_modifier_effect("mine_RGO_size", true);
	ret &= add_modifier_effect("movement_cost", false);
	ret &= add_modifier_effect("railroads", true); // capitalist likelihood for railroads vs factories
	ret &= add_modifier_effect("supply_limit", true, RAW_DECIMAL);

	/* Military Modifier Effects */
	ret &= add_modifier_effect("attack", true, INT);
	ret &= add_modifier_effect("attrition", false, RAW_DECIMAL);
	ret &= add_modifier_effect("combat_width", false);
	ret &= add_modifier_effect("defence", true, INT);
	ret &= add_modifier_effect("experience", true);
	ret &= add_modifier_effect("morale", true);
	ret &= add_modifier_effect("organisation", true);
	ret &= add_modifier_effect("reconnaissance", true);
	ret &= add_modifier_effect("reliability", true, RAW_DECIMAL);
	ret &= add_modifier_effect("speed", true);

	return ret;
}

void ModifierManager::register_complex_modifier(std::string_view identifier) {
	complex_modifiers.emplace(identifier);
}

bool ModifierManager::load_crime_modifiers(ast::NodeCPtr root) {
	// TODO - DEV TASK: read crime modifiers
	return true;
}

bool ModifierManager::load_event_modifiers(ast::NodeCPtr root) {
	// TODO - DEV TASK: read event modifiers - example framework below
	return true;
	/*return expect_dictionary_reserve_length(
		event_modifiers,
		[this](std::string_view key, ast::NodeCPtr value) -> bool {
			ModifierValue modifier_value;
			Modifier::icon_t icon = 0;
			bool ret = expect_modifier_value_and_keys(
				move_variable_callback(modifier_value),
				"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon))
			)(value);
			ret &= add_event_modifier(key, std::move(modifier_value), icon);
			return ret;
		}
	)(root);*/
}

bool ModifierManager::load_static_modifiers(ast::NodeCPtr root) {
	// TODO - DEV TASK: read static modifiers
	return true;
}

bool ModifierManager::load_triggered_modifiers(ast::NodeCPtr root) {
	// TODO - DEV TASK: read triggered modifiers
	return true;
}

key_value_callback_t ModifierManager::_modifier_effect_callback(
	ModifierValue& modifier, key_value_callback_t default_callback, ModifierEffectValidator auto effect_validator
) const {
	std::function<bool(ModifierEffect const*, ast::NodeCPtr)> add_modifier_cb = [this, &modifier, effect_validator](ModifierEffect const* effect, ast::NodeCPtr value) -> bool {
		if (effect_validator(*effect)) {
			if (!modifier.values.contains(effect)) {
				return expect_fixed_point(assign_variable_callback(modifier.values[effect]))(value);
			} else {
				Logger::error("Duplicate modifier effect: ", effect->get_identifier());
				return false;
			}
		} else {
			Logger::error("Failed to validate modifier effect: ", effect->get_identifier());
			return false;
		}
	};

	return [this, &modifier, default_callback, effect_validator, add_modifier_cb](std::string_view key, ast::NodeCPtr value) -> bool {
		ModifierEffect const* effect = get_modifier_effect_by_identifier(key);
		if (effect != nullptr && value->is_type<ast::IdentifierNode>()) {
			return add_modifier_cb(effect, value);
		} else if (complex_modifiers.contains(key) && value->is_derived_from<ast::AbstractListNode>()) {
			return expect_dictionary([this, &key, &add_modifier_cb, &default_callback](std::string_view identifier, ast::NodeCPtr node) -> bool {
				std::string flat_identifier = std::string(key);
				flat_identifier += "_";
				flat_identifier += identifier;
				ModifierEffect const* effect = get_modifier_effect_by_identifier(flat_identifier);
				if(effect != nullptr) {
					return add_modifier_cb(effect, node);
				} else {
					Logger::error("Could not find flattened modifier: ", flat_identifier);
					return false;
				}
			})(value);
		} else return default_callback(key, value);
	};
}

node_callback_t ModifierManager::expect_validated_modifier_value_and_default(
	callback_t<ModifierValue&&> modifier_callback, key_value_callback_t default_callback,
	ModifierEffectValidator auto effect_validator
) const {
	return [this, modifier_callback, default_callback, effect_validator](ast::NodeCPtr root) -> bool {
		ModifierValue modifier;
		bool ret = expect_dictionary(_modifier_effect_callback(modifier, default_callback, effect_validator))(root);
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
