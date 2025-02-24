#pragma once

#include <array>
#include <vector>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/ParallelProcessorWithSharesValues.hpp"

namespace OpenVic {
	struct BuyUpToOrder;
	struct CountryDefines;
	struct GoodDefinition;
	struct GoodInstanceManager;
	struct MarketSellOrder;

	static constexpr size_t VECTORS_FOR_GOODMARKET = 2;
	struct MarketInstance : private ParallelProcessorWithSharesValues<std::array<std::vector<fixed_point_t>, VECTORS_FOR_GOODMARKET>> {
	private:
		CountryDefines const& country_defines;
		GoodInstanceManager& good_instance_manager;
	public:
		MarketInstance(CountryDefines const& new_country_defines, GoodInstanceManager& new_good_instance_manager);
		bool get_is_available(GoodDefinition const& good_definition) const;
		fixed_point_t get_max_next_price(GoodDefinition const& good_definition) const;
		fixed_point_t get_min_next_price(GoodDefinition const& good_definition) const;
		fixed_point_t get_max_money_to_allocate_to_buy_quantity(GoodDefinition const& good_definition, const fixed_point_t quantity) const;
		fixed_point_t get_price_inverse(GoodDefinition const& good_definition) const;
		void place_buy_up_to_order(BuyUpToOrder const& buy_up_to_order);
		void place_market_sell_order(MarketSellOrder const& market_sell_order);
		void execute_orders();
		void record_price_history();
	};
}
