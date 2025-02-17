#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodInstanceManager;
	struct ModifierEffectCache;
	struct Pop;
	struct ProductionType;

	struct ArtisanalProducer {
	private:
		ModifierEffectCache const& modifier_effect_cache;
		GoodDefinition::good_definition_map_t stockpile;

		//only used during day tick (from artisan_tick() until MarketInstance.execute_orders())
		GoodDefinition::good_definition_map_t max_quantity_to_buy_per_good;

		ProductionType const& PROPERTY(production_type);
		fixed_point_t PROPERTY(current_production);

	public:
		ArtisanalProducer(
			ModifierEffectCache const& new_modifier_effect_cache,
			GoodDefinition::good_definition_map_t&& new_stockpile,
			ProductionType const& new_production_type,
			fixed_point_t new_current_production
		);

		void artisan_tick(Pop& pop);

		//thread safe
		//adds to stockpile up to max_quantity_to_buy and returns quantity added to stockpile
		fixed_point_t add_to_stockpile(GoodDefinition const& good, const fixed_point_t quantity);
	};
}
