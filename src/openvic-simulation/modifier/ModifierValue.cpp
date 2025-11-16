#include "ModifierValue.hpp"

#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;

void ModifierValue::trim() {
	erase_if(values, [](effect_map_t::value_type const& value) -> bool {
		return value.second == 0;
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
	return 0;
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

ModifierValue& ModifierValue::operator*=(const fixed_point_t right) {
	for (auto value : mutable_iterator(values)) {
		value.second *= right;
	}
	return *this;
}

ModifierValue ModifierValue::operator*(const fixed_point_t right) const {
	ModifierValue copy = *this;
	return copy *= right;
}

void ModifierValue::apply_exclude_targets(ModifierEffect::target_t excluded_targets) {
	using enum ModifierEffect::target_t;

	// We could test if excluded_targets is NO_TARGETS (and so we do nothing) or ALL_TARGETS (and so we clear everything),
	// but so long as this is always called with an explicit/hardcoded value then we'll never have either of those cases.
	erase_if(
		values,
		[excluded_targets](effect_map_t::value_type const& value) -> bool {
			return !ModifierEffect::excludes_targets(value.first->targets, excluded_targets);
		}
	);
}

void ModifierValue::multiply_add_exclude_targets(
	ModifierValue const& other, fixed_point_t multiplier, ModifierEffect::target_t excluded_targets
) {
	using enum ModifierEffect::target_t;

	if (multiplier == fixed_point_t::_1 && excluded_targets == NO_TARGETS) {
		*this += other;
	} else if (multiplier != 0) {
		// We could test that excluded_targets != ALL_TARGETS, but in practice it's always
		// called with an explcit/hardcoded value and so won't ever exclude everything.
		for (effect_map_t::value_type const& value : other.values) {
			if (ModifierEffect::excludes_targets(value.first->targets, excluded_targets)) {
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
