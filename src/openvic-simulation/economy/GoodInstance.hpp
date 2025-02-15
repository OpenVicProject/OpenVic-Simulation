#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct GoodInstanceManager;

	struct GoodInstance : HasIdentifierAndColour, GoodMarket {
		friend struct GoodInstanceManager;

	private:
		GoodInstance(GoodDefinition const& new_good_definition, GameRulesManager const& new_game_rules_manager);

	public:
		GoodInstance(GoodInstance&&) = default;

		// Is the good available for trading? (e.g. should be shown in trade menu)
		// is_tradeable has no effect on this, only is_money and availability
		constexpr bool is_trading_good() const {
			return is_available && !good_definition.get_is_money();
		}
	};

	struct GoodInstanceManager {
	private:
		GoodDefinitionManager const& PROPERTY(good_definition_manager);

		IdentifierRegistry<GoodInstance> IDENTIFIER_REGISTRY(good_instance);

	public:
		GoodInstanceManager(GoodDefinitionManager const& new_good_definition_manager);

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(good_instance);
		bool setup_goods(GameRulesManager const& game_rules_manager);

		GoodInstance& get_good_instance_from_definition(GoodDefinition const& good);
		GoodInstance const& get_good_instance_from_definition(GoodDefinition const& good) const;

		void enable_good(GoodDefinition const& good);
	};
}
