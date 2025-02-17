#pragma once

#include <memory>
#include <mutex>

#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/ValueHistory.hpp"

namespace OpenVic {
	struct GameRulesManager;

	struct GoodMarket {
	private:
		static constexpr int32_t exponential_price_change_shift = 7;
		std::unique_ptr<std::mutex> buy_lock;
		std::unique_ptr<std::mutex> sell_lock;
		GameRulesManager const& game_rules_manager;
		fixed_point_t absolute_maximum_price;
		fixed_point_t absolute_minimum_price;

		//TODO get from pool instead of storing as field
		//only used inside execute_orders()
		std::vector<fixed_point_t> quantity_bought_per_order;
		std::vector<fixed_point_t> purchasing_power_per_order;

		//only used during day tick (from actors placing order until execute_orders())
		std::vector<GoodBuyUpToOrder> buy_up_to_orders;
		std::vector<GoodMarketSellOrder> market_sell_orders;

	protected:
		GoodDefinition const& PROPERTY_ACCESS(good_definition, protected);
		bool PROPERTY_ACCESS(is_available, protected);

	private:
		fixed_point_t PROPERTY(price);
		fixed_point_t PROPERTY(price_inverse);
		fixed_point_t PROPERTY(price_change_yesterday);
		fixed_point_t PROPERTY(max_next_price);
		fixed_point_t PROPERTY(min_next_price);
		fixed_point_t PROPERTY(total_demand_yesterday);
		fixed_point_t PROPERTY(total_supply_yesterday);
		fixed_point_t PROPERTY(quantity_traded_yesterday);
		ValueHistory<fixed_point_t> PROPERTY(price_history);

		void update_next_price_limits();
	public:
		GoodMarket(GameRulesManager const& new_game_rules_manager, GoodDefinition const& new_good_definition);
		GoodMarket(GoodMarket&&) = default;

		//thread safe
		void add_buy_up_to_order(GoodBuyUpToOrder&& buy_up_to_order);
		void add_market_sell_order(GoodMarketSellOrder&& market_sell_order);

		//not thread safe
		void execute_orders();
		void on_use_exponential_price_changes_changed();
		void record_price_history();
	};
}