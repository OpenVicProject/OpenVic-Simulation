#pragma once

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#define ARGS \
	std::string_view type, ModifierValue&& modifier, std::string_view on_completion, fixed_point_t completion_size, \
	level_t max_level, Good::good_map_t&& goods_cost, fixed_point_t cost, Timespan build_time, bool visibility, bool on_map, \
	bool default_enabled, ProductionType const* production_type, bool pop_build_factory, bool strategic_factory, \
	bool advanced_factory, level_t fort_level, uint64_t naval_capacity, std::vector<fixed_point_t>&& colonial_points, \
	bool in_province, bool one_per_state, fixed_point_t colonial_range, fixed_point_t infrastructure, \
	bool spawn_railway_track, bool sail, bool steam, bool capital, bool port

namespace OpenVic {

	struct BuildingTypeManager;

	/* REQUIREMENTS:
	 * MAP-11, MAP-72, MAP-73
	 * MAP-12, MAP-75, MAP-76
	 * MAP-13, MAP-78, MAP-79
	 */
	struct BuildingType : HasIdentifier {
		friend struct BuildingTypeManager;

		using level_t = int16_t;

	private:
		std::string PROPERTY(type);
		ModifierValue PROPERTY(modifier);
		std::string PROPERTY(on_completion); // probably sound played on completion
		fixed_point_t PROPERTY(completion_size);
		level_t PROPERTY(max_level);
		Good::good_map_t PROPERTY(goods_cost);
		fixed_point_t PROPERTY(cost);
		Timespan PROPERTY(build_time); // time
		bool PROPERTY(visibility);
		bool PROPERTY(on_map); // onmap

		bool PROPERTY(default_enabled);
		ProductionType const* PROPERTY(production_type);
		bool PROPERTY(pop_build_factory);
		bool PROPERTY(strategic_factory);
		bool PROPERTY(advanced_factory);

		level_t PROPERTY(fort_level); // probably the step-per-level

		uint64_t PROPERTY(naval_capacity);
		std::vector<fixed_point_t> PROPERTY(colonial_points);
		bool PROPERTY(in_province); // province
		bool PROPERTY(one_per_state);
		fixed_point_t PROPERTY(colonial_range);

		fixed_point_t PROPERTY(infrastructure);
		bool PROPERTY(spawn_railway_track);

		bool PROPERTY(sail); // only in clipper shipyard
		bool PROPERTY(steam); // only in steamer shipyard
		bool PROPERTY(capital); // only in naval base
		bool PROPERTY(port); // only in naval base

		BuildingType(std::string_view identifier, ARGS);

	public:
		BuildingType(BuildingType&&) = default;
	};

	struct BuildingTypeManager {
		using level_t = BuildingType::level_t; // this is getting ridiculous

	private:
		IdentifierRegistry<BuildingType> building_types;

	public:
		BuildingTypeManager();

		bool add_building_type(std::string_view identifier, ARGS);
		IDENTIFIER_REGISTRY_ACCESSORS(building_type)

		bool load_buildings_file(
			GoodManager const& good_manager, ProductionTypeManager const& production_type_manager,
			ModifierManager& modifier_manager, ast::NodeCPtr root
		);
	};
}
