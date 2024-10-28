#include "MarketSellOrder.hpp"

using namespace OpenVic;

GoodMarketSellOrder::GoodMarketSellOrder(
	const fixed_point_t new_quantity,
	const std::function<void(const SellResult)> new_after_trade
):
	quantity { new_quantity },
	after_trade { new_after_trade }
	{}

MarketSellOrder::MarketSellOrder(
	GoodDefinition const& new_good,
	const fixed_point_t new_quantity,
	const std::function<void(const SellResult)> new_after_trade
): GoodMarketSellOrder(new_quantity, new_after_trade),
	good { &new_good }
	{}