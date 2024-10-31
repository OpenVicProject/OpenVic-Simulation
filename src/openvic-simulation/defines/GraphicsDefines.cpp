#include "GraphicsDefines.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

GraphicsDefines::GraphicsDefines()
  : cities_sprawl_offset {},
	cities_sprawl_width {},
	cities_sprawl_height {},
	cities_sprawl_iterations {},
	cities_mesh_pool_size_for_country {},
	cities_mesh_pool_size_for_culture {},
	cities_mesh_pool_size_for_generic {},
	cities_mesh_types_count {},
	cities_mesh_sizes_count {},
	cities_special_buildings_pool_size {},
	cities_size_max_population_k {} {}

std::string_view GraphicsDefines::get_name() const {
	return "graphics";
}

node_callback_t GraphicsDefines::expect_defines() {
	return expect_dictionary_keys(
		"CITIES_SPRAWL_OFFSET", ONE_EXACTLY, expect_uint(assign_variable_callback(cities_sprawl_offset)),
		"CITIES_SPRAWL_WIDTH", ONE_EXACTLY, expect_uint(assign_variable_callback(cities_sprawl_width)),
		"CITIES_SPRAWL_HEIGHT", ONE_EXACTLY, expect_uint(assign_variable_callback(cities_sprawl_height)),
		"CITIES_SPRAWL_ITERATIONS", ONE_EXACTLY, expect_uint(assign_variable_callback(cities_sprawl_iterations)),
		"CITIES_MESH_POOL_SIZE_FOR_COUNTRY", ONE_EXACTLY,
			expect_uint(assign_variable_callback(cities_mesh_pool_size_for_country)),
		"CITIES_MESH_POOL_SIZE_FOR_CULTURE", ONE_EXACTLY,
			expect_uint(assign_variable_callback(cities_mesh_pool_size_for_culture)),
		"CITIES_MESH_POOL_SIZE_FOR_GENERIC", ONE_EXACTLY,
			expect_uint(assign_variable_callback(cities_mesh_pool_size_for_generic)),
		"CITIES_MESH_TYPES_COUNT", ONE_EXACTLY, expect_uint(assign_variable_callback(cities_mesh_types_count)),
		"CITIES_MESH_SIZES_COUNT", ONE_EXACTLY, expect_uint(assign_variable_callback(cities_mesh_sizes_count)),
		"CITIES_SPECIAL_BUILDINGS_POOL_SIZE", ONE_EXACTLY,
			expect_uint(assign_variable_callback(cities_special_buildings_pool_size)),
		"CITIES_SIZE_MAX_POPULATION_K", ONE_EXACTLY, expect_uint(assign_variable_callback(cities_size_max_population_k))
	);
}
