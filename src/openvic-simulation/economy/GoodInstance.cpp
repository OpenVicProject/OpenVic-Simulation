#include "GoodInstance.hpp"

using namespace OpenVic;

GoodInstance::GoodInstance(GoodDefinition const& new_good_definition)
  : HasIdentifierAndColour { new_good_definition }, good_definition { new_good_definition },
	price { new_good_definition.get_base_price() }, available { new_good_definition.is_available_from_start() } {}

bool GoodInstanceManager::setup_good_instances(GoodDefinitionManager const& good_definition_manager) {
	good_instances.reset();
	good_instances.reserve(good_definition_manager.get_good_definition_count());

	bool ret = true;

	for (GoodDefinition const& good : good_definition_manager.get_good_definitions()) {
		ret &= good_instances.add_item({ good });
	}

	return ret;
}
