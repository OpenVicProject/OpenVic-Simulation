#pragma once

#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct GameRulesManager;
	struct GoodDefinition;
	struct GoodInstanceManager;

	struct GoodInstance : HasIdentifierAndColour, HasIndex<GoodInstance, good_index_t>, GoodMarket {
		friend struct GoodInstanceManager;

	public:
		//pointers instead of references to allow construction via std::tuple
		GoodInstance(
			GoodDefinition const* new_good_definition,
			GameRulesManager const* new_game_rules_manager
		);
		GoodInstance(GoodInstance const&) = delete;
		GoodInstance& operator=(GoodInstance const&) = delete;
		GoodInstance(GoodInstance&&) = delete;
		GoodInstance& operator=(GoodInstance&&) = delete;

		// Is the good available for trading? (e.g. should be shown in trade menu)
		// is_tradeable has no effect on this, only is_money and availability
		bool is_trading_good() const;
	};
}
