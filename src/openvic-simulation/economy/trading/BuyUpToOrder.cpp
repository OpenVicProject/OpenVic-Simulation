#include "BuyUpToOrder.hpp"

using namespace OpenVic;

GoodBuyUpToOrder::GoodBuyUpToOrder(
	const fixed_point_t new_max_quantity,
	const fixed_point_t new_money_to_spend,
	std::function<void(const BuyResult)>&& new_after_trade
) : max_quantity { new_max_quantity },
	money_to_spend { new_money_to_spend },
	after_trade { std::move(new_after_trade) }
	{}

BuyUpToOrder::BuyUpToOrder(
	GoodDefinition const& new_good,
	const fixed_point_t new_max_quantity,
	const fixed_point_t new_money_to_spend,
	std::function<void(const BuyResult)>&& new_after_trade
) : GoodBuyUpToOrder(
		new_max_quantity,
		new_money_to_spend,
		std::move(new_after_trade)
	),
 	good { new_good }
	{}