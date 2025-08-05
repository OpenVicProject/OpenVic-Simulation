#include "GoodInstance.hpp"

#include <tuple>

#include "openvic-simulation/economy/GoodDefinition.hpp"

using namespace OpenVic;

GoodInstance::GoodInstance(
	GoodDefinition const* new_good_definition,
	GameRulesManager const* new_game_rules_manager
) : HasIdentifierAndColour { *new_good_definition },
  	HasIndex<GoodInstance> { new_good_definition->get_index() },
	GoodMarket { *new_game_rules_manager, *new_good_definition }
	{}

GoodInstanceManager::GoodInstanceManager(
	GoodDefinitionManager const& new_good_definition_manager,
	GameRulesManager const& game_rules_manager
) : good_definition_manager { new_good_definition_manager },
	good_instance_by_definition {
		new_good_definition_manager.get_good_definitions(),
		[game_rules_manager_ptr=&game_rules_manager](GoodDefinition const& good_definition) -> auto {
			return std::make_tuple(&good_definition, game_rules_manager_ptr);
		}
	} { assert(new_good_definition_manager.good_definitions_are_locked()); }

bool GoodInstance::is_trading_good() const {
	return is_available && !get_good_definition().get_is_money();
}

GoodInstance* GoodInstanceManager::get_good_instance_by_identifier(std::string_view identifier) {
	GoodDefinition const* good_definition = good_definition_manager.get_good_definition_by_identifier(identifier);
	return good_definition == nullptr
		? nullptr
		: &get_good_instance_by_definition(*good_definition);
}
GoodInstance const* GoodInstanceManager::get_good_instance_by_identifier(std::string_view identifier) const {
	GoodDefinition const* good_definition = good_definition_manager.get_good_definition_by_identifier(identifier);
	return good_definition == nullptr
		? nullptr
		: &get_good_instance_by_definition(*good_definition);
}
GoodInstance* GoodInstanceManager::get_good_instance_by_index(typename GoodInstance::index_t index) {
	return good_instance_by_definition.contains_index(index)
		? &good_instance_by_definition.at_index(index)
		: nullptr;
}
GoodInstance const* GoodInstanceManager::get_good_instance_by_index(typename GoodInstance::index_t index) const {
	return good_instance_by_definition.contains_index(index)
		? &good_instance_by_definition.at_index(index)
		: nullptr;
}
GoodInstance& GoodInstanceManager::get_good_instance_by_definition(GoodDefinition const& good_definition) {
	return good_instance_by_definition.at(good_definition);
}
GoodInstance const& GoodInstanceManager::get_good_instance_by_definition(GoodDefinition const& good_definition) const {
	return good_instance_by_definition.at(good_definition);
}

void GoodInstanceManager::enable_good(GoodDefinition const& good) {
	get_good_instance_by_definition(good).is_available = true;
}
