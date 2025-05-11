#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {

	struct BuildingTypeManager;

	/* REQUIREMENTS:
	 * MAP-11, MAP-72, MAP-73
	 * MAP-12, MAP-75, MAP-76
	 * MAP-13, MAP-78, MAP-79
	 */
	struct BuildingType : Modifier {
		friend struct BuildingTypeManager;

		using level_t = int16_t;
		using naval_capacity_t = uint64_t;

		struct building_type_args_t {
			std::string_view type, on_completion;
			ModifierValue modifier;
			fixed_point_t completion_size = 0, cost = 0, colonial_range = 0, infrastructure = 0;
			BuildingType::level_t max_level = 0, fort_level = 0;
			GoodDefinition::good_definition_map_t goods_cost;
			Timespan build_time;
			bool on_map = false, default_enabled = false, pop_build_factory = false, strategic_factory = false,
				advanced_factory = false, in_province = false, one_per_state = false, spawn_railway_track = false,
				sail = false, steam = false, capital = false, port = false;
			ProductionType const* production_type = nullptr;
			naval_capacity_t naval_capacity = 0;
			memory::vector<fixed_point_t> colonial_points;

			building_type_args_t() = default;
			building_type_args_t(building_type_args_t&&) = default;
		};

	private:
		memory::string PROPERTY(type);
		memory::string PROPERTY(on_completion); // probably sound played on completion
		fixed_point_t PROPERTY(completion_size);
		level_t PROPERTY(max_level);
		GoodDefinition::good_definition_map_t PROPERTY(goods_cost);
		fixed_point_t PROPERTY(cost);
		Timespan PROPERTY(build_time); // time
		bool PROPERTY(on_map); // onmap

		bool PROPERTY_CUSTOM_PREFIX(default_enabled, is);
		ProductionType const* PROPERTY(production_type);
		bool PROPERTY(pop_build_factory);
		bool PROPERTY(strategic_factory);
		bool PROPERTY(advanced_factory);

		level_t PROPERTY(fort_level); // fort bonus step-per-level

		naval_capacity_t PROPERTY(naval_capacity);
		memory::vector<fixed_point_t> PROPERTY(colonial_points);
		bool PROPERTY_CUSTOM_PREFIX(in_province, is); // province
		bool PROPERTY(one_per_state);
		fixed_point_t PROPERTY(colonial_range);

		fixed_point_t PROPERTY(infrastructure);
		bool PROPERTY(spawn_railway_track);

		bool PROPERTY(sail); // only in clipper shipyard
		bool PROPERTY(steam); // only in steamer shipyard
		bool PROPERTY(capital); // only in naval base
		bool PROPERTY_CUSTOM_PREFIX(port, is); // only in naval base

		BuildingType(std::string_view identifier, building_type_args_t& building_type_args);

	public:
		BuildingType(BuildingType&&) = default;
	};

	struct BuildingTypeManager {
		using level_t = BuildingType::level_t; // this is getting ridiculous

	private:
		IdentifierRegistry<BuildingType> IDENTIFIER_REGISTRY(building_type);
		string_set_t PROPERTY(building_type_types);
		memory::vector<BuildingType const*> PROPERTY(province_building_types);
		BuildingType const* PROPERTY(port_building_type);

	public:
		BuildingTypeManager();

		bool add_building_type(std::string_view identifier, BuildingType::building_type_args_t& building_type_args);

		bool load_buildings_file(
			GoodDefinitionManager const& good_definition_manager, ProductionTypeManager const& production_type_manager,
			ModifierManager& modifier_manager, ast::NodeCPtr root
		);
	};
}
