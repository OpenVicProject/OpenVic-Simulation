#pragma once

#include "openvic-simulation/economy/trading/SellResult.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct GoodDefinition;

	struct GoodMarketSellOrder {
		using actor_t = void*;
		using callback_t = void (*)(actor_t, SellResult const&, memory::vector<fixed_point_t>&);

	private:
		const actor_t actor;
		const callback_t after_trade;

	public:
		CountryInstance const* const country_nullable;
		const fixed_point_t quantity;

		constexpr GoodMarketSellOrder(
			CountryInstance const* const new_country_nullable,
			const fixed_point_t new_quantity,
			const actor_t new_actor,
			const callback_t new_after_trade
		) : country_nullable { new_country_nullable },
			quantity { new_quantity },
			actor { new_actor },
			after_trade { new_after_trade }
			{}

		constexpr void call_after_trade(SellResult const& sell_result, memory::vector<fixed_point_t>& reusable_vector) const {
			after_trade(actor, sell_result, reusable_vector);
		}
	};

	struct MarketSellOrder : GoodMarketSellOrder {
	public:
		GoodDefinition const& good;

		constexpr MarketSellOrder(
			GoodDefinition const& new_good,
			CountryInstance const* const new_country_nullable,
			const fixed_point_t new_quantity,
			const actor_t new_actor,
			const callback_t new_after_trade
		) : GoodMarketSellOrder {
				new_country_nullable,
				new_quantity,
				new_actor,
				new_after_trade
			},
			good { new_good }
			{}
	};
}