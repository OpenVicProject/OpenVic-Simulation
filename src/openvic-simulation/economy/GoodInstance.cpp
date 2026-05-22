#include "GoodInstance.hpp"

#include "openvic-simulation/economy/GoodDefinition.hpp"

using namespace OpenVic;

GoodInstance::GoodInstance(
	GoodDefinition const* new_good_definition,
	GameRulesManager const* new_game_rules_manager
) : HasIdentifierAndColour { *new_good_definition },
  	HasIndex { new_good_definition->index },
	GoodMarket { *new_game_rules_manager, *new_good_definition }
	{}

bool GoodInstance::is_trading_good() const {
	return is_available && !good_definition.is_money;
}
