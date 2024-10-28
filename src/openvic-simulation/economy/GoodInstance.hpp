#pragma once

#include <deque>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodInstanceManager;

	struct GoodInstance : HasIdentifierAndColour {
		friend struct GoodInstanceManager;

	private:
		GoodDefinition const& PROPERTY(good_definition);
		fixed_point_t PROPERTY(price);
		bool PROPERTY(is_available);
		fixed_point_t PROPERTY(total_supply_yesterday);
		std::deque<GoodMarketSellOrder> market_sell_orders;
		
		GoodInstance(GoodDefinition const& new_good_definition);

	public:
		GoodInstance(GoodInstance&&) = default;

		//thread safe
		void add_market_sell_order(const GoodMarketSellOrder market_sell_order);

		//not thread safe
		void execute_orders();
	};

	struct GoodInstanceManager {
	private:
		IdentifierRegistry<GoodInstance> IDENTIFIER_REGISTRY(good_instance);

	public:
		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(good_instance);
		bool setup(GoodDefinitionManager const& good_definition_manager);
	};
}
