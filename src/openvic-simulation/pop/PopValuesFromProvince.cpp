#include "PopValuesFromProvince.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp" // IWYU pragma: keep for constructor requirement
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

using namespace OpenVic;

void PopStrataValuesFromProvince::update_pop_strata_values_from_province(
	PopsDefines const& defines,
	Strata const& strata,
	ProvinceInstance const& province
) {
	ModifierEffectCache const& modifier_effect_cache = province.get_modifier_effect_cache();
	ModifierEffectCache::strata_effects_t const& strata_effects = modifier_effect_cache.get_strata_effects(strata);
	fixed_point_t shared_base_needs_scalar = defines.get_base_goods_demand()
		* (fixed_point_t::_1 + province.get_modifier_effect_value(*modifier_effect_cache.get_goods_demand()));

	fixed_point_t invention_needs_scalar = 1;
	CountryInstance const* const owner_nullable = province.get_owner();
	if (owner_nullable != nullptr) {
		CountryInstance const& owner = *owner_nullable;
		shared_base_needs_scalar *= fixed_point_t::_1 + owner.get_plurality_untracked() / 100;
		invention_needs_scalar += owner.get_inventions_count_untracked() * defines.get_invention_impact_on_demand();
	}

	shared_life_needs_scalar = shared_base_needs_scalar
		* (fixed_point_t::_1 + province.get_modifier_effect_value(*strata_effects.get_life_needs()));
	shared_everyday_needs_scalar = shared_base_needs_scalar
		* invention_needs_scalar
		* (fixed_point_t::_1 + province.get_modifier_effect_value(*strata_effects.get_everyday_needs()));
	shared_luxury_needs_scalar = shared_base_needs_scalar
		* invention_needs_scalar
		* (fixed_point_t::_1 + province.get_modifier_effect_value(*strata_effects.get_luxury_needs()));
}

PopValuesFromProvince::PopValuesFromProvince(
	PopsDefines const& new_defines,
	decltype(reusable_goods_mask)::keys_span_type good_keys,
	decltype(effects_by_strata)::keys_span_type strata_keys
) : defines { new_defines },
	reusable_goods_mask { good_keys },
	effects_by_strata { strata_keys }
	{}

PopStrataValuesFromProvince const& PopValuesFromProvince::get_effects_by_strata(Strata const& key) const {
	return effects_by_strata.at(key);
}

void PopValuesFromProvince::update_pop_values_from_province(ProvinceInstance const& province) {
	for (auto [strata, values] : effects_by_strata) {
		values.update_pop_strata_values_from_province(defines, strata, province);
	}
}