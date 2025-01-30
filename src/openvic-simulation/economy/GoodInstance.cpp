#include "GoodInstance.hpp"

using namespace OpenVic;

GoodInstance::GoodInstance(GoodDefinition const& new_good_definition, GameRulesManager const& new_game_rules_manager)
  : HasIdentifierAndColour { new_good_definition },
	buy_lock { std::make_unique<std::mutex>() },
	sell_lock { std::make_unique<std::mutex>() },
	game_rules_manager { new_game_rules_manager },
  	good_definition { new_good_definition },
	price { new_good_definition.get_base_price() },
	price_change_yesterday { fixed_point_t::_0() },
	max_next_price {},
	min_next_price {},
	is_available { new_good_definition.get_is_available_from_start() },
	total_demand_yesterday { fixed_point_t::_0() },
	total_supply_yesterday { fixed_point_t::_0() },
	quantity_traded_yesterday { fixed_point_t::_0() },
	buy_up_to_orders {},
	market_sell_orders {}
	{ update_next_price_limits(); }

void GoodInstance::update_next_price_limits() {
	if(game_rules_manager.get_use_exponential_price_changes()) {
		const fixed_point_t max_change = price >> 6;
		max_next_price = std::min(
			fixed_point_t::usable_max(),
			price + max_change
		);
		min_next_price = std::max(
			fixed_point_t::epsilon(),
			price - max_change
		);
	} else {
		max_next_price = std::min(
			std::min(
				good_definition.get_base_price() * 5,
				fixed_point_t::usable_max()
			),
			price + fixed_point_t::_1() / fixed_point_t::_100()
		);
		min_next_price = std::max(
			std::max(
				good_definition.get_base_price() * 22 / fixed_point_t::_100(),
				fixed_point_t::epsilon()
			),
			price - fixed_point_t::_1() / fixed_point_t::_100()
		);
	}
}

void GoodInstance::add_buy_up_to_order(GoodBuyUpToOrder&& buy_up_to_order) {
	const std::lock_guard<std::mutex> lock {*buy_lock};
	buy_up_to_orders.push_back(std::move(buy_up_to_order));
}

void GoodInstance::add_market_sell_order(GoodMarketSellOrder&& market_sell_order) {
	const std::lock_guard<std::mutex> lock {*sell_lock};
	market_sell_orders.push_back(std::move(market_sell_order));
}

void GoodInstance::execute_orders() {
	//MarketInstance ensured only orders with quantity > 0 are added.
	//So running total > 0 unless orders are empty.
	fixed_point_t demand_running_total = fixed_point_t::_0();
	for (GoodBuyUpToOrder const& buy_up_to_order : buy_up_to_orders) {
		demand_running_total += buy_up_to_order.get_max_quantity();
	}

	fixed_point_t supply_running_total = fixed_point_t::_0();
	for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
		supply_running_total += market_sell_order.get_quantity();
	}
	
	fixed_point_t new_price;
	if (demand_running_total > supply_running_total) {
		new_price = max_next_price;
		quantity_traded_yesterday = supply_running_total;
	} else if (demand_running_total < supply_running_total) {
		new_price = min_next_price;
		quantity_traded_yesterday = demand_running_total;
	} else {
		quantity_traded_yesterday = demand_running_total;
		new_price = price;
	}
	
	for (GoodBuyUpToOrder const& buy_up_to_order : buy_up_to_orders) {
		const fixed_point_t quantity_bought = buy_up_to_order.get_max_quantity() * quantity_traded_yesterday / demand_running_total;
		const fixed_point_t money_spend = quantity_bought * new_price;
		buy_up_to_order.get_after_trade()({
			quantity_bought,
			buy_up_to_order.get_money_to_spend() - money_spend
		});
	}

	for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
		const fixed_point_t quantity_sold = market_sell_order.get_quantity() * quantity_traded_yesterday / supply_running_total;
		market_sell_order.get_after_trade()({
			quantity_sold,
			quantity_sold * new_price
		});
	}

	price_change_yesterday = new_price - price;
	total_demand_yesterday = demand_running_total;
	total_supply_yesterday = supply_running_total;
	buy_up_to_orders.clear();
	market_sell_orders.clear();
	if (new_price != price) {
		update_next_price_limits();
	}
}

bool GoodInstanceManager::setup_goods(GoodDefinitionManager const& good_definition_manager, GameRulesManager const& game_rules_manager) {
	if (good_instances_are_locked()) {
		Logger::error("Cannot set up good instances - they are already locked!");
		return false;
	}

	good_instances.reserve(good_definition_manager.get_good_definition_count());

	bool ret = true;

	for (GoodDefinition const& good : good_definition_manager.get_good_definitions()) {
		ret &= good_instances.add_item({ good, game_rules_manager });
	}

	lock_good_instances();

	return ret;
}

GoodInstance& GoodInstanceManager::get_good_instance_from_definition(GoodDefinition const& good) {
	return good_instances.get_items()[good.get_index()];
}

GoodInstance const& GoodInstanceManager::get_good_instance_from_definition(GoodDefinition const& good) const {
	return good_instances.get_items()[good.get_index()];
}