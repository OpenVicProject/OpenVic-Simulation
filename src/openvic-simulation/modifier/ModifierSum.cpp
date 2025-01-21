#include "ModifierSum.hpp"

#include "openvic-simulation/modifier/Modifier.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"

using namespace OpenVic;

std::string_view ModifierSum::source_to_string(modifier_source_t const& source) {
	return std::visit(
		[](HasGetIdentifier auto const* has_identifier) -> std::string_view {
			return has_identifier->get_identifier();
		},
		source
	);
}

std::string ModifierSum::modifier_entry_t::to_string() const {
	return StringUtils::append_string_views(
		"[", modifier.get_identifier(), ", ", multiplier.to_string(), ", ", source_to_string(source), ", ",
		ModifierEffect::target_to_string(excluded_targets), "]"
	);
}

fixed_point_t ModifierSum::modifier_entry_t::get_modifier_effect_value(ModifierEffect const& effect) const {
	return modifier.get_effect(effect) * multiplier;
}

fixed_point_t ModifierSum::modifier_entry_t::get_modifier_effect_value_nullcheck(ModifierEffect const* effect) const {
	return effect != nullptr ? get_modifier_effect_value(*effect) : fixed_point_t::_0();
}

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
	Modifier const& modifier, modifier_source_t const& source, fixed_point_t multiplier,
	ModifierEffect::target_t excluded_targets
) {
	// We could test that excluded_targets != ALL_TARGETS, but in practice it's always
	// called with an explcit/hardcoded value and so won't ever exclude everything.
	if (multiplier != fixed_point_t::_0()) {
		modifiers.emplace_back(modifier, multiplier, source, excluded_targets);
		value_sum.multiply_add_exclude_targets(modifier, multiplier, excluded_targets);
	}
}

void ModifierSum::add_modifier_nullcheck(
	Modifier const* modifier, modifier_source_t const& source, fixed_point_t multiplier,
	ModifierEffect::target_t excluded_targets
) {
	if (modifier != nullptr) {
		add_modifier(*modifier, source, multiplier, excluded_targets);
	}
}

void ModifierSum::add_modifier_sum_exclude_targets(
	ModifierSum const& modifier_sum, ModifierEffect::target_t excluded_targets
) {
	reserve_more(modifiers, modifier_sum.modifiers.size());

	// We could test that excluded_targets != ALL_TARGETS, but in practice it's always
	// called with an explcit/hardcoded value and so won't ever exclude everything.
	for (modifier_entry_t const& modifier_entry : modifier_sum.modifiers) {
		add_modifier(
			modifier_entry.modifier,
			modifier_entry.source,
			modifier_entry.multiplier,
			modifier_entry.excluded_targets | excluded_targets
		);
	}
}

void ModifierSum::add_modifier_sum_exclude_source(ModifierSum const& modifier_sum, modifier_source_t const& excluded_source) {
	reserve_more(modifiers, modifier_sum.modifiers.size());

	for (modifier_entry_t const& modifier_entry : modifier_sum.modifiers) {
		if (modifier_entry.source != excluded_source) {
			add_modifier(
				modifier_entry.modifier,
				modifier_entry.source,
				modifier_entry.multiplier,
				modifier_entry.excluded_targets
			);
		}
	}
}
