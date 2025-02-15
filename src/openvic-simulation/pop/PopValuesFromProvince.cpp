#include "PopValuesFromProvince.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;

void PopStrataValuesFromProvince::update_pop_strata_values_from_province(PopValuesFromProvince const& parent, Strata const& strata) {
	ProvinceInstance const& province = *parent.get_province();
	ModifierEffectCache const& modifier_effect_cache = province.get_modifier_effect_cache();
	ModifierEffectCache::strata_effects_t const& strata_effects = modifier_effect_cache.get_strata_effects()[strata];
	PopsDefines const& defines = parent.get_defines();
	fixed_point_t shared_base_needs_scalar = defines.get_base_goods_demand()
		* (fixed_point_t::_1() + province.get_modifier_effect_value(*modifier_effect_cache.get_goods_demand()));

	fixed_point_t invention_needs_scalar = fixed_point_t::_1();
	CountryInstance const* const owner_nullable = province.get_owner();
	if (owner_nullable != nullptr) {
		CountryInstance const& owner = *owner_nullable;
		shared_base_needs_scalar *= fixed_point_t::_1() + owner.get_plurality() / 100;
		invention_needs_scalar += owner.get_inventions_count() * defines.get_invention_impact_on_demand();
	}

	shared_life_needs_scalar = shared_base_needs_scalar
		* (fixed_point_t::_1() + province.get_modifier_effect_value(*strata_effects.get_life_needs()));
	shared_everyday_needs_scalar = shared_base_needs_scalar
		* invention_needs_scalar
		* (fixed_point_t::_1() + province.get_modifier_effect_value(*strata_effects.get_everyday_needs()));
	shared_luxury_needs_scalar = shared_base_needs_scalar
		* invention_needs_scalar
		* (fixed_point_t::_1() + province.get_modifier_effect_value(*strata_effects.get_luxury_needs()));
}

PopValuesFromProvince::PopValuesFromProvince(
	PopsDefines const& new_defines,
	decltype(effects_per_strata)::keys_type const& strata_keys
) : defines { new_defines },
	effects_per_strata { &strata_keys }
	{}

void PopValuesFromProvince::update_pop_values_from_province() {
	if (OV_unlikely(province == nullptr)) {
		OpenVic::Logger::error("PopValuesFromProvince has no province. Check ProvinceInstance::setup correctly sets it.");
		return;
	}

	for (auto [strata, values] : effects_per_strata) {
		values.update_pop_strata_values_from_province(*this, strata);
	}
}