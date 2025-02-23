#include "GoodMarket.hpp"

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

using namespace OpenVic;
static constexpr size_t MONTHS_OF_PRICE_HISTORY = 36;

GoodMarket::GoodMarket(GameRulesManager const& new_game_rules_manager, GoodDefinition const& new_good_definition)
  : buy_lock { std::make_unique<std::mutex>() },
	sell_lock { std::make_unique<std::mutex>() },
	game_rules_manager { new_game_rules_manager },
	good_definition { new_good_definition },
	price { new_good_definition.get_base_price() },
	is_available { new_good_definition.get_is_available_from_start() },
	price_history { MONTHS_OF_PRICE_HISTORY, new_good_definition.get_base_price() }
	{
		on_use_exponential_price_changes_changed();
		update_next_price_limits();
	}

void GoodMarket::on_use_exponential_price_changes_changed() {
	if (game_rules_manager.get_use_exponential_price_changes()) {
		absolute_maximum_price = fixed_point_t::usable_max();
		absolute_minimum_price = fixed_point_t::epsilon() << exponential_price_change_shift;
	} else {
		absolute_maximum_price = std::min(
			good_definition.get_base_price() * 5,
			fixed_point_t::usable_max()
		);
		absolute_minimum_price = std::max(
			good_definition.get_base_price() * 22 / 100,
			fixed_point_t::epsilon()
		);
	}
}

void GoodMarket::update_next_price_limits() {
	const fixed_point_t max_price_change = game_rules_manager.get_use_exponential_price_changes()
		? price >> exponential_price_change_shift
		: fixed_point_t::_0_01();

	max_next_price = std::min(
		absolute_maximum_price,
		price + max_price_change
	);
	min_next_price = std::max(
		absolute_minimum_price,
		price - max_price_change
	);

	price_inverse = fixed_point_t::_1() / price;
}

void GoodMarket::add_buy_up_to_order(GoodBuyUpToOrder const& buy_up_to_order) {
	const std::lock_guard<std::mutex> lock_guard { *buy_lock };
	buy_up_to_orders.push_back(buy_up_to_order);
}

void GoodMarket::add_market_sell_order(GoodMarketSellOrder const& market_sell_order) {
	const std::lock_guard<std::mutex> lock_guard { *sell_lock };
	market_sell_orders.push_back(market_sell_order);
}

void GoodMarket::execute_orders(std::vector<fixed_point_t>& reusable_vector_0, std::vector<fixed_point_t>& reusable_vector_1) {
	if (!is_available) {
		//price remains the same
		price_change_yesterday
			= quantity_traded_yesterday
			= total_demand_yesterday
			= total_supply_yesterday
			= fixed_point_t::_0();

		for (GoodBuyUpToOrder const& buy_up_to_order : buy_up_to_orders) {
			buy_up_to_order.call_after_trade(BuyResult::no_purchase_result(good_definition));
		}

		for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
			market_sell_order.call_after_trade(SellResult::no_sales_result());
		}
		return;
	}

	fixed_point_t new_price;
	//MarketInstance ensured only orders with quantity > 0 are added.
	//So running total > 0 unless orders are empty.
	fixed_point_t demand_sum = fixed_point_t::_0(),
		supply_sum = fixed_point_t::_0();
	if (market_sell_orders.empty()) {
		quantity_traded_yesterday = fixed_point_t::_0();
		fixed_point_t max_affordable_price = price;
		for (GoodBuyUpToOrder const& buy_up_to_order : buy_up_to_orders) {
			const fixed_point_t affordable_price = buy_up_to_order.get_affordable_price();
			if (affordable_price > max_affordable_price) {
				max_affordable_price = affordable_price;
			}

			demand_sum += buy_up_to_order.get_max_quantity();
			buy_up_to_order.call_after_trade(BuyResult::no_purchase_result(good_definition));
		}

		if (game_rules_manager.get_use_optimal_pricing()) {
			new_price = std::min(max_next_price, max_affordable_price);
		} else {
			//TODO use Victoria 2's square root mechanic, see https://github.com/OpenVicProject/OpenVic/issues/288
			if (demand_sum > fixed_point_t::_0()) {
				new_price = max_next_price;
			} else {
				new_price = price;
			}
		}
	} else {
		for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
			supply_sum += market_sell_order.get_quantity();
		}
		std::vector<fixed_point_t>& quantity_bought_per_order = reusable_vector_0;
		std::vector<fixed_point_t>& purchasing_power_per_order = reusable_vector_1;
		quantity_bought_per_order.resize(buy_up_to_orders.size());
		purchasing_power_per_order.resize(buy_up_to_orders.size());

		fixed_point_t money_left_to_spend_sum = fixed_point_t::_0(); //sum of money_to_spend for all buyers that can't afford their max_quantity
		fixed_point_t max_quantity_to_buy_sum = fixed_point_t::_0();
		fixed_point_t purchasing_power_sum = fixed_point_t::_0();
		fixed_point_t remaining_supply = supply_sum;
		for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
			GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];
			if (game_rules_manager.get_use_optimal_pricing()) {
				const fixed_point_t affordable_price = buy_up_to_order.get_affordable_price();
				if (affordable_price > min_next_price) {
					//no point selling lower as it would not attract more buyers
					min_next_price = affordable_price;
				}
			}

			demand_sum += buy_up_to_order.get_max_quantity();

			if (buy_up_to_order.get_money_to_spend() <= fixed_point_t::_0()) {
				purchasing_power_per_order[i] = quantity_bought_per_order[i] = fixed_point_t::_0();
				continue;
			}

			fixed_point_t purchasing_power = purchasing_power_per_order[i] = buy_up_to_order.get_money_to_spend() / max_next_price;
			if (purchasing_power >= buy_up_to_order.get_max_quantity()) {
				quantity_bought_per_order[i] = buy_up_to_order.get_max_quantity();
				max_quantity_to_buy_sum += buy_up_to_order.get_max_quantity();
				remaining_supply -= buy_up_to_order.get_max_quantity();
			} else {
				quantity_bought_per_order[i] = fixed_point_t::_0();
				max_quantity_to_buy_sum += purchasing_power;
				money_left_to_spend_sum += buy_up_to_order.get_money_to_spend();
				purchasing_power_sum += purchasing_power;
			}
		}

		if (max_quantity_to_buy_sum >= supply_sum) {
			//sell for max_next_price
			if (game_rules_manager.get_use_optimal_pricing()) {
				new_price = max_next_price;
			} else {
				//TODO use Victoria 2's square root mechanic, see https://github.com/OpenVicProject/OpenVic/issues/288
				new_price = max_next_price;
			}

			bool someone_bought_max_quantity;
			do {
				someone_bought_max_quantity = false;
				for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
					GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];
					if (quantity_bought_per_order[i] == buy_up_to_order.get_max_quantity()) {
						continue;
					}

					const fixed_point_t distributed_supply
						= quantity_bought_per_order[i]
						= fixed_point_t::mul_div(
							remaining_supply,
							purchasing_power_per_order[i],
							purchasing_power_sum
						);
					if (distributed_supply >= buy_up_to_order.get_max_quantity()) {
						someone_bought_max_quantity = true;
						quantity_bought_per_order[i] = buy_up_to_order.get_max_quantity();
						remaining_supply -= buy_up_to_order.get_max_quantity();
						purchasing_power_sum -= purchasing_power_per_order[i];
					}
				}
			} while (someone_bought_max_quantity);

			quantity_traded_yesterday = fixed_point_t::_0();
			for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
				GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];
				const fixed_point_t quantity_bought = quantity_bought_per_order[i];
				fixed_point_t money_spent;
				if (quantity_bought == fixed_point_t::_0()) {
					money_spent = fixed_point_t::_0();
				} else {
					quantity_traded_yesterday += quantity_bought;
					money_spent = std::max(
					   quantity_bought * new_price,
					   fixed_point_t::epsilon() //we know from purchasing power that you can afford it.
				   );
				}
				buy_up_to_order.call_after_trade({
					good_definition,
					quantity_bought,
					money_spent
				});
			}
		} else {
			//sell below max_next_price
			if (game_rules_manager.get_use_optimal_pricing()) {
				//drop price while remaining_supply > 0 && new_price > min_next_price
				while (remaining_supply > fixed_point_t::_0()) {
					const fixed_point_t possible_price = money_left_to_spend_sum / remaining_supply;

					if (possible_price >= new_price) {
						//use previous new_price
						break;
					}

					if (possible_price < min_next_price) {
						new_price = min_next_price;
						break;
					}

					new_price = possible_price;

					for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
						GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];
						if (quantity_bought_per_order[i] == buy_up_to_order.get_max_quantity()) {
							continue;
						}

						if (buy_up_to_order.get_money_to_spend() >= new_price * buy_up_to_order.get_max_quantity()) {
							quantity_bought_per_order[i] = buy_up_to_order.get_max_quantity();
							remaining_supply -= buy_up_to_order.get_max_quantity();
							money_left_to_spend_sum -= buy_up_to_order.get_money_to_spend();
						}
					}
				}
			} else {
				//TODO use Victoria 2's square root mechanic, see https://github.com/OpenVicProject/OpenVic/issues/288
				if (supply_sum > demand_sum) {
					new_price = min_next_price;
				} else {
					new_price = price;
				}
			}

			quantity_traded_yesterday = fixed_point_t::_0();
			//figure out how much every buyer bought
			for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
				GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];
				const fixed_point_t quantity_bought
					= quantity_bought_per_order[i]
					= std::min(
					buy_up_to_order.get_max_quantity(),
					buy_up_to_order.get_money_to_spend() / new_price
				);

				if (quantity_bought <= fixed_point_t::_0()) {
					buy_up_to_order.call_after_trade(BuyResult::no_purchase_result(good_definition));
				} else {
					const fixed_point_t money_spent = std::max(
						quantity_bought * new_price,
						fixed_point_t::epsilon()
					);
					quantity_traded_yesterday += quantity_bought;
					buy_up_to_order.call_after_trade({
						good_definition,
						quantity_bought,
						money_spent
					});
				}
			}
		}

		for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
			const fixed_point_t quantity_sold = fixed_point_t::mul_div(
				market_sell_order.get_quantity(),
				quantity_traded_yesterday,
				supply_sum
			);
			fixed_point_t money_gained;
			if (quantity_sold == fixed_point_t::_0()) {
				money_gained = fixed_point_t::_0();
			} else {
				money_gained = std::max(
					quantity_sold * new_price,
					fixed_point_t::epsilon() //round up
				);
			}
			market_sell_order.call_after_trade({
				quantity_sold,
				money_gained
			});
		}

		market_sell_orders.clear();
		reusable_vector_0.clear();
		reusable_vector_1.clear();
	}

	price_change_yesterday = new_price - price;
	total_demand_yesterday = demand_sum;
	total_supply_yesterday = supply_sum;
	buy_up_to_orders.clear();
	if (new_price != price) {
		price = new_price;
		update_next_price_limits();
	}
}

void GoodMarket::record_price_history() {
	price_history.push_back(price);
}