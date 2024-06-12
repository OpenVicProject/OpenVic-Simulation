#pragma once

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"

namespace OpenVic {
	struct EconomyManager {
	private:
		BuildingTypeManager PROPERTY_REF(building_type_manager);
		GoodDefinitionManager PROPERTY_REF(good_definition_manager);
		GoodInstanceManager PROPERTY_REF(good_instance_manager);
		ProductionTypeManager PROPERTY_REF(production_type_manager);

	public:
		inline bool load_production_types_file(PopManager const& pop_manager, ast::NodeCPtr root) {
			return production_type_manager.load_production_types_file(good_definition_manager, pop_manager, root);
		}

		inline bool load_buildings_file(ModifierManager& modifier_manager, ast::NodeCPtr root) {
			return building_type_manager.load_buildings_file(
				good_definition_manager, production_type_manager, modifier_manager, root
			);
		}

		inline bool setup_good_instances() {
			return good_instance_manager.setup_good_instances(good_definition_manager);
		}
	};
}
