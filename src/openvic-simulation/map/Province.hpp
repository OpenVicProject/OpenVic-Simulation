#pragma once

#include <cassert>

#include "openvic-simulation/economy/Building.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/pop/Pop.hpp"

namespace OpenVic {
	struct Map;
	struct Region;
	struct Good;
	struct TerrainType;
	struct TerrainTypeMapping;

	/* REQUIREMENTS:
	 * MAP-5, MAP-7, MAP-8, MAP-43, MAP-47
	 * POP-22
	 */
	struct Province : HasIdentifierAndColour {
		friend struct Map;

		using index_t = uint16_t;
		using life_rating_t = int8_t;
		using distance_t = uint16_t;
		using flags_t = uint16_t;

		enum struct colony_status_t : int8_t { STATE, PROTECTORATE, COLONY };

		struct adjacency_t {
			friend struct Province;

		private:
			Province const* const province;
			const distance_t distance;
			flags_t flags;

			adjacency_t(Province const* province, distance_t distance, flags_t flags);

		public:
			distance_t get_distance() const;
			flags_t get_flags() const;
		};

		struct province_positions_t {
			fvec2_t text;
			fixed_point_t text_rotation;
			fixed_point_t text_scale;
			fvec2_t unit;
			fvec2_t city;
			fvec2_t factory;
			fvec2_t building_construction;
			fvec2_t military_construction;
			fvec2_t fort;
			fixed_point_t fort_rotation;
			fvec2_t railroad;
			fixed_point_t railroad_rotation;
			fvec2_t navalbase;
			fixed_point_t navalbase_rotation;
		};

		static constexpr index_t NULL_INDEX = 0, MAX_INDEX = std::numeric_limits<index_t>::max();

	private:
		const index_t index;
		Region* region = nullptr;
		bool on_map = false, has_region = false, water = false;
		life_rating_t life_rating = 0;
		colony_status_t colony_status = colony_status_t::STATE;
		IdentifierRegistry<BuildingInstance> buildings;
		// TODO - change this into a factory-like structure
		Good const* rgo = nullptr;

		std::vector<Pop> pops;
		Pop::pop_size_t total_population;
		decimal_map_t<PopType const*> PROPERTY(pop_type_distribution);
		decimal_map_t<Ideology const*> PROPERTY(ideology_distribution);
		decimal_map_t<Culture const*> PROPERTY(culture_distribution);
		decimal_map_t<Religion const*> PROPERTY(religion_distribution);

		std::vector<adjacency_t> adjacencies;
		province_positions_t positions;

		TerrainType const* terrain_type = nullptr;

		void _set_terrain_type(TerrainType const* type);

		Province(std::string_view new_identifier, colour_t new_colour, index_t new_index);

	public:
		Province(Province&&) = default;

		index_t get_index() const;
		Region* get_region() const;
		bool get_on_map() const;
		bool get_has_region() const;
		bool get_water() const;
		TerrainType const* get_terrain_type() const;
		life_rating_t get_life_rating() const;
		colony_status_t get_colony_status() const;
		bool load_positions(BuildingManager const& building_manager, ast::NodeCPtr root);

		bool add_building(BuildingInstance&& building_instance);
		IDENTIFIER_REGISTRY_ACCESSORS(building)
		void reset_buildings();
		bool expand_building(std::string_view building_type_identifier);
		Good const* get_rgo() const;
		std::string to_string() const;

		bool load_pop_list(PopManager const& pop_manager, ast::NodeCPtr root);
		bool add_pop(Pop&& pop);
		void clear_pops();
		size_t get_pop_count() const;
		std::vector<Pop> const& get_pops() const;
		Pop::pop_size_t get_total_population() const;
		void update_pops();

		void update_state(Date today);
		void tick(Date today);

		bool is_adjacent_to(Province const* province);
		bool add_adjacency(Province const* province, distance_t distance, flags_t flags);
		std::vector<adjacency_t> const& get_adjacencies() const;
	};
}
