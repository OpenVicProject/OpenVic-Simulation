#include "MarketInstance.hpp"

#include "openvic-simulation/defines/CountryDefines.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstanceManager.hpp"
#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"
#include "openvic-simulation/core/Typedefs.hpp"

using namespace OpenVic;

bool MarketInstance::get_is_available(const good_index_t good_index) const {
	return good_instance_manager.get_good_instance_by_index(good_index)->get_is_available();
}

fixed_point_t MarketInstance::get_max_next_price(const good_index_t good_index) const {
	return good_instance_manager.get_good_instance_by_index(good_index)->get_max_next_price();
}

fixed_point_t MarketInstance::get_min_next_price(const good_index_t good_index) const {
	return good_instance_manager.get_good_instance_by_index(good_index)->get_min_next_price();
}

fixed_point_t MarketInstance::get_max_money_to_allocate_to_buy_quantity(const good_index_t good_index, const fixed_point_t quantity) const {
	//round up so money_to_spend >= max_next_price * max_quantity_to_buy;
	//always add epsilon as money_to_spend == max_next_price * max_quantity_to_buy is rare and this is cheaper for performance.
	return quantity * get_max_next_price(good_index) + fixed_point_t::epsilon;
}

GoodInstance const& MarketInstance::get_good_instance(const good_index_t good_index) const {
	return *good_instance_manager.get_good_instance_by_index(good_index);
}

void MarketInstance::place_buy_up_to_order(BuyUpToOrder&& buy_up_to_order) {
	const good_index_t good_index = buy_up_to_order.good_index;
	if (OV_unlikely(buy_up_to_order.max_quantity <= 0)) {
		spdlog::error_s(
			"Received BuyUpToOrder for {} with max quantity {}",
			good_index, buy_up_to_order.max_quantity
		);
		buy_up_to_order.call_after_trade(BuyResult::no_purchase_result(good_index));
		return;
	}

	GoodMarket& good_instance = *good_instance_manager.get_good_instance_by_index(good_index);
	good_instance.add_buy_up_to_order(std::move(buy_up_to_order));
}

void MarketInstance::place_market_sell_order(MarketSellOrder&& market_sell_order, memory::vector<fixed_point_t>& reusable_vector) {
	const good_index_t good_index = market_sell_order.good_index;
	const fixed_point_t quantity = market_sell_order.quantity;

	if (OV_unlikely(quantity <= 0)) {
		spdlog::error_s(
			"Received MarketSellOrder for {} with quantity {}",
			good_index, quantity
		);
		market_sell_order.call_after_trade(SellResult::no_sales_result(good_index), reusable_vector);
		return;
	}

	GoodMarket& good_instance = *good_instance_manager.get_good_instance_by_index(good_index);
	if (good_instance.good_definition.is_money) {
		market_sell_order.call_after_trade(
			{
				market_sell_order.good_index,
				quantity,
				quantity * country_defines.get_gold_to_worker_pay_rate() * good_instance.good_definition.base_price
			},
			reusable_vector
		);
		return;
	}

	good_instance.add_market_sell_order(std::move(market_sell_order));
}

void MarketInstance::execute_orders() {
	thread_pool.process(work_t::GOOD_EXECUTE_ORDERS);
}

void MarketInstance::record_price_history() {
	for (GoodMarket& good_instance : good_instance_manager.get_good_instances()) {
		good_instance.record_price_history();
	}
}
