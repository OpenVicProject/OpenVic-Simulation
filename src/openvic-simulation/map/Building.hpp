#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/economy/ProductionType.hpp"

#define ARGS BuildingType const& type, sound_t on_completion, fixed_point_t completion_size, level_t max_level, std::map<const Good*, fixed_point_t> build_cost, \
				Timespan build_time, bool visibility, bool on_map, const bool default_enabled, ProductionType const* production_type, bool pop_build_factory, \
				bool strategic_factory, level_t fort_level, uint64_t naval_capacity, std::vector<uint64_t> colonial_points, bool in_province, bool one_per_state, \
				uint64_t colonial_range, fixed_point_t infrastructure, fixed_point_t movement_cost, bool spawn_railway_track

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

		using level_t = uint8_t;
		using sound_t = std::string_view;

	private:
		BuildingType const& type;
		const sound_t on_completion; //probably sound played on completion
		const fixed_point_t completion_size;
		const level_t max_level;
		const std::map<const Good*, fixed_point_t> build_cost;
		const Timespan build_time; //time
		const bool visibility;
		const bool on_map; //onmap
		
		const bool default_enabled; //some(false)
		ProductionType const* production_type;
		const bool pop_build_factory;
		const bool strategic_factory;

		const level_t fort_level; //some(0), probably the step-per-level?

		const uint64_t naval_capacity; //some(0)
		const std::vector<uint64_t> colonial_points;
		const bool in_province; //province some(false)
		const bool one_per_state;
		const uint64_t colonial_range;

		const fixed_point_t infrastructure;
		const fixed_point_t movement_cost;
		const bool spawn_railway_track; //some(false)

		//any modifiers really

		Building(std::string_view identifier, ARGS);

	public:
		Building(Building&&) = default;

		BuildingType const& get_type() const;
		sound_t get_on_completion() const;
		fixed_point_t get_completion_size() const;
		level_t get_max_level() const;
		std::map<const Good*, fixed_point_t> const& get_build_cost() const;
		Timespan get_build_time() const;
		bool has_visibility() const;
		bool is_on_map() const;

		bool is_default_enabled() const;
		ProductionType const* get_production_type() const;
		bool is_pop_built_factory() const;
		bool is_strategic_factory() const;
		
		level_t get_fort_level() const;

		uint64_t get_naval_capacity() const;
		std::vector<uint64_t> const& get_colonial_points() const;
		bool is_in_province() const;
		bool is_one_per_state() const;
		uint64_t get_colonial_range() const;
		
		fixed_point_t get_infrastructure() const;
		fixed_point_t get_movement_cost() const;
		bool spawned_railway_track() const;
	};

	struct BuildingType : HasIdentifier {
		friend struct BuildingManager;

	private:
		BuildingType(const std::string_view new_identifier);

	public:
		BuildingType(BuildingType&&) = default;
	};

	enum class ExpansionState {
		CannotExpand,
		CanExpand,
		Preparing,
		Expanding
	};

	struct BuildingInstance : HasIdentifier { //used in the actual game
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
		Date const& get_start_date() const;
		Date const& get_end_date() const;
		float get_expansion_progress() const;

		bool expand();
		void update_state(Date const& today);
		void tick(Date const& today);

	};

	struct Province;

	struct BuildingManager {
	private:
		IdentifierRegistry<BuildingType> building_types;
		IdentifierRegistry<Building> buildings;

	public:
		BuildingManager();

		bool add_building_type(const std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(BuildingType, building_type)
		bool generate_province_buildings(Province& province) const;

		bool add_building(std::string_view identifier);
		IDENTIFIER_REGISTRY_ACCESSORS(Building, building)

		//bool load_buildings_file(ast::NodeCPtr root);
	};
}
