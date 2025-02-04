#include "MarketInstance.hpp"

#include "openvic-simulation/utility/CompilerFeatureTesting.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;

MarketInstance::MarketInstance(GoodInstanceManager& new_good_instance_manager)
: good_instance_manager { new_good_instance_manager} {}

void MarketInstance::place_buy_up_to_order(BuyUpToOrder&& buy_up_to_order) {
	GoodDefinition const& good = buy_up_to_order.get_good();
	if (OV_unlikely(buy_up_to_order.get_max_quantity() <= 0)) {
		Logger::error("Received BuyUpToOrder for ",good," with max quantity ",buy_up_to_order.get_max_quantity());
		buy_up_to_order.get_after_trade()(BuyResult::no_purchase_result());
		return;
	}

	GoodInstance& good_instance = good_instance_manager.get_good_instance_from_definition(good);
	good_instance.add_buy_up_to_order(std::move(buy_up_to_order));
}
void MarketInstance::place_market_sell_order(MarketSellOrder&& market_sell_order) {
	GoodDefinition const& good = market_sell_order.get_good();
	if (OV_unlikely(market_sell_order.get_quantity() <= 0)) {
		Logger::error("Received MarketSellOrder for ",good," with quantity ",market_sell_order.get_quantity());
		market_sell_order.get_after_trade()(SellResult::no_sales_result());
		return;
	}

	GoodInstance& good_instance = good_instance_manager.get_good_instance_from_definition(good);
	good_instance.add_market_sell_order(std::move(market_sell_order));
}

void MarketInstance::execute_orders() {
	auto& good_instances = good_instance_manager.get_good_instances();
	try_parallel_for_each(
		good_instances.begin(),
		good_instances.end(),
		[](GoodInstance& good_instance) -> void {
			good_instance.execute_orders();
		}
	);
}

void MarketInstance::record_price_history() {
	for (GoodInstance& good_instance : good_instance_manager.get_good_instances()) {
		good_instance.record_price_history();
	}
}
