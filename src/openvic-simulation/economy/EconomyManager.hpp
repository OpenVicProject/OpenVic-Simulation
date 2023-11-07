#pragma once

#include "openvic-simulation/economy/Building.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"

namespace OpenVic {
	struct EconomyManager {
	private:
		BuildingManager building_manager;
		GoodManager good_manager;
		ProductionTypeManager production_type_manager;

	public:
		REF_GETTERS(building_manager)
		REF_GETTERS(good_manager)
		REF_GETTERS(production_type_manager)

		inline bool load_production_types_file(PopManager const& pop_manager, ast::NodeCPtr root) {
			return production_type_manager.load_production_types_file(good_manager, pop_manager, root);
		}

		inline bool load_buildings_file(ModifierManager& modifier_manager, ast::NodeCPtr root) {
			return building_manager.load_buildings_file(good_manager, production_type_manager, modifier_manager, root);
		}
	};
}
