#pragma once

#include <optional>

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/economy/BuildingLevel.hpp"
#include "openvic-simulation/economy/BuildingRestrictionCategory.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct BuildingTypeManager;
	struct CountryInstance;
	struct GoodDefinition;
	struct GoodDefinitionManager;
	struct ModifierEffectCache;
	struct ProductionType;
	struct ProductionTypeManager;
	struct ProvinceDefinition;
	struct ProvinceInstance;

	/* REQUIREMENTS:
	 * MAP-11, MAP-72, MAP-73
	 * MAP-12, MAP-75, MAP-76
	 * MAP-13, MAP-78, MAP-79
	 */
	struct BuildingType : HasIndex<BuildingType, building_type_index_t>, Modifier {
		friend BuildingTypeManager;
		using naval_capacity_t = uint64_t;

		struct building_type_args_t {
			std::string_view type, on_completion;
			ModifierValue modifier;
			fixed_point_t completion_size = 0, cost = 0, colonial_range = 0, infrastructure = 0;
			building_level_t max_level = building_level_t { 0 }, fort_level = building_level_t { 0 };
			fixed_point_map_t<GoodDefinition const*> goods_cost;
			Timespan build_time;
			bool on_map = false, default_enabled = false, pop_build_factory = false, strategic_factory = false,
				advanced_factory = false, in_province = false, one_per_state = false, spawn_railway_track = false,
				sail = false, steam = false, capital = false, port = false;
			ProductionType const* production_type = nullptr;
			naval_capacity_t naval_capacity = 0;
			memory::vector<fixed_point_t> colonial_points;

			building_type_args_t() {};
			building_type_args_t(building_type_args_t&&) = default;
		};

	private:
		const memory::string type;
		const bool is_in_province;
		const bool is_port;
		const bool capital; // only in naval base

		const memory::string on_completion; //unknown


		fixed_point_map_t<GoodDefinition const*> PROPERTY(goods_cost);
		memory::vector<fixed_point_t> SPAN_PROPERTY(colonial_points);

	public:
		//general attributes
		const std::optional<province_building_index_t> province_building_index;

		const bool is_pop_build_factory;
		const bool is_strategic_factory;
		const bool is_advanced_factory;
		const bool is_sail; // only in clipper shipyard
		const bool is_steam; // only in steamer shipyard

		const bool should_display_on_map;
		const bool should_spawn_railway_track;

		const bool is_limited_to_one_per_state;
		const BuildingRestrictionCategory restriction_category;
		const bool is_enabled_by_default;
		const building_level_t max_level;
		const fixed_point_t completion_size;

		//costs
		const fixed_point_t cost;
		const Timespan build_time;

		//effects
		ProductionType const* const production_type;
		const building_level_t fort_level; // fort bonus step-per-level
		const naval_capacity_t naval_capacity;
		const fixed_point_t colonial_range;
		const fixed_point_t infrastructure;

		BuildingType(
			index_t new_index,
			std::optional<province_building_index_t> new_province_building_index,
			std::string_view new_identifier,
			building_type_args_t& building_type_args
		);
		BuildingType(BuildingType&&) = default;

		[[nodiscard]] bool can_be_built_in(
			ModifierEffectCache const& modifier_effect_cache,
			const building_level_t desired_level,
			CountryInstance const& actor,
			ProvinceInstance const& location
		) const;
		[[nodiscard]] bool can_be_built_in(ProvinceDefinition const& location) const;
	};

	struct BuildingTypeManager {
	private:
		IdentifierRegistry<BuildingType> IDENTIFIER_REGISTRY(building_type);
		string_set_t PROPERTY(building_type_types);
		memory::vector<BuildingType const*> SPAN_PROPERTY(province_building_types);
		BuildingType const* PROPERTY(infrastructure_building_type);
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
