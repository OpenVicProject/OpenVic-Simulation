#include "MarketInstance.hpp"

#include "openvic-simulation/defines/CountryDefines.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;

bool MarketInstance::get_is_available(GoodDefinition const& good_definition) const {
	return good_instance_manager.get_good_instance_by_definition(good_definition).get_is_available();
}

fixed_point_t MarketInstance::get_max_next_price(GoodDefinition const& good_definition) const {
	return good_instance_manager.get_good_instance_by_definition(good_definition).get_max_next_price();
}

fixed_point_t MarketInstance::get_min_next_price(GoodDefinition const& good_definition) const {
	return good_instance_manager.get_good_instance_by_definition(good_definition).get_min_next_price();
}

fixed_point_t MarketInstance::get_max_money_to_allocate_to_buy_quantity(GoodDefinition const& good_definition, const fixed_point_t quantity) const {
	//round up so money_to_spend >= max_next_price * max_quantity_to_buy;
	//always add epsilon as money_to_spend == max_next_price * max_quantity_to_buy is rare and this is cheaper for performance.
	return quantity * get_max_next_price(good_definition) + fixed_point_t::epsilon;
}

fixed_point_t MarketInstance::get_price_inverse(GoodDefinition const& good_definition) const {
	return good_instance_manager.get_good_instance_by_definition(good_definition).get_price_inverse();
}

void MarketInstance::place_buy_up_to_order(BuyUpToOrder&& buy_up_to_order) {
	GoodDefinition const& good = buy_up_to_order.get_good();
	if (OV_unlikely(buy_up_to_order.get_max_quantity() <= 0)) {
		Logger::error("Received BuyUpToOrder for ",good," with max quantity ",buy_up_to_order.get_max_quantity());
		buy_up_to_order.call_after_trade(BuyResult::no_purchase_result(good));
		return;
	}

	GoodMarket& good_instance = good_instance_manager.get_good_instance_by_definition(good);
	good_instance.add_buy_up_to_order(std::move(buy_up_to_order));
}

void MarketInstance::place_market_sell_order(MarketSellOrder&& market_sell_order, memory::vector<fixed_point_t>& reusable_vector) {
	GoodDefinition const& good = market_sell_order.get_good();
	if (OV_unlikely(market_sell_order.get_quantity() <= 0)) {
		Logger::error("Received MarketSellOrder for ", good, " with quantity ", market_sell_order.get_quantity());
		market_sell_order.call_after_trade(SellResult::no_sales_result(), reusable_vector);
		return;
	}

	if (good.get_is_money()) {
		market_sell_order.call_after_trade(
			{
				market_sell_order.get_quantity(),
				market_sell_order.get_quantity() * country_defines.get_gold_to_worker_pay_rate() * good.get_base_price()
			},
			reusable_vector
		);
		return;
	}

	GoodMarket& good_instance = good_instance_manager.get_good_instance_by_definition(good);
	good_instance.add_market_sell_order(std::move(market_sell_order));
}

void MarketInstance::execute_orders() {
	thread_pool.process_good_execute_orders();
}

void MarketInstance::record_price_history() {
	for (GoodMarket& good_instance : good_instance_manager.get_good_instances()) {
		good_instance.record_price_history();
	}
}
