#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct BuyUpToOrder;
	struct CountryDefines;
	struct GoodDefinition;
	struct GoodInstanceManager;
	struct MarketSellOrder;

	struct MarketInstance {
	private:
		CountryDefines const& country_defines;
		GoodInstanceManager& good_instance_manager;
	public:
		MarketInstance(CountryDefines const& new_country_defines, GoodInstanceManager& new_good_instance_manager);
		bool get_is_available(GoodDefinition const& good_definition) const;
		fixed_point_t get_max_next_price(GoodDefinition const& good_definition) const;
		fixed_point_t get_price_inverse(GoodDefinition const& good_definition) const;
		void place_buy_up_to_order(BuyUpToOrder&& buy_up_to_order);
		void place_market_sell_order(MarketSellOrder&& market_sell_order);
		void execute_orders();
		void record_price_history();
	};
}
