#pragma once

#include "openvic-simulation/economy/trading/BuyResult.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
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