#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/trading/SellResult.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodMarketSellOrder {
	private:
		const fixed_point_t PROPERTY(quantity);
		const std::function<void(const SellResult)> PROPERTY(after_trade);

	public:
		GoodMarketSellOrder(
			const fixed_point_t new_quantity,
			const std::function<void(const SellResult)> new_after_trade
		);
	};

	struct MarketSellOrder : GoodMarketSellOrder {
	private:
		GoodDefinition const* const PROPERTY(good);

	public:
		MarketSellOrder(
			GoodDefinition const& new_good,
			const fixed_point_t new_quantity,
			const std::function<void(const SellResult)> new_after_trade
		);
	};
}