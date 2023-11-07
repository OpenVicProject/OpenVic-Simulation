#pragma once

#include "openvic-simulation/Modifier.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#define ARGS \
	ModifierValue&& modifier, std::string_view on_completion, fixed_point_t completion_size, level_t max_level, \
	Good::good_map_t&& goods_cost, fixed_point_t cost, Timespan build_time, bool visibility, bool on_map, \
	bool default_enabled, ProductionType const* production_type, bool pop_build_factory, bool strategic_factory, \
	bool advanced_factory, level_t fort_level, uint64_t naval_capacity, std::vector<fixed_point_t>&& colonial_points, \
	bool in_province, bool one_per_state, fixed_point_t colonial_range, fixed_point_t infrastructure, \
	bool spawn_railway_track, bool sail, bool steam, bool capital, bool port

namespace OpenVic {

	struct BuildingManager;
	struct BuildingType;

	/* REQUIREMENTS:
	 * MAP-11, MAP-72, MAP-73
	 * MAP-12, MAP-75, MAP-76
	 * MAP-13, MAP-78, MAP-79
	 */
	struct Building : HasIdentifier {
		friend struct BuildingManager;

		using level_t = int16_t;

	private:
		BuildingType const& type;
		const ModifierValue modifier;
		const std::string on_completion; // probably sound played on completion
		const fixed_point_t completion_size;
		const level_t max_level;
		const Good::good_map_t goods_cost;
		const fixed_point_t cost;
		const Timespan build_time; // time
		const bool visibility;
		const bool on_map; // onmap

		const bool default_enabled;
		ProductionType const* production_type;
		const bool pop_build_factory;
		const bool strategic_factory;
		const bool advanced_factory;

		const level_t fort_level; // probably the step-per-level

		const uint64_t naval_capacity;
		const std::vector<fixed_point_t> colonial_points;
		const bool in_province; // province
		const bool one_per_state;
		const fixed_point_t colonial_range;

		const fixed_point_t infrastructure;
		const bool spawn_railway_track;

		const bool sail; // only in clipper shipyard
		const bool steam; // only in steamer shipyard
		const bool capital; // only in naval base
		const bool port; // only in naval base

		Building(std::string_view identifier, BuildingType const& type, ARGS);

	public:
		Building(Building&&) = default;

		BuildingType const& get_type() const;
		ModifierValue const& get_modifier() const;
		std::string_view get_on_completion() const;
		fixed_point_t get_completion_size() const;
		level_t get_max_level() const;
		Good::good_map_t const& get_goods_cost() const;
		fixed_point_t get_cost() const;
		Timespan get_build_time() const;
		bool has_visibility() const;
		bool is_on_map() const;

		bool is_default_enabled() const;
		ProductionType const* get_production_type() const;
		bool is_pop_built_factory() const;
		bool is_strategic_factory() const;
		bool is_advanced_factory() const;

		level_t get_fort_level() const;

		uint64_t get_naval_capacity() const;
		std::vector<fixed_point_t> const& get_colonial_points() const;
		bool is_in_province() const;
		bool is_one_per_state() const;
		fixed_point_t get_colonial_range() const;

		fixed_point_t get_infrastructure() const;
		bool spawned_railway_track() const;
	};

	struct BuildingType : HasIdentifier {
		friend struct BuildingManager;

	private:
		BuildingType(std::string_view new_identifier);

	public:
		BuildingType(BuildingType&&) = default;
	};

	enum class ExpansionState { CannotExpand, CanExpand, Preparing, Expanding };

	struct BuildingInstance : HasIdentifier { // used in the actual game
		friend struct BuildingManager;
		using level_t = Building::level_t;

	private:
		Building const& building;

		level_t level = 0;
		ExpansionState expansion_state = ExpansionState::CannotExpand;
		Date start, end;
		float expansion_progress;

		bool _can_expand() const;

		BuildingInstance(Building const& building);

	public:
		BuildingInstance(BuildingInstance&&) = default;

		Building const& get_building() const;

		level_t get_current_level() const;
		ExpansionState get_expansion_state() const;
		Date get_start_date() const;
		Date get_end_date() const;
		float get_expansion_progress() const;

		bool expand();
		void update_state(Date today);
		void tick(Date today);
	};

	struct Province;

	struct BuildingManager {
		using level_t = Building::level_t; // this is getting ridiculous

	private:
		IdentifierRegistry<BuildingType> building_types;
		IdentifierRegistry<Building> buildings;

	public:
		BuildingManager();

		bool add_building_type(std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(building_type)

		bool add_building(std::string_view identifier, BuildingType const* type, ARGS);
		IDENTIFIER_REGISTRY_ACCESSORS(building)

		bool load_buildings_file(
			GoodManager const& good_manager, ProductionTypeManager const& production_type_manager,
			ModifierManager& modifier_manager, ast::NodeCPtr root
		);

		bool generate_province_buildings(Province& province) const;
	};
}
