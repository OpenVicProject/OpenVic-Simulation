#pragma once

#include <cassert>

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/politics/Ideology.hpp"
#include "openvic-simulation/pop/Pop.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"

namespace OpenVic {
	struct Map;
	struct Region;
	struct State;
	struct Crime;
	struct Good;
	struct TerrainType;
	struct TerrainTypeMapping;
	struct ProvinceHistoryEntry;
	struct ProvinceSetModifier;
	using Climate = ProvinceSetModifier;
	using Continent = ProvinceSetModifier;

	/* REQUIREMENTS:
	 * MAP-5, MAP-7, MAP-8, MAP-43, MAP-47
	 * POP-22
	 */
	struct Province : HasIdentifierAndColour {
		friend struct Map;

		using index_t = uint16_t;
		using life_rating_t = int8_t;
		using distance_t = fixed_point_t;

		enum struct colony_status_t : uint8_t { STATE, PROTECTORATE, COLONY };

		struct adjacency_t {
			using data_t = uint8_t;
			static constexpr data_t NO_CANAL = 0;

			enum struct type_t : uint8_t {
				LAND,       /* Between two land provinces */
				WATER,      /* Between two water provinces */
				COASTAL,    /* Between a land province and a water province */
				IMPASSABLE, /* Between two land provinces (non-traversable) */
				STRAIT,     /* Between two land provinces with a water through province */
				CANAL       /* Between two water provinces with a land through province */
			};

			/* Type display name used for logging */
			static std::string_view get_type_name(type_t type);

		private:
			Province const* PROPERTY(to);
			Province const* PROPERTY(through);
			distance_t PROPERTY(distance);
			type_t PROPERTY(type);
			data_t PROPERTY(data); // represents canal index, 0 for non-canal adjacencies

		public:
			adjacency_t(
				Province const* new_to, distance_t new_distance, type_t new_type, Province const* new_through, data_t new_data
			);
			adjacency_t(adjacency_t const&) = delete;
			adjacency_t(adjacency_t&&) = default;
			adjacency_t& operator=(adjacency_t const&) = delete;
			adjacency_t& operator=(adjacency_t&&) = default;
		};

		struct province_positions_t {
			/* Calculated average */
			fvec2_t centre;

			/* Province name placement */
			fvec2_t text;
			fixed_point_t text_rotation;
			fixed_point_t text_scale;

			/* Model positions */
			std::optional<fvec2_t> unit;
			fvec2_t city;
			fvec2_t factory;
			fvec2_t building_construction;
			fvec2_t military_construction;
			ordered_map<BuildingType const*, fvec2_t> building_position;
			fixed_point_map_t<BuildingType const*> building_rotation;
		};

		static constexpr index_t NULL_INDEX = 0, MAX_INDEX = std::numeric_limits<index_t>::max();

	private:
		/* Immutable attributes (unchanged after initial game load) */
		const index_t PROPERTY(index);
		Region const* PROPERTY(region);
		Climate const* PROPERTY(climate);
		Continent const* PROPERTY(continent);
		bool PROPERTY(on_map);
		bool PROPERTY(has_region);
		bool PROPERTY_CUSTOM_PREFIX(water, is);
		bool PROPERTY_CUSTOM_PREFIX(coastal, is);
		bool PROPERTY_CUSTOM_PREFIX(port, has);
		/* Terrain type calculated from terrain image */
		TerrainType const* PROPERTY(default_terrain_type);

		std::vector<adjacency_t> PROPERTY(adjacencies);
		province_positions_t PROPERTY(positions);

		/* Mutable attributes (reset before loading history) */
		TerrainType const* PROPERTY(terrain_type);
		life_rating_t PROPERTY(life_rating);
		colony_status_t PROPERTY(colony_status);
		State const* PROPERTY_RW(state);
		Country const* PROPERTY(owner);
		Country const* PROPERTY(controller);
		std::vector<Country const*> PROPERTY(cores);
		bool PROPERTY(slave);
		Crime const* PROPERTY_RW(crime);
		// TODO - change this into a factory-like structure
		Good const* PROPERTY(rgo);
		IdentifierRegistry<BuildingInstance> IDENTIFIER_REGISTRY(building);

		std::vector<Pop> PROPERTY(pops);
		Pop::pop_size_t PROPERTY(total_population);
		fixed_point_map_t<PopType const*> PROPERTY(pop_type_distribution);
		fixed_point_map_t<Ideology const*> PROPERTY(ideology_distribution);
		fixed_point_map_t<Culture const*> PROPERTY(culture_distribution);
		fixed_point_map_t<Religion const*> PROPERTY(religion_distribution);

		Province(std::string_view new_identifier, colour_t new_colour, index_t new_index);

	public:
		Province(Province&&) = default;

		bool operator==(Province const& other) const;
		std::string to_string() const;

		bool load_positions(BuildingTypeManager const& building_type_manager, ast::NodeCPtr root);

		bool expand_building(size_t building_index);

		bool add_pop(Pop&& pop);
		bool add_pop_vec(std::vector<Pop> const& pop_vec);
		size_t get_pop_count() const;
		void update_pops();

		void update_gamestate(Date today);
		void tick(Date today);

		adjacency_t const* get_adjacency_to(Province const* province) const;
		bool is_adjacent_to(Province const* province) const;
		std::vector<adjacency_t const*> get_adjacencies_going_through(Province const* province) const;
		bool has_adjacency_going_through(Province const* province) const;

		fvec2_t get_unit_position() const;

		bool reset(BuildingTypeManager const& building_type_manager);
		bool apply_history_to_province(ProvinceHistoryEntry const* entry);
	};
}
