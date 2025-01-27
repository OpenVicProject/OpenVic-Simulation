#pragma once

#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct MarketInstance;
	struct ModifierEffectCache;
	struct Pop;
	struct ProductionType;

	struct ArtisanalProducer {
	private:
		MarketInstance& market_instance;
		ModifierEffectCache const& modifier_effect_cache;
		GoodDefinition::good_definition_map_t stockpile;
		ProductionType const& PROPERTY(production_type);
		fixed_point_t PROPERTY(current_production);

	public:
		ArtisanalProducer(
			MarketInstance& new_market_instance,
			ModifierEffectCache const& new_modifier_effect_cache,
			GoodDefinition::good_definition_map_t&& new_stockpile,
			ProductionType const& new_production_type,
			fixed_point_t new_current_production
		);
		void artisan_tick(Pop& pop);
	};
}
