#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include <openvic-dataloader/csv/LineObject.hpp>

#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/pathfinding/PointMap.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/BMP.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	namespace fs = std::filesystem;

	struct BuildingTypeManager;
	struct ModifierManager;

	struct RiverSegment {
		friend struct MapDefinition;

	private:
		memory::vector<ivec2_t> PROPERTY(points);

	public:
		const uint8_t size;

		RiverSegment(uint8_t new_size, memory::vector<ivec2_t>&& new_points);
		RiverSegment(RiverSegment&&) = default;
	};

	/* REQUIREMENTS:
	 * MAP-4
	 */
	struct MapDefinition {
#pragma pack(push, 1)
		/* Used to represent tightly packed 3-byte integer pixel information. */
		struct shape_pixel_t {
			ProvinceDefinition::province_number_t province_number;
			TerrainTypeMapping::index_t terrain;
		};
#pragma pack(pop)

	private:
		using colour_index_map_t = ordered_map<colour_t, ProvinceDefinition::index_t>;
		using river_t = memory::vector<RiverSegment>;

		IdentifierRegistry<ProvinceDefinition> IDENTIFIER_REGISTRY(province_definition);
		IdentifierRegistry<Region> IDENTIFIER_REGISTRY(region);
		IdentifierRegistry<Climate> IDENTIFIER_REGISTRY(climate);
		IdentifierRegistry<Continent> IDENTIFIER_REGISTRY(continent);
		ProvinceSet water_provinces;
		TerrainTypeManager PROPERTY_REF(terrain_type_manager);

		memory::vector<river_t> PROPERTY(rivers); // TODO: calculate provinces affected by crossing
		void _trace_river(BMP& rivers_bmp, ivec2_t start, river_t& river);

		ivec2_t PROPERTY(dims, { 0, 0 });
		memory::vector<shape_pixel_t> PROPERTY(province_shape_image);
		colour_index_map_t colour_index_map;

		ProvinceDefinition::index_t PROPERTY(max_provinces);

		PointMap PROPERTY_REF(path_map_land);
		PointMap PROPERTY_REF(path_map_sea);

		ProvinceDefinition::province_number_t get_province_number_from_colour(colour_t colour) const;
		bool _generate_standard_province_adjacencies();

		inline constexpr int32_t get_pixel_index_from_pos(ivec2_t pos) const {
			return pos.x + pos.y * dims.x;
		}

		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(province_definition);
		ProvinceDefinition* get_province_definition_from_number(
			const ProvinceDefinition::province_number_t province_number
		);

	public:
		MapDefinition();

		ProvinceDefinition const* get_province_definition_from_number(
			const ProvinceDefinition::province_number_t province_number
		) const;

		inline constexpr int32_t get_width() const { return dims.x; }
		inline constexpr int32_t get_height() const { return dims.y; }

		bool add_province_definition(std::string_view identifier, colour_t colour);

		ProvinceDefinition::distance_t calculate_distance_between(
			ProvinceDefinition const& from, ProvinceDefinition const& to
		) const;
		bool add_standard_adjacency(ProvinceDefinition& from, ProvinceDefinition& to);
		bool add_special_adjacency(
			ProvinceDefinition& from, ProvinceDefinition& to, ProvinceDefinition::adjacency_t::type_t type,
			ProvinceDefinition const* through, ProvinceDefinition::adjacency_t::data_t data
		);

		bool set_water_province(std::string_view identifier);
		bool set_water_province_list(memory::vector<std::string_view> const& list);
		void lock_water_provinces();

		size_t get_land_province_count() const;
		size_t get_water_province_count() const;

		ProvinceDefinition::province_number_t get_province_number_at(ivec2_t pos) const;

	private:
		ProvinceDefinition* get_province_definition_at(ivec2_t pos);

		/* This provides a safe way to remove the const qualifier of a ProvinceDefinition const*, via a non-const Map.
		 * It uses a const_cast (the fastest/simplest solution), but this could also be done without it by looking up the
		 * ProvinceDefinition* using the ProvinceDefinition const*'s index. Requiring a non-const Map ensures that this
		 * function can only be used where the ProvinceDefinition* could already be accessed by other means, such as the
		 * index method, preventing misleading code, or in the worst case undefined behaviour. */
		constexpr ProvinceDefinition* remove_province_definition_const(ProvinceDefinition const* province) {
			return const_cast<ProvinceDefinition*>(province);
		}

	public:
		ProvinceDefinition const* get_province_definition_at(ivec2_t pos) const;
		bool set_max_provinces(ProvinceDefinition::index_t new_max_provinces);

		bool add_region(std::string_view identifier, memory::vector<ProvinceDefinition const*>&& provinces, colour_t colour);

		bool load_province_definitions(std::span<const ovdl::csv::LineObject> lines);
		/* Must be loaded after adjacencies so we know what provinces are coastal, and so can have a port */
		bool load_province_positions(BuildingTypeManager const& building_type_manager, ast::NodeCPtr root);
		static bool load_region_colours(ast::NodeCPtr root, memory::vector<colour_t>& colours);
		bool load_region_file(ast::NodeCPtr root, std::span<const colour_t> colours);
		bool load_map_images(fs::path const& province_path, fs::path const& terrain_path, fs::path const& rivers_path, bool detailed_errors);
		bool generate_and_load_province_adjacencies(std::span<const ovdl::csv::LineObject> additional_adjacencies);
		bool load_climate_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
		bool load_continent_file(ModifierManager const& modifier_manager, ast::NodeCPtr root);
	};
}
