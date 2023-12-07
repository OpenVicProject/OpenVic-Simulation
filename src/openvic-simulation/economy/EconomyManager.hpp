#pragma once

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"

namespace OpenVic {
	struct EconomyManager {
	private:
		BuildingTypeManager PROPERTY_REF(building_type_manager);
		GoodManager PROPERTY_REF(good_manager);
		ProductionTypeManager PROPERTY_REF(production_type_manager);

	public:
		inline bool load_production_types_file(PopManager const& pop_manager, ast::NodeCPtr root) {
			return production_type_manager.load_production_types_file(good_manager, pop_manager, root);
		}

		inline bool load_buildings_file(ModifierManager& modifier_manager, ast::NodeCPtr root) {
			return building_type_manager.load_buildings_file(good_manager, production_type_manager, modifier_manager, root);
		}
	};
}
