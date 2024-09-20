#include "ModifierSum.hpp"

using namespace OpenVic;

void ModifierSum::clear() {
	modifiers.clear();
	value_sum.clear();
}

bool ModifierSum::empty() {
	return modifiers.empty();
}

fixed_point_t ModifierSum::get_effect(ModifierEffect const& effect, bool* effect_found) const {
	return value_sum.get_effect(effect, effect_found);
}

bool ModifierSum::has_effect(ModifierEffect const& effect) const {
	return value_sum.has_effect(effect);
}

void ModifierSum::add_modifier(Modifier const& modifier, fixed_point_t multiplier) {
	modifiers[&modifier] += multiplier;
	value_sum.multiply_add(modifier, multiplier);
}

void ModifierSum::add_modifier_sum(ModifierSum const& modifier_sum) {
	modifiers += modifier_sum.modifiers;
	value_sum += modifier_sum.value_sum;
}

ModifierSum& ModifierSum::operator+=(Modifier const& modifier) {
	add_modifier(modifier);
	return *this;
}

ModifierSum& ModifierSum::operator+=(ModifierSum const& modifier_sum) {
	add_modifier_sum(modifier_sum);
	return *this;
}

// TODO - include value_sum[effect] in result? Early return if lookup in value_sum fails?
std::vector<std::pair<Modifier const*, fixed_point_t>> ModifierSum::get_contributing_modifiers(
	ModifierEffect const& effect
) const {
	std::vector<std::pair<Modifier const*, fixed_point_t>> ret;

	for (auto const& [modifier, multiplier] : modifiers) {
		bool effect_found = false;
		const fixed_point_t value = modifier->get_effect(effect, &effect_found);

		if (effect_found) {
			ret.emplace_back(modifier, value * multiplier);
		}
	}

	return ret;
}
