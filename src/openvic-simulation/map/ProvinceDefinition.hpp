#pragma once

#include <optional>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {

	struct MapDefinition;
	struct Region;
	struct TerrainType;
	struct ProvinceSetModifier;
	using Climate = ProvinceSetModifier;
	using Continent = ProvinceSetModifier;
	struct BuildingType;
	struct BuildingTypeManager;

	/* REQUIREMENTS:
	 * MAP-5, MAP-7, MAP-8, MAP-43, MAP-47
	 * POP-22
	 */
	struct ProvinceDefinition : HasIdentifierAndColour, HasIndex<ProvinceDefinition, uint16_t> {
		friend struct MapDefinition;

		using distance_t = fixed_point_t; // should this go inside adjacency_t?

		struct adjacency_t {
			using data_t = uint8_t;
			static constexpr data_t DEFAULT_DATA = 0;

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
			ProvinceDefinition const* PROPERTY(to);
			ProvinceDefinition const* PROPERTY(through);
			distance_t PROPERTY(distance);
			type_t PROPERTY(type);
			data_t PROPERTY(data); // represents canal index, 0 for non-canal adjacencies

		public:
			adjacency_t(
				ProvinceDefinition const* new_to, distance_t new_distance, type_t new_type,
				ProvinceDefinition const* new_through, data_t new_data
			);
			adjacency_t(adjacency_t const&) = delete;
			adjacency_t(adjacency_t&&) = default;
			adjacency_t& operator=(adjacency_t const&) = delete;
			adjacency_t& operator=(adjacency_t&&) = default;
		};

		struct province_positions_t {
			/* Province name placement */
			std::optional<fvec2_t> text_position;
			std::optional<fixed_point_t> text_rotation;
			std::optional<fixed_point_t> text_scale;

			/* Model positions */
			std::optional<fvec2_t> unit;
			std::optional<fvec2_t> city;
			std::optional<fvec2_t> town;
			std::optional<fvec2_t> factory;
			std::optional<fvec2_t> building_construction;
			std::optional<fvec2_t> military_construction;
			ordered_map<BuildingType const*, fvec2_t> building_position;
			fixed_point_map_t<BuildingType const*> building_rotation;
		};

		static constexpr index_t NULL_INDEX = 0, MAX_INDEX = std::numeric_limits<index_t>::max();

	private:
		/* Immutable attributes (unchanged after initial game load) */
		Region const* PROPERTY(region, nullptr);
		Climate const* PROPERTY(climate, nullptr);
		Continent const* PROPERTY(continent, nullptr);
		bool PROPERTY(on_map, false);
		bool PROPERTY_CUSTOM_PREFIX(water, is, false);
		bool PROPERTY_CUSTOM_PREFIX(coastal, is, false);
		bool PROPERTY_CUSTOM_PREFIX(port, has, false);
		ProvinceDefinition const* PROPERTY(port_adjacent_province, nullptr);
		/* Terrain type calculated from terrain image */
		TerrainType const* PROPERTY(default_terrain_type, nullptr);

		memory::vector<adjacency_t> PROPERTY(adjacencies);
		/* Calculated mean pixel position. */
		fvec2_t PROPERTY(centre);
		province_positions_t positions {};

		ProvinceDefinition(std::string_view new_identifier, colour_t new_colour, index_t new_index);

	public:
		ProvinceDefinition(ProvinceDefinition&&) = default;

		memory::string to_string() const;

		inline constexpr bool has_region() const {
			return region != nullptr;
		}

		/* The positions' y coordinates need to be inverted. */
		bool load_positions(
			MapDefinition const& map_definition, BuildingTypeManager const& building_type_manager, ast::NodeCPtr root
		);

		fvec2_t get_text_position() const;
		fixed_point_t get_text_rotation() const;
		fixed_point_t get_text_scale() const;

		/* This returns a pointer to the position of the specified building type, or nullptr if none exists. */
		fvec2_t const* get_building_position(BuildingType const* building_type) const;
		fixed_point_t get_building_rotation(BuildingType const* building_type) const;

		adjacency_t const* get_adjacency_to(ProvinceDefinition const* province) const;
		bool is_adjacent_to(ProvinceDefinition const* province) const;
		memory::vector<adjacency_t const*> get_adjacencies_going_through(ProvinceDefinition const* province) const;
		bool has_adjacency_going_through(ProvinceDefinition const* province) const;

		fvec2_t get_unit_position() const;
		fvec2_t get_city_position() const;
		fvec2_t get_town_position() const;
		fvec2_t get_factory_position() const;
		fvec2_t get_building_construction_position() const;
		fvec2_t get_military_construction_position() const;
	};
}
