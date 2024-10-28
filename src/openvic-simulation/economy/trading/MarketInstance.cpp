#include "MarketInstance.hpp"

using namespace OpenVic;

MarketInstance::MarketInstance(GoodInstanceManager& new_good_instance_manager)
: good_instance_manager { new_good_instance_manager} {}

void MarketInstance::place_market_sell_order(MarketSellOrder&& market_sell_order) {
	GoodDefinition const& good = market_sell_order.get_good();
	GoodInstance& good_instance = good_instance_manager.get_good_instance_from_definition(good);
	good_instance.add_market_sell_order(std::move(market_sell_order));
}

void MarketInstance::execute_orders() {
	std::vector<GoodInstance>& good_instances = good_instance_manager.get_good_instances();
	for (GoodInstance& good_instance : good_instances) {
		good_instance.execute_orders();
	}
}
