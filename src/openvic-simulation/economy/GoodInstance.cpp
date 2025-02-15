#include "GoodInstance.hpp"

using namespace OpenVic;

GoodInstance::GoodInstance(GoodDefinition const& new_good_definition, GameRulesManager const& new_game_rules_manager)
  : HasIdentifierAndColour { new_good_definition },
	GoodMarket { new_game_rules_manager, new_good_definition }
	{}

GoodInstanceManager::GoodInstanceManager(GoodDefinitionManager const& new_good_definition_manager)
	: good_definition_manager { new_good_definition_manager } {}

bool GoodInstanceManager::setup_goods(GameRulesManager const& game_rules_manager) {
	if (good_instances_are_locked()) {
		Logger::error("Cannot set up good instances - they are already locked!");
		return false;
	}

	good_instances.reserve(good_definition_manager.get_good_definition_count());

	bool ret = true;

	for (GoodDefinition const& good : good_definition_manager.get_good_definitions()) {
		ret &= good_instances.add_item({ good, game_rules_manager });
	}

	lock_good_instances();

	return ret;
}

GoodInstance& GoodInstanceManager::get_good_instance_from_definition(GoodDefinition const& good) {
	return good_instances.get_items()[good.get_index()];
}

GoodInstance const& GoodInstanceManager::get_good_instance_from_definition(GoodDefinition const& good) const {
	return good_instances.get_items()[good.get_index()];
}

void GoodInstanceManager::enable_good(GoodDefinition const& good) {
	get_good_instance_from_definition(good).is_available = true;
}
