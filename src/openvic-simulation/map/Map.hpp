#pragma once

#include <filesystem>
#include <functional>

#include <openvic-dataloader/csv/LineObject.hpp>

#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/map/TerrainType.hpp"

namespace OpenVic {
	namespace fs = std::filesystem;

	struct Mapmode : HasIdentifier {
		friend struct Map;

		using colour_func_t = std::function<colour_t(Map const&, Province const&)>;
		using index_t = size_t;

	private:
		const index_t index;
		const colour_func_t colour_func;

		Mapmode(std::string_view new_identifier, index_t new_index, colour_func_t new_colour_func);

	public:
		static const Mapmode ERROR_MAPMODE;

		Mapmode(Mapmode&&) = default;

		index_t get_index() const;
		colour_t get_colour(Map const& map, Province const& province) const;
	};

	struct GoodManager;

	/* REQUIREMENTS:
	 * MAP-4
	 */
	struct Map {

#pragma pack(push, 1)
		/* Used to represent tightly packed 3-byte integer pixel information. */
		struct shape_pixel_t {
			Province::index_t index;
			TerrainTypeMapping::index_t terrain;
		};
#pragma pack(pop)
	private:
		using colour_index_map_t = std::map<colour_t, Province::index_t>;

		IdentifierRegistry<Province> provinces;
		IdentifierRegistry<Region> regions;
		IdentifierRegistry<Mapmode> mapmodes;
		ProvinceSet water_provinces;
		TerrainTypeManager terrain_type_manager;

		size_t width = 0, height = 0;
		std::vector<shape_pixel_t> province_shape_image;
		colour_index_map_t colour_index_map;

		Province::index_t max_provinces = Province::MAX_INDEX;
		Province::index_t selected_province = Province::NULL_INDEX;
		Pop::pop_size_t highest_province_population, total_map_population;

		Province::index_t get_index_from_colour(colour_t colour) const;
		bool _generate_province_adjacencies();

	public:
		Map();

		bool add_province(std::string_view identifier, colour_t colour);
		IDENTIFIER_REGISTRY_ACCESSORS(province)
		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(province)

		bool set_water_province(std::string_view identifier);
		bool set_water_province_list(std::vector<std::string_view> const& list);
		void lock_water_provinces();

		Province* get_province_by_index(Province::index_t index);
		Province const* get_province_by_index(Province::index_t index) const;
		Province::index_t get_province_index_at(size_t x, size_t y) const;
		bool set_max_provinces(Province::index_t new_max_provinces);
		Province::index_t get_max_provinces() const;
		void set_selected_province(Province::index_t index);
		Province::index_t get_selected_province_index() const;
		Province const* get_selected_province() const;

		size_t get_width() const;
		size_t get_height() const;
		std::vector<shape_pixel_t> const& get_province_shape_image() const;
		REF_GETTERS(terrain_type_manager)

		bool add_region(std::string_view identifier, std::vector<std::string_view> const& province_identifiers);
		IDENTIFIER_REGISTRY_ACCESSORS(region)
		IDENTIFIER_REGISTRY_NON_CONST_ACCESSORS(region)

		bool add_mapmode(std::string_view identifier, Mapmode::colour_func_t colour_func);
		IDENTIFIER_REGISTRY_ACCESSORS(mapmode)
		Mapmode const* get_mapmode_by_index(size_t index) const;
		static constexpr size_t MAPMODE_COLOUR_SIZE = 4;
		bool generate_mapmode_colours(Mapmode::index_t index, uint8_t* target) const;

		bool setup(BuildingManager const& building_manager, PopManager const& pop_manager);

		void update_highest_province_population();
		Pop::pop_size_t get_highest_province_population() const;
		void update_total_map_population();
		Pop::pop_size_t get_total_map_population() const;

		void update_state(Date const& today);
		void tick(Date const& today);

		bool load_province_definitions(std::vector<ovdl::csv::LineObject> const& lines);
		bool load_province_positions(BuildingManager const& building_manager, ast::NodeCPtr root);
		bool load_region_file(ast::NodeCPtr root);
		bool load_map_images(fs::path const& province_path, fs::path const& terrain_path, bool detailed_errors);
		bool generate_and_load_province_adjacencies(std::vector<ovdl::csv::LineObject> const& additional_adjacencies);
	};
}
