#pragma once

#include "openvic-simulation/economy/trading/SellResult.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodDefinition;

	struct GoodMarketSellOrder {
		using actor_t = void*;
		using callback_t = void (*)(actor_t, SellResult const&);

	private:
		const fixed_point_t PROPERTY(quantity);
		actor_t actor;
		callback_t after_trade;

	public:
		constexpr GoodMarketSellOrder(
			const fixed_point_t new_quantity,
			actor_t new_actor,
			callback_t new_after_trade
		) : quantity { new_quantity },
			actor { new_actor },
			after_trade { new_after_trade }
			{}

		constexpr void call_after_trade(SellResult const& sell_result) const {
			after_trade(actor, sell_result);
		}
	};

	struct MarketSellOrder : GoodMarketSellOrder {
	private:
		GoodDefinition const& PROPERTY(good);

	public:
		constexpr MarketSellOrder(
			GoodDefinition const& new_good,
			const fixed_point_t new_quantity,
			actor_t new_actor,
			callback_t new_after_trade
		) : GoodMarketSellOrder { new_quantity, new_actor, new_after_trade },
			good { new_good }
			{}
	};
}