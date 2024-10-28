#include "MarketInstance.hpp"

#include <execution>

using namespace OpenVic;

bool MarketInstance::setup(GoodInstanceManager& new_good_instance_manager) {
	good_instance_manager = &new_good_instance_manager;
	return true;
}

void MarketInstance::place_market_sell_order(const MarketSellOrder market_sell_order) {
	GoodDefinition const* const good = market_sell_order.get_good();
	GoodInstance* const good_instance = good_instance_manager->get_good_instance_by_identifier(good->get_identifier());
	good_instance->add_market_sell_order(market_sell_order);
}

void MarketInstance::execute_orders() {
	std::vector<GoodInstance>& good_instances = good_instance_manager->get_good_instances();
	std::for_each(
		std::execution::par,
		good_instances.begin(),
		good_instances.end(),
		[](GoodInstance& good_instance) -> void {
			good_instance.execute_orders();
		}
	);
}
