#pragma once

#include <optional>

#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ArtisanalProducerDeps;
	struct GoodDefinition;
	struct GoodInstanceManager;
	struct ModifierEffectCache;
	struct Pop;
	struct PopValuesFromProvince;
	struct ProductionType;
	struct ProvinceInstance;
	struct RandomU32;

	struct ArtisanalProducer {
	private:
		ModifierEffectCache const& modifier_effect_cache;
		fixed_point_map_t<GoodDefinition const*> stockpile;

		//only used during day tick (from artisan_tick() until MarketInstance.execute_orders())
		fixed_point_map_t<GoodDefinition const*> max_quantity_to_buy_per_good;

		ProductionType const* PROPERTY(production_type_nullable);
		fixed_point_t PROPERTY(current_production);
		fixed_point_t PROPERTY(costs_of_production);

		void set_production_type(ProductionType const* const new_production_type);
		ProductionType const* pick_production_type(
			Pop& pop,
			PopValuesFromProvince const& values_from_province,
			RandomU32& random_number_generator
		) const;

		static fixed_point_t calculate_production_type_score(
			const fixed_point_t revenue,
			const fixed_point_t costs,
			const pop_size_t workforce
		);

	public:
		ArtisanalProducer(
			ArtisanalProducerDeps const& artisanal_producer_deps,
			fixed_point_map_t<GoodDefinition const*>&& new_stockpile,
			ProductionType const* const new_production_type,
			const fixed_point_t new_current_production
		);
		ArtisanalProducer(ArtisanalProducerDeps const& artisanal_producer_deps) : ArtisanalProducer(
			artisanal_producer_deps,
			fixed_point_map_t<GoodDefinition const*> {},
			nullptr,
			fixed_point_t::_0
		) { };

		void artisan_tick(
			Pop& pop,
			PopValuesFromProvince const& values_from_province,
			RandomU32& random_number_generator,
			IndexedFlatMap<GoodDefinition, char>& reusable_goods_mask,
			memory::vector<fixed_point_t>& pop_max_quantity_to_buy_per_good,
			memory::vector<fixed_point_t>& pop_money_to_spend_per_good,
			memory::vector<fixed_point_t>& reusable_map_0,
			memory::vector<fixed_point_t>& reusable_map_1
		);

		//thread safe if called once per good and stockpile already has an entry.
		//adds to stockpile up to max_quantity_to_buy and returns quantity added to stockpile
		fixed_point_t add_to_stockpile(GoodDefinition const& good, const fixed_point_t quantity);

		//optional to handle invalid
		static std::optional<fixed_point_t> estimate_production_type_score(
			GoodInstanceManager const& good_instance_manager,
			ProductionType const& production_type,
			ProvinceInstance& location,
			const fixed_point_t max_cost_multiplier
		);
	};
}
