#include "ModifierValue.hpp"

#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;

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

fixed_point_t ModifierValue::get_effect_nullcheck(ModifierEffect const* effect, bool* effect_found) const {
	if (effect != nullptr) {
		return get_effect(*effect, effect_found);
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

void ModifierValue::apply_target_filter(ModifierEffect::target_t targets) {
	using enum ModifierEffect::target_t;

	erase_if(
		values,
		[targets](effect_map_t::value_type const& value) -> bool {
			return (value.first->get_targets() & targets) == NO_TARGETS;
		}
	);
}

void ModifierValue::multiply_add_filter(
	ModifierValue const& other, fixed_point_t multiplier, ModifierEffect::target_t targets
) {
	using enum ModifierEffect::target_t;

	if (multiplier == fixed_point_t::_1() && targets == ALL_TARGETS) {
		*this += other;
	} else if (multiplier != fixed_point_t::_0() && targets != NO_TARGETS) {
		for (effect_map_t::value_type const& value : other.values) {
			if ((value.first->get_targets() & targets) != NO_TARGETS) {
				values[value.first] += value.second * multiplier;
			}
		}
	}
}

namespace OpenVic { // so the compiler shuts up
	std::ostream& operator<<(std::ostream& stream, ModifierValue const& value) {
		for (ModifierValue::effect_map_t::value_type const& effect : value.values) {
			stream << effect.first << ": " << effect.second << "\n";
		}
		return stream;
	}
}
