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

void ModifierSum::add_modifier(Modifier const& modifier, modifier_source_t source, fixed_point_t multiplier) {
	modifiers.emplace_back(&modifier, multiplier, source);
	value_sum.multiply_add(modifier, multiplier);
}

void ModifierSum::add_modifier_sum(ModifierSum const& modifier_sum) {
	modifiers.insert(modifiers.end(), modifier_sum.modifiers.begin(), modifier_sum.modifiers.end());
	value_sum += modifier_sum.value_sum;
}

void ModifierSum::add_modifier_sum_exclude_source(ModifierSum const& modifier_sum, modifier_source_t const& excluded_source) {
	for (modifier_entry_t const& modifier_entry : modifier_sum.modifiers) {
		if (modifier_entry.source != excluded_source) {
			add_modifier(*modifier_entry.modifier, modifier_entry.source, modifier_entry.multiplier);
		}
	}
}

// TODO - include value_sum[effect] in result? Early return if lookup in value_sum fails?
std::vector<ModifierSum::modifier_entry_t> ModifierSum::get_contributing_modifiers(
	ModifierEffect const& effect
) const {
	std::vector<modifier_entry_t> ret;

	for (modifier_entry_t const& modifier_entry : modifiers) {
		bool effect_found = false;
		const fixed_point_t value = modifier_entry.modifier->get_effect(effect, &effect_found);

		if (effect_found) {
			ret.push_back(modifier_entry);
		}
	}

	return ret;
}
