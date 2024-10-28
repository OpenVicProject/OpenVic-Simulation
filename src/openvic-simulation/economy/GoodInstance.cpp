#include "GoodInstance.hpp"

using namespace OpenVic;

GoodInstance::GoodInstance(GoodDefinition const& new_good_definition)
  : HasIdentifierAndColour { new_good_definition },
  	good_definition { new_good_definition },
	price { new_good_definition.get_base_price() },
	is_available { new_good_definition.get_is_available_from_start() },
	total_supply_yesterday { fixed_point_t::_0() },
	market_sell_orders {}
	{}

void GoodInstance::add_market_sell_order(const GoodMarketSellOrder market_sell_order) {
	market_sell_orders.push_back(market_sell_order);
}

void GoodInstance::execute_orders() {
	const fixed_point_t price = get_price();

	fixed_point_t supply_running_total = fixed_point_t::_0();
	for(GoodMarketSellOrder const& market_sell_order : market_sell_orders) {
		const fixed_point_t market_sell_quantity = market_sell_order.get_quantity();
		supply_running_total += market_sell_quantity;
		market_sell_order.get_after_trade()({
			market_sell_quantity,
			market_sell_quantity * price
		});
	}

	total_supply_yesterday = supply_running_total;
	market_sell_orders.clear();
}

bool GoodInstanceManager::setup(GoodDefinitionManager const& good_definition_manager) {
	if (good_instances_are_locked()) {
		Logger::error("Cannot set up good instances - they are already locked!");
		return false;
	}

	good_instances.reserve(good_definition_manager.get_good_definition_count());

	bool ret = true;

	for (GoodDefinition const& good : good_definition_manager.get_good_definitions()) {
		ret &= good_instances.add_item({ good });
	}

	lock_good_instances();

	return ret;
}
