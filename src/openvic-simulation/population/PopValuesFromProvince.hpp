#pragma once

#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GameRulesManager;
	struct GoodInstanceManager;
	struct ModifierEffectCache;
	struct ProductionType;
	struct ProductionTypeManager;
	struct ProvinceInstance;
	struct PopsDefines;
	struct PopValuesFromProvince;
	struct Strata;

	struct PopStrataValuesFromProvince {
	private:
		fixed_point_t PROPERTY(shared_life_needs_scalar);
		fixed_point_t PROPERTY(shared_everyday_needs_scalar);
		fixed_point_t PROPERTY(shared_luxury_needs_scalar);
	public:
		void update_pop_strata_values_from_province(
			PopsDefines const& defines,
			ModifierEffectCache const& modifier_effect_cache,
			Strata const& strata,
			ProvinceInstance const& province
		);
	};

	struct PopValuesFromProvince {
	private:

		GameRulesManager const& game_rules_manager;
		GoodInstanceManager const& good_instance_manager;
		ModifierEffectCache const& modifier_effect_cache;
		ProductionTypeManager const& production_type_manager;
		fixed_point_t PROPERTY(max_cost_multiplier);
		OV_IFLATMAP_PROPERTY(Strata, PopStrataValuesFromProvince, effects_by_strata);
		//excludes availability of goods on market
		memory::vector<std::pair<ProductionType const*, fixed_point_t>> SPAN_PROPERTY(ranked_artisanal_production_types);
	public:
		PopsDefines const& defines;

		PopValuesFromProvince(
			GameRulesManager const& new_game_rules_manager,
			GoodInstanceManager const& new_good_instance_manager,
			ModifierEffectCache const& new_modifier_effect_cache,
			ProductionTypeManager const& new_production_type_manager,
			PopsDefines const& new_defines,
			decltype(effects_by_strata)::keys_span_type strata_keys
		);
		PopValuesFromProvince(PopValuesFromProvince&&) = default;

		void update_pop_values_from_province(ProvinceInstance& province);
	};
}