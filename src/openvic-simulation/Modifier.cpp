#include "Modifier.hpp"

using namespace OpenVic;

ModifierEffect::ModifierEffect(const std::string_view new_identifier, bool new_positive_good)
	: HasIdentifier { new_identifier }, positive_good { new_positive_good } {}

bool ModifierEffect::get_positive_good() const {
	return positive_good;
}

ModifierValue::ModifierValue() = default;
ModifierValue::ModifierValue(ModifierValue const&) = default;
ModifierValue::ModifierValue(ModifierValue&&) = default;

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
		if (successful != nullptr) *successful = true;
		return it->second;
	}
	if (successful != nullptr) *successful = false;
	return fixed_point_t::_0();
}

bool ModifierValue::has_effect(ModifierEffect const* effect) const {
	return values.find(effect) != values.end();
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

std::ostream& OpenVic::operator<<(std::ostream& stream, ModifierValue const& value) {
	for (ModifierValue::effect_map_t::value_type const& effect : value.values) {
		stream << effect.first << ": " << effect.second << "\n";
	}
	return stream;
}

Modifier::Modifier(const std::string_view new_identifier, ModifierValue&& new_values, icon_t new_icon)
	: HasIdentifier { new_identifier }, ModifierValue { std::move(new_values) }, icon { new_icon } {}

Modifier::icon_t Modifier::get_icon() const {
	return icon;
}

Modifier const& ModifierInstance::get_modifier() const {
	return modifier;
}

Date const& ModifierInstance::get_expiry_date() const {
	return expiry_date;
}

ModifierManager::ModifierManager()
	: modifier_effects { "modifier effects"}, modifiers { "modifiers" } {}

bool ModifierManager::add_modifier_effect(const std::string_view identifier, bool positive_good) {
	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}
	return modifier_effects.add_item({ identifier, positive_good });
}

bool ModifierManager::add_modifier(const std::string_view identifier, ModifierValue&& values, Modifier::icon_t icon) {
	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}
	if (icon <= 0) {
		Logger::error("Invalid modifier icon for ", identifier, ": ", icon);
		return false;
	}
	return modifiers.add_item({ identifier, std::move(values), icon });
}
