#pragma once

#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodDefinition;
	struct GoodInstanceManager;
	struct ModifierEffectCache;
	struct Pop;
	struct ProductionType;

	struct ArtisanalProducer {
	private:
		ModifierEffectCache const& modifier_effect_cache;
		fixed_point_map_t<GoodDefinition const*> stockpile;

		//only used during day tick (from artisan_tick() until MarketInstance.execute_orders())
		fixed_point_map_t<GoodDefinition const*> max_quantity_to_buy_per_good;

		ProductionType const& PROPERTY(production_type);
		fixed_point_t PROPERTY(current_production);

	public:
		ArtisanalProducer(
			ModifierEffectCache const& new_modifier_effect_cache,
			fixed_point_map_t<GoodDefinition const*>&& new_stockpile,
			ProductionType const& new_production_type,
			fixed_point_t new_current_production
		);

		void artisan_tick(
			Pop& pop,
			const fixed_point_t max_cost_multiplier,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			memory::vector<fixed_point_t>& pop_max_quantity_to_buy_per_good,
			memory::vector<fixed_point_t>& pop_money_to_spend_per_good,
			memory::vector<fixed_point_t>& reusable_map_0,
			memory::vector<fixed_point_t>& reusable_map_1
		);

		//thread safe if called once per good and stockpile already has an entry.
		//adds to stockpile up to max_quantity_to_buy and returns quantity added to stockpile
		fixed_point_t add_to_stockpile(GoodDefinition const& good, const fixed_point_t quantity);
	};
}
