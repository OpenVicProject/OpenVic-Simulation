#include "ModifierSum.hpp"

#include "openvic-simulation/modifier/Modifier.hpp"

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

fixed_point_t ModifierSum::get_effect_nullcheck(ModifierEffect const* effect, bool* effect_found) const {
	return value_sum.get_effect_nullcheck(effect, effect_found);
}

bool ModifierSum::has_effect(ModifierEffect const& effect) const {
	return value_sum.has_effect(effect);
}

void ModifierSum::add_modifier(
	Modifier const& modifier, modifier_source_t const& source, fixed_point_t multiplier, ModifierEffect::target_t targets
) {
	using enum ModifierEffect::target_t;

	if (multiplier != fixed_point_t::_0() && targets != NO_TARGETS) {
		modifiers.emplace_back(&modifier, multiplier, source, targets);
		value_sum.multiply_add_filter(modifier, multiplier, targets);
	}
}

void ModifierSum::add_modifier_nullcheck(
	Modifier const* modifier, modifier_source_t const& source, fixed_point_t multiplier, ModifierEffect::target_t targets
) {
	if (modifier != nullptr) {
		add_modifier(*modifier, source, multiplier, targets);
	}
}

void ModifierSum::add_modifier_sum(ModifierSum const& modifier_sum) {
	modifiers.insert(modifiers.end(), modifier_sum.modifiers.begin(), modifier_sum.modifiers.end());
	value_sum += modifier_sum.value_sum;
}

void ModifierSum::add_modifier_sum_filter_targets(ModifierSum const& modifier_sum, ModifierEffect::target_t targets) {
	using enum ModifierEffect::target_t;

	for (modifier_entry_t const& modifier_entry : modifier_sum.modifiers) {
		ModifierEffect::target_t new_targets = modifier_entry.targets & targets;

		if (new_targets != NO_TARGETS) {
			add_modifier(*modifier_entry.modifier, modifier_entry.source, modifier_entry.multiplier, new_targets);
		}
	}
}

void ModifierSum::add_modifier_sum_exclude_source(ModifierSum const& modifier_sum, modifier_source_t const& excluded_source) {
	for (modifier_entry_t const& modifier_entry : modifier_sum.modifiers) {
		if (modifier_entry.source != excluded_source) {
			add_modifier(*modifier_entry.modifier, modifier_entry.source, modifier_entry.multiplier, modifier_entry.targets);
		}
	}
}

// TODO - include value_sum[effect] in result? Early return if lookup in value_sum fails?

void ModifierSum::push_contributing_modifiers(
	ModifierEffect const& effect, std::vector<modifier_entry_t>& contributions
) const {
	using enum ModifierEffect::target_t;

	for (modifier_entry_t const& modifier_entry : modifiers) {
		if ((modifier_entry.targets & effect.get_targets()) != NO_TARGETS) {
			bool effect_found = false;
			const fixed_point_t value = modifier_entry.modifier->get_effect(effect, &effect_found);

			if (effect_found) {
				contributions.push_back(modifier_entry);
			}
		}
	}
}

std::vector<ModifierSum::modifier_entry_t> ModifierSum::get_contributing_modifiers(ModifierEffect const& effect) const {
	std::vector<modifier_entry_t> contributions;

	push_contributing_modifiers(effect, contributions);

	return contributions;
}
