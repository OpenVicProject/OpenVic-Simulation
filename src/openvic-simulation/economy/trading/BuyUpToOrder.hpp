#pragma once

#include "openvic-simulation/economy/trading/BuyResult.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodDefinition;

	struct GoodBuyUpToOrder {
	private:
		const fixed_point_t PROPERTY(max_quantity);
		const fixed_point_t PROPERTY(money_to_spend);
		std::function<void(const BuyResult)> PROPERTY(after_trade);

	public:
		GoodBuyUpToOrder(
			const fixed_point_t new_max_quantity,
			const fixed_point_t new_money_to_spend,
			std::function<void(const BuyResult)>&& new_after_trade
		);
		GoodBuyUpToOrder(GoodBuyUpToOrder&&) = default;

		//highest price per unit at which the buyer can afford max_quantity
		constexpr fixed_point_t get_affordable_price() const {
			return money_to_spend / max_quantity;
		}
	};

	struct BuyUpToOrder : GoodBuyUpToOrder {
	private:
		GoodDefinition const& PROPERTY(good);

	public:
		BuyUpToOrder(
			GoodDefinition const& new_good,
			const fixed_point_t new_max_quantity,
			const fixed_point_t new_money_to_spend,
			std::function<void(const BuyResult)>&& new_after_trade
		);
		BuyUpToOrder(BuyUpToOrder&&) = default;
	};
}