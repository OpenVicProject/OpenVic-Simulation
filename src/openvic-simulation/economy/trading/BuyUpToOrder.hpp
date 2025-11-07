#pragma once

#include "openvic-simulation/economy/trading/BuyResult.hpp"
#include "openvic-simulation/core/Property.hpp"

namespace OpenVic {
	struct CountryInstance;

	struct GoodBuyUpToOrder {
		using actor_t = void*;
		using callback_t = void(*)(const actor_t, BuyResult const&);

	private:
		CountryInstance const* const PROPERTY(country_nullable);
		const fixed_point_t PROPERTY(max_quantity);
		const fixed_point_t PROPERTY(money_to_spend);
		const actor_t actor;
		const callback_t after_trade;

	public:
		constexpr GoodBuyUpToOrder(
			CountryInstance const* const new_country_nullable,
			const fixed_point_t new_max_quantity,
			const fixed_point_t new_money_to_spend,
			const actor_t new_actor,
			const callback_t new_after_trade
		) : country_nullable { new_country_nullable },
			max_quantity { new_max_quantity },
			money_to_spend { new_money_to_spend },
			actor { new_actor },
			after_trade { new_after_trade }
			{}

		constexpr void call_after_trade(BuyResult const& buy_result) const {
			after_trade(actor, buy_result);
		}

		//highest price per unit at which the buyer can afford max_quantity
		constexpr fixed_point_t get_affordable_price() const {
			return money_to_spend / max_quantity;
		}
	};

	struct BuyUpToOrder : GoodBuyUpToOrder {
	private:
		GoodDefinition const& PROPERTY(good);

	public:
		constexpr BuyUpToOrder(
			GoodDefinition const& new_good,
			CountryInstance const* const new_country_nullable,
			const fixed_point_t new_max_quantity,
			const fixed_point_t new_money_to_spend,
			const actor_t new_actor,
			const callback_t new_after_trade
		) : GoodBuyUpToOrder {
				new_country_nullable,
				new_max_quantity,
				new_money_to_spend,
				new_actor,
				new_after_trade
			},
			good { new_good }
			{}

			constexpr void call_after_trade(BuyResult const& buy_result) const {
				GoodBuyUpToOrder::call_after_trade(buy_result);
			}
	};
}