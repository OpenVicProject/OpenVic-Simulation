#include "ModifierSum.hpp"

#include "openvic-simulation/modifier/Modifier.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/utility/Concepts.hpp"

using namespace OpenVic;

std::string_view modifier_entry_t::source_to_string(modifier_source_t const& source) {
	return std::visit(
		[](has_get_identifier auto const* has_identifier) -> std::string_view {
			return has_identifier->get_identifier();
		},
		source
	);
}

memory::string modifier_entry_t::to_string() const {
	return StringUtils::append_string_views(
		"[", modifier.get_identifier(), ", ", multiplier.to_string(), ", ", source_to_string(source), ", ",
		ModifierEffect::target_to_string(excluded_targets), "]"
	);
}

void ModifierSum::clear() {
	modifiers.clear();
	value_sum.clear();
}

bool ModifierSum::empty() {
	return modifiers.empty();
}

fixed_point_t ModifierSum::get_modifier_effect_value(ModifierEffect const& effect, bool* effect_found) const {
	return value_sum.get_effect(effect, effect_found);
}

bool ModifierSum::has_modifier_effect(ModifierEffect const& effect) const {
	return value_sum.has_effect(effect);
}

void ModifierSum::add_modifier(
	Modifier const& modifier, fixed_point_t multiplier, modifier_entry_t::modifier_source_t const& source,
	ModifierEffect::target_t excluded_targets
) {
	// We could test that excluded_targets != ALL_TARGETS, but in practice it's always
	// called with an explcit/hardcoded value and so won't ever exclude everything.
	if (multiplier != fixed_point_t::_0) {
		modifier_entry_t const& new_entry = modifiers.emplace_back(
			modifier,
			multiplier,
			modifier_entry_t::source_or_null_fallback(source, this_source),
			excluded_targets | this_excluded_targets
		);
		value_sum.multiply_add_exclude_targets(new_entry.modifier, new_entry.multiplier, new_entry.excluded_targets);
	}
}

void ModifierSum::add_modifier_sum(ModifierSum const& modifier_sum) {
	reserve_more(modifiers, modifier_sum.modifiers.size());

	// We could test that excluded_targets != ALL_TARGETS, but in practice it's always
	// called with an explcit/hardcoded value and so won't ever exclude everything.
	for (modifier_entry_t const& modifier_entry : modifier_sum.modifiers) {
		add_modifier(
			modifier_entry.modifier,
			modifier_entry.multiplier,
			modifier_entry.source,
			modifier_entry.excluded_targets
		);
	}
}
