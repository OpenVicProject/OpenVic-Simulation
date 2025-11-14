#pragma once

#include <optional>

#include "openvic-simulation/types/IndexedFlatMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/fixed_point/Fraction.hpp"
#include "openvic-simulation/types/PopSize.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct ArtisanalProducerDeps;
	struct CountryInstance;
	struct EconomyDefines;
	struct GoodDefinition;
	struct GoodInstanceManager;
	struct MarketInstance;
	struct ModifierEffectCache;
	struct Pop;
	struct PopValuesFromProvince;
	struct ProductionType;
	struct ProvinceInstance;
	struct RandomU32;

	struct ArtisanalProducer {
	private:
		EconomyDefines const& economy_defines;
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
			MarketInstance const& market_instance,
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
	
	private:
		struct artisan_tick_handler {
		private:
			CountryInstance* const country_to_report_economy_nullable;
			memory::vector<fixed_point_t>& demand_per_input;
			size_t distinct_goods_to_buy = 0;
			Fraction inputs_bought_fraction;
			MarketInstance const& market_instance;
			memory::vector<fixed_point_t>& max_price_per_input;
			Pop& pop;
			ProductionType const& production_type;
			IndexedFlatMap<GoodDefinition, char>& wants_more_mask;

		public:
			artisan_tick_handler(
				CountryInstance* const new_country_to_report_economy_nullable,
				memory::vector<fixed_point_t>& new_demand_per_input,
				MarketInstance const& new_market_instance,
				memory::vector<fixed_point_t>& new_max_price_per_input,
				Pop& new_pop,
				ProductionType const& new_production_type,
				IndexedFlatMap<GoodDefinition, char>& new_wants_more_mask
			) : country_to_report_economy_nullable { new_country_to_report_economy_nullable },
				demand_per_input { new_demand_per_input },
				market_instance { new_market_instance },
				max_price_per_input { new_max_price_per_input },
				pop { new_pop },
				production_type { new_production_type },
				wants_more_mask { new_wants_more_mask } {}

			void calculate_inputs(fixed_point_map_t<GoodDefinition const*> const& stockpile);
			void produce(
				fixed_point_t& costs_of_production,
				fixed_point_t& current_production,
				fixed_point_map_t<GoodDefinition const*>& stockpile
			);
			void allocate_money_for_inputs(
				fixed_point_map_t<GoodDefinition const*>& max_quantity_to_buy_per_good,
				memory::vector<fixed_point_t>& pop_max_quantity_to_buy_per_good,
				memory::vector<fixed_point_t>& pop_money_to_spend_per_good,
				fixed_point_map_t<GoodDefinition const*> const& stockpile,
				PopValuesFromProvince const& values_from_province
			);
		};
	};
}
