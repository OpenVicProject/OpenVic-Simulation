#include "PopValuesFromProvince.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/defines/PopsDefines.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducer.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
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
	GameRulesManager const& new_game_rules_manager,
	GoodInstanceManager const& new_good_instance_manager,
	ModifierEffectCache const& new_modifier_effect_cache,
	ProductionTypeManager const& new_production_type_manager,
	PopsDefines const& new_defines,
	decltype(effects_by_strata)::keys_span_type strata_keys
) : game_rules_manager { new_game_rules_manager },
	good_instance_manager { new_good_instance_manager },
	modifier_effect_cache { new_modifier_effect_cache },
	production_type_manager { new_production_type_manager },
	defines { new_defines },
	effects_by_strata { strata_keys }
	{}

PopStrataValuesFromProvince const& PopValuesFromProvince::get_effects_by_strata(Strata const& key) const {
	return effects_by_strata.at(key);
}

void PopValuesFromProvince::update_pop_values_from_province(ProvinceInstance& province) {
	for (auto [strata, values] : effects_by_strata) {
		values.update_pop_strata_values_from_province(defines, strata, province);
	}

	max_cost_multiplier = 1;	
	CountryInstance* const country_to_report_economy_nullable = province.get_country_to_report_economy();
	if (country_to_report_economy_nullable != nullptr) {
		const fixed_point_t tariff_rate = country_to_report_economy_nullable->effective_tariff_rate.get_untracked();
		if (tariff_rate > fixed_point_t::_0) {
			max_cost_multiplier += tariff_rate; //max (domestic cost, imported cost)
		}
	}
	
	ranked_artisanal_production_types.clear();
	ranked_artisanal_production_types.reserve(production_type_manager.get_production_type_count());
	for (auto const& production_type : production_type_manager.get_production_types()) {
		const std::optional<fixed_point_t> estimated_score = ArtisanalProducer::estimate_production_type_score(
			good_instance_manager,
			production_type,
			province,
			max_cost_multiplier
		);

		if (estimated_score.has_value() && estimated_score.value() > 0) {
			ranked_artisanal_production_types.push_back({&production_type, estimated_score.value()});
		}
	}

	if (!ranked_artisanal_production_types.empty()) {
		std::sort(ranked_artisanal_production_types.begin(), ranked_artisanal_production_types.end(),
			[](const auto& a, const auto& b) {
				return a.second > b.second;
			}
		);
	}
}