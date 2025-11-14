#pragma once

#include <optional>

#include "openvic-simulation/economy/trading/BuyResult.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct GoodBuyUpToOrder {
		using actor_t = void*;
		using callback_t = void(*)(const actor_t, BuyResult const&);

	private:
		const actor_t actor;
		const callback_t after_trade;

	public:
		const std::optional<country_index_t> country_index_optional;
		const fixed_point_t max_quantity;
		const fixed_point_t money_to_spend;

		constexpr GoodBuyUpToOrder(
			const std::optional<country_index_t> new_country_index_optional,
			const fixed_point_t new_max_quantity,
			const fixed_point_t new_money_to_spend,
			const actor_t new_actor,
			const callback_t new_after_trade
		) : country_index_optional { new_country_index_optional },
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
	public:
		GoodDefinition const& good;

		constexpr BuyUpToOrder(
			GoodDefinition const& new_good,
			const std::optional<country_index_t> new_country_index_optional,
			const fixed_point_t new_max_quantity,
			const fixed_point_t new_money_to_spend,
			const actor_t new_actor,
			const callback_t new_after_trade
		) : GoodBuyUpToOrder {
				new_country_index_optional,
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