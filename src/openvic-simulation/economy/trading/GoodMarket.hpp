#pragma once

#include <mutex>

#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IndexedMap.hpp"
#include "openvic-simulation/types/ValueHistory.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct GameRulesManager;
	struct GoodDefinition;

	struct GoodMarket {
	private:
		GoodDefinition const& PROPERTY(good_definition);

		static constexpr int32_t exponential_price_change_shift = 7;
		memory::unique_ptr<std::mutex> buy_lock = memory::make_unique<std::mutex>();
		memory::unique_ptr<std::mutex> sell_lock = memory::make_unique<std::mutex>();
		GameRulesManager const& game_rules_manager;
		fixed_point_t absolute_maximum_price;
		fixed_point_t absolute_minimum_price;

		//only used during day tick (from actors placing order until execute_orders())
		memory::vector<GoodBuyUpToOrder> buy_up_to_orders;
		memory::vector<GoodMarketSellOrder> market_sell_orders;

		void execute_buy_orders(
			const fixed_point_t new_price,
			IndexedMap<CountryInstance, fixed_point_t> const& actual_bought_per_country,
			IndexedMap<CountryInstance, fixed_point_t> const& supply_per_country,
			std::span<const fixed_point_t> quantity_bought_per_order
		);

	protected:
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
		static constexpr size_t VECTORS_FOR_EXECUTE_ORDERS = 2;
		void execute_orders(
			IndexedMap<CountryInstance, fixed_point_t>& reusable_country_map_0,
			IndexedMap<CountryInstance, fixed_point_t>& reusable_country_map_1,
			utility::forwardable_span<
				memory::vector<fixed_point_t>,
				VECTORS_FOR_EXECUTE_ORDERS
			> reusable_vectors
		);
		void on_use_exponential_price_changes_changed();
		void record_price_history();
	};
}