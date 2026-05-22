#pragma once

#include <functional>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/core/stl/containers/TypedSpan.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct GoodDefinitionManager;
	struct ModifierEffectCache;
	struct ProductionTypeManager;

	struct BuildingTypeManager {
	private:
		IdentifierRegistry<BuildingType> IDENTIFIER_REGISTRY(building_type);
		string_set_t PROPERTY(building_type_types);
		BuildingType const* PROPERTY(infrastructure_building_type);
		BuildingType const* PROPERTY(port_building_type);

		memory::vector<std::reference_wrapper<const BuildingType>> province_building_types;
	public:
		constexpr TypedSpan<province_building_index_t, const std::reference_wrapper<const BuildingType>> get_province_building_types() const {
			return province_building_types;
		}

		BuildingTypeManager();

		bool add_building_type(std::string_view identifier, BuildingType::building_type_args_t& building_type_args);

		bool load_buildings_file(
			GoodDefinitionManager const& good_definition_manager, ProductionTypeManager const& production_type_manager,
			ModifierManager& modifier_manager, ovdl::v2script::ast::Node const* root
		);
	};
}
