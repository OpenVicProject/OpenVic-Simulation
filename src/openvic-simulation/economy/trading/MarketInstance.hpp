#pragma once

#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"

namespace OpenVic {
	struct MarketInstance {
	private:
		GoodInstanceManager& PROPERTY(good_instance_manager);
	public:
		MarketInstance(GoodInstanceManager& new_good_instance_manager);
		void place_buy_up_to_order(BuyUpToOrder&& buy_up_to_order);
		void place_market_sell_order(MarketSellOrder&& market_sell_order);
		void execute_orders();
		void record_price_history();
	};
}
