#pragma once

#include <memory>
#include <mutex>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/ValueHistory.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodInstanceManager;

	struct GoodInstance : HasIdentifierAndColour {
		friend struct GoodInstanceManager;

	private:
		GoodDefinition const& PROPERTY(good_definition);

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

		fixed_point_t PROPERTY(price);
		fixed_point_t PROPERTY(price_inverse);
		fixed_point_t PROPERTY(price_change_yesterday);
		fixed_point_t PROPERTY(max_next_price);
		fixed_point_t PROPERTY(min_next_price);
		bool PROPERTY(is_available);
		fixed_point_t PROPERTY(total_demand_yesterday);
		fixed_point_t PROPERTY(total_supply_yesterday);
		fixed_point_t PROPERTY(quantity_traded_yesterday);
		ValueHistory<fixed_point_t> PROPERTY(price_history);

		GoodInstance(GoodDefinition const& new_good_definition, GameRulesManager const& new_game_rules_manager);

		void update_next_price_limits();
	public:
		GoodInstance(GoodInstance&&) = default;

		// Is the good available for trading? (e.g. should be shown in trade menu)
		// is_tradeable has no effect on this, only is_money and availability
		constexpr bool is_trading_good() const {
			return is_available && !good_definition.get_is_money();
		}

		//thread safe
		void add_buy_up_to_order(GoodBuyUpToOrder&& buy_up_to_order);
		void add_market_sell_order(GoodMarketSellOrder&& market_sell_order);

		//not thread safe
		void execute_orders();
		void on_use_exponential_price_changes_changed();
		void record_price_history();
	};

	struct GoodInstanceManager {
	private:
		GoodDefinitionManager const& PROPERTY(good_definition_manager);

		IdentifierRegistry<GoodInstance> IDENTIFIER_REGISTRY(good_instance);

	public:
		GoodInstanceManager(GoodDefinitionManager const& new_good_definition_manager);

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(good_instance);
		bool setup_goods(GameRulesManager const& game_rules_manager);

		GoodInstance& get_good_instance_from_definition(GoodDefinition const& good);
		GoodInstance const& get_good_instance_from_definition(GoodDefinition const& good) const;

		void enable_good(GoodDefinition const& good);
	};
}
