#pragma once

#include "openvic-simulation/economy/BuildingTypeManager.hpp"
#include "openvic-simulation/economy/GoodDefinitionManager.hpp"
#include "openvic-simulation/economy/production/ProductionTypeManager.hpp"

namespace ovdl::v2script {
	class Parser;
}

namespace OpenVic {
	struct EconomyManager {
	private:
		BuildingTypeManager PROPERTY_REF(building_type_manager);
		GoodDefinitionManager PROPERTY_REF(good_definition_manager);
		ProductionTypeManager PROPERTY_REF(production_type_manager);

	public:
		inline bool load_production_types_file(
			GameRulesManager const& game_rules_manager,
			PopManager const& pop_manager,
			ovdl::v2script::Parser const& parser
		) {
			return production_type_manager.load_production_types_file(
				game_rules_manager,
				good_definition_manager,
				pop_manager,
				parser
			);
		}

		inline bool load_buildings_file(ModifierManager& modifier_manager, ovdl::v2script::ast::Node const* root) {
			return building_type_manager.load_buildings_file(
				good_definition_manager, production_type_manager, modifier_manager, root
			);
		}
	};
}
