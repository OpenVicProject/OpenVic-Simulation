#include "GoodMarket.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
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
		absolute_maximum_price = fixed_point_t::usable_max;
		absolute_minimum_price = fixed_point_t::epsilon << exponential_price_change_shift;
	} else {
		absolute_maximum_price = std::min(
			good_definition.get_base_price() * 5,
			fixed_point_t::usable_max
		);
		absolute_minimum_price = std::max(
			good_definition.get_base_price() * 22 / 100,
			fixed_point_t::epsilon
		);
	}
}

void GoodMarket::update_next_price_limits() {
	const fixed_point_t max_price_change = game_rules_manager.get_use_exponential_price_changes()
		? price >> exponential_price_change_shift
		: fixed_point_t::_0_01;

	max_next_price = std::min(
		absolute_maximum_price,
		price + max_price_change
	);
	min_next_price = std::max(
		absolute_minimum_price,
		price - max_price_change
	);

	price_inverse = fixed_point_t::_1 / price;
}

void GoodMarket::add_buy_up_to_order(GoodBuyUpToOrder&& buy_up_to_order) {
	const std::lock_guard<std::mutex> lock_guard { *buy_lock };
	buy_up_to_orders.push_back(std::move(buy_up_to_order));
}

void GoodMarket::add_market_sell_order(GoodMarketSellOrder&& market_sell_order) {
	const std::lock_guard<std::mutex> lock_guard { *sell_lock };
	market_sell_orders.push_back(std::move(market_sell_order));
}

void GoodMarket::execute_orders(
	IndexedMap<CountryInstance, fixed_point_t>& reusable_country_map_0,
	IndexedMap<CountryInstance, fixed_point_t>& reusable_country_map_1,
	std::vector<fixed_point_t>& reusable_vector_0,
	std::vector<fixed_point_t>& reusable_vector_1
) {
	if (!is_available) {
		//price remains the same
		price_change_yesterday
			= quantity_traded_yesterday
			= total_demand_yesterday
			= total_supply_yesterday
			= 0;

		for (GoodBuyUpToOrder const& buy_up_to_order : buy_up_to_orders) {
			buy_up_to_order.call_after_trade(BuyResult::no_purchase_result(good_definition));
		}

		for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
			market_sell_order.call_after_trade(SellResult::no_sales_result(), reusable_vector_0);
		}
		return;
	}

	fixed_point_t new_price;
	//MarketInstance ensured only orders with quantity > 0 are added.
	//So running total > 0 unless orders are empty.
	fixed_point_t demand_sum = 0,
		supply_sum = 0;
	if (market_sell_orders.empty()) {
		quantity_traded_yesterday = 0;
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
			if (demand_sum > fixed_point_t::_0) {
				new_price = max_next_price;
			} else {
				new_price = price;
			}
		}
	} else {
		IndexedMap<CountryInstance, fixed_point_t>& supply_per_country = reusable_country_map_0;
		IndexedMap<CountryInstance, fixed_point_t>& actual_bought_per_country = reusable_country_map_1;
		for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
			CountryInstance const* const country_nullable = market_sell_order.get_country_nullable();
			if (country_nullable != nullptr) {
				supply_per_country[*country_nullable] += market_sell_order.get_quantity();
			}
			supply_sum += market_sell_order.get_quantity();
		}
		std::vector<fixed_point_t>& quantity_bought_per_order = reusable_vector_0;
		std::vector<fixed_point_t>& purchasing_power_per_order = reusable_vector_1;
		quantity_bought_per_order.resize(buy_up_to_orders.size());
		purchasing_power_per_order.resize(buy_up_to_orders.size());

		fixed_point_t money_left_to_spend_sum = 0; //sum of money_to_spend for all buyers that can't afford their max_quantity
		fixed_point_t max_quantity_to_buy_sum = 0;
		fixed_point_t purchasing_power_sum = 0;
		for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
			GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];
			const fixed_point_t max_quantity = buy_up_to_order.get_max_quantity();
			const fixed_point_t money_to_spend = buy_up_to_order.get_money_to_spend();

			if (game_rules_manager.get_use_optimal_pricing()) {
				const fixed_point_t affordable_price = buy_up_to_order.get_affordable_price();
				if (affordable_price > min_next_price) {
					//no point selling lower as it would not attract more buyers
					min_next_price = affordable_price;
				}
			}

			demand_sum += max_quantity;

			if (money_to_spend <= fixed_point_t::_0) {
				purchasing_power_per_order[i] = 0;
				continue;
			}

			fixed_point_t purchasing_power = purchasing_power_per_order[i] = money_to_spend / max_next_price;
			if (purchasing_power >= max_quantity) {
				max_quantity_to_buy_sum += max_quantity;
				money_left_to_spend_sum += max_quantity * max_next_price;
				purchasing_power_sum += max_quantity;
			} else {
				max_quantity_to_buy_sum += purchasing_power;
				money_left_to_spend_sum += money_to_spend;
				purchasing_power_sum += purchasing_power;
			}
		}

		fixed_point_t remaining_supply = supply_sum;
		const bool is_selling_for_max_price = max_quantity_to_buy_sum >= supply_sum;
		if (is_selling_for_max_price) {
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
					const fixed_point_t max_quantity = buy_up_to_order.get_max_quantity();
					fixed_point_t& distributed_supply = quantity_bought_per_order[i];
					if (distributed_supply == max_quantity) {
						continue;
					}

					CountryInstance const* const country_nullable = buy_up_to_order.get_country_nullable();
					if (country_nullable != nullptr) {
						//subtract as it might be updated below
						actual_bought_per_country[*country_nullable] -= distributed_supply;
					}

					distributed_supply = fixed_point_t::mul_div(
						remaining_supply,
						purchasing_power_per_order[i],
						purchasing_power_sum
					);

					if (distributed_supply >= max_quantity) {
						someone_bought_max_quantity = true;
						distributed_supply = max_quantity;
						remaining_supply -= max_quantity;
						purchasing_power_sum -= purchasing_power_per_order[i];
					}

					if (country_nullable != nullptr) {
						actual_bought_per_country[*country_nullable] += distributed_supply;
					}

					if (someone_bought_max_quantity) {
						break;
					}
				}
			} while (someone_bought_max_quantity);

			execute_buy_orders(
				new_price,
				actual_bought_per_country,
				supply_per_country,
				quantity_bought_per_order
			);
		} else {
			//sell below max_next_price
			if (game_rules_manager.get_use_optimal_pricing()) {
				//drop price while remaining_supply > 0 && new_price > min_next_price
				while (remaining_supply > fixed_point_t::_0) {
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

			//figure out how much every buyer bought
			for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
				GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];

				const fixed_point_t quantity_bought
					= quantity_bought_per_order[i]
					= std::min(
					buy_up_to_order.get_max_quantity(),
					buy_up_to_order.get_money_to_spend() / new_price
				);

				CountryInstance const* const country_nullable = buy_up_to_order.get_country_nullable();
				if (country_nullable != nullptr) {
					actual_bought_per_country[*country_nullable] += quantity_bought_per_order[i];
				}
			}

			execute_buy_orders(
				new_price,
				actual_bought_per_country,
				supply_per_country,
				quantity_bought_per_order
			);
		}
		reusable_vector_0.clear();
		reusable_vector_1.clear();

		//figure out how much of each order was sold
		if (quantity_traded_yesterday == supply_sum) {
			//everything was sold
			for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
				const fixed_point_t quantity_sold = market_sell_order.get_quantity();
				fixed_point_t money_gained;
				if (quantity_sold == fixed_point_t::_0) {
					money_gained = 0;
				} else {
					money_gained = std::max(
						quantity_sold * new_price,
						fixed_point_t::epsilon //round up
					);
				}
				market_sell_order.call_after_trade(
					{
						quantity_sold,
						money_gained
					},
					reusable_vector_0
				);
			}
		} else {
			//quantity is evenly divided after taking domestic buyers into account
			fixed_point_t total_quantity_traded_domestically = 0;
			for (auto const& [country, actual_bought] : actual_bought_per_country) {
				const fixed_point_t supply = supply_per_country[country];
				const fixed_point_t traded_domestically = std::min(supply, actual_bought);
				total_quantity_traded_domestically += traded_domestically;
			}

			const fixed_point_t total_quantity_traded_as_export = quantity_traded_yesterday - total_quantity_traded_domestically;
			const fixed_point_t total_quantity_offered_as_export = supply_sum - total_quantity_traded_domestically;
			for (GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
				const fixed_point_t quantity_offered = market_sell_order.get_quantity();

				fixed_point_t quantity_sold_domestically;
				fixed_point_t quantity_offered_as_export;
				CountryInstance const* const country_nullable = market_sell_order.get_country_nullable();
				if (country_nullable == nullptr) {
					quantity_sold_domestically = 0;
					quantity_offered_as_export = quantity_offered;
				} else {
					const fixed_point_t total_bought_domestically = actual_bought_per_country[*country_nullable];
					const fixed_point_t total_domestic_supply = supply_per_country[*country_nullable];
					quantity_sold_domestically = total_bought_domestically >= total_domestic_supply
						? market_sell_order.get_quantity()
						: fixed_point_t::mul_div(
							market_sell_order.get_quantity(),
							total_bought_domestically,
							total_domestic_supply //> 0 as we're selling
						);
					quantity_offered_as_export = quantity_offered - quantity_sold_domestically;
				}

				const fixed_point_t fair_share_of_exports = fixed_point_t::mul_div(
					quantity_offered_as_export,
					total_quantity_traded_as_export,
					total_quantity_offered_as_export
				);

				const fixed_point_t quantity_sold = quantity_sold_domestically + fair_share_of_exports;
				fixed_point_t money_gained;
				if (quantity_sold == fixed_point_t::_0) {
					money_gained = 0;
				} else {
					money_gained = std::max(
						quantity_sold * new_price,
						fixed_point_t::epsilon //round up
					);
				}
				market_sell_order.call_after_trade(
					{
						quantity_sold,
						money_gained
					},
					reusable_vector_0
				);
			}
		}

		reusable_country_map_0.clear();
		reusable_country_map_1.clear();
		market_sell_orders.clear();
		supply_per_country.clear();
		actual_bought_per_country.clear();
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

void GoodMarket::execute_buy_orders(
	const fixed_point_t new_price,
	IndexedMap<CountryInstance, fixed_point_t> const& actual_bought_per_country,
	IndexedMap<CountryInstance, fixed_point_t> const& supply_per_country,
	std::span<const fixed_point_t> quantity_bought_per_order
) {
	quantity_traded_yesterday = 0;
	for (size_t i = 0; i < buy_up_to_orders.size(); i++) {
		GoodBuyUpToOrder const& buy_up_to_order = buy_up_to_orders[i];
		const fixed_point_t quantity_bought = quantity_bought_per_order[i];

		if (quantity_bought == fixed_point_t::_0) {
			buy_up_to_order.call_after_trade(BuyResult::no_purchase_result(good_definition));
		} else {
			quantity_traded_yesterday += quantity_bought;
			const fixed_point_t money_spent_total = std::max(
				quantity_bought * new_price,
				fixed_point_t::epsilon //we know from purchasing power that you can afford it.
			);

			fixed_point_t money_spent_on_imports;			
			CountryInstance const* const country_nullable = buy_up_to_order.get_country_nullable();
			if (country_nullable == nullptr) {
				//could be trade between native Americans and tribal Africa, so it's all imported
				money_spent_on_imports = money_spent_total;
			} else {
				//must be > 0, since quantity_bought > 0
				const fixed_point_t actual_bought_in_my_country = actual_bought_per_country[*country_nullable];
				const fixed_point_t supply_in_my_country = supply_per_country[*country_nullable];

				if (supply_in_my_country >= actual_bought_in_my_country) {
					//no imports
					money_spent_on_imports = 0;
				} else {
					const fixed_point_t money_spent_domestically = fixed_point_t::mul_div(
						money_spent_total,
						supply_in_my_country,
						actual_bought_in_my_country
					);

					money_spent_on_imports = money_spent_total - money_spent_domestically;
				}
			}
			buy_up_to_order.call_after_trade({
				good_definition,
				quantity_bought,
				money_spent_total,
				money_spent_on_imports
			});
		}
	}
}

void GoodMarket::record_price_history() {
	price_history.push_back(price);
}