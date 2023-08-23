#include "Map.hpp"

#include <cassert>
#include <unordered_set>

#include "openvic/economy/Good.hpp"
#include "openvic/utility/Logger.hpp"

using namespace OpenVic;

Mapmode::Mapmode(const std::string_view new_identifier, index_t new_index, colour_func_t new_colour_func)
	: HasIdentifier { new_identifier },
	  index { new_index },
	  colour_func { new_colour_func } {
	assert(colour_func != nullptr);
}

const Mapmode Mapmode::ERROR_MAPMODE { "mapmode_error", 0,
	[](Map const& map, Province const& province) -> colour_t { return 0xFFFF0000; } };

Mapmode::index_t Mapmode::get_index() const {
	return index;
}

colour_t Mapmode::get_colour(Map const& map, Province const& province) const {
	return colour_func ? colour_func(map, province) : NULL_COLOUR;
}

Map::Map() : provinces { "provinces" },
			 regions { "regions" },
			 mapmodes { "mapmodes" } {}

return_t Map::add_province(const std::string_view identifier, colour_t colour) {
	if (provinces.get_item_count() >= Province::MAX_INDEX) {
		Logger::error("The map's province list is full - there can be at most ", Province::MAX_INDEX, " provinces");
		return FAILURE;
	}
	if (identifier.empty()) {
		Logger::error("Invalid province identifier - empty!");
		return FAILURE;
	}
	if (colour == NULL_COLOUR || colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid province colour for ", identifier, ": ", Province::colour_to_hex_string(colour));
		return FAILURE;
	}
	Province new_province { identifier, colour, static_cast<Province::index_t>(provinces.get_item_count() + 1) };
	const Province::index_t index = get_index_from_colour(colour);
	if (index != Province::NULL_INDEX) {
		Logger::error("Duplicate province colours: ", get_province_by_index(index)->to_string(), " and ", new_province.to_string());
		return FAILURE;
	}
	colour_index_map[new_province.get_colour()] = new_province.get_index();
	return provinces.add_item(std::move(new_province));
}

void Map::lock_provinces() {
	provinces.lock();
}

return_t Map::set_water_province(const std::string_view identifier) {
	if (water_provinces.is_locked()) {
		Logger::error("The map's water provinces have already been locked!");
		return FAILURE;
	}
	Province* province = get_province_by_identifier(identifier);
	if (province == nullptr) {
		Logger::error("Unrecognised water province identifier: ", identifier);
		return FAILURE;
	}
	if (province->is_water()) {
		Logger::error("Province ", identifier, " is already a water province!");
		return FAILURE;
	}
	if (water_provinces.add_province(province) != SUCCESS) {
		Logger::error("Failed to add province ", identifier, " to water province set!");
		return FAILURE;
	}
	province->water = true;
	return SUCCESS;
}

void Map::lock_water_provinces() {
	water_provinces.lock();
	Logger::info("Locked water provinces after registering ", water_provinces.get_province_count());
}

return_t Map::add_region(const std::string_view identifier, std::vector<std::string_view> const& province_identifiers) {
	if (identifier.empty()) {
		Logger::error("Invalid region identifier - empty!");
		return FAILURE;
	}
	Region new_region { identifier };
	return_t ret = SUCCESS;
	for (const std::string_view province_identifier : province_identifiers) {
		Province* province = get_province_by_identifier(province_identifier);
		if (province != nullptr) {
			if (new_region.contains_province(province)) {
				Logger::error("Duplicate province identifier ", province_identifier, " in region ", identifier);
				ret = FAILURE;
			} else {
				size_t other_region_index = reinterpret_cast<size_t>(province->get_region());
				if (other_region_index != 0) {
					other_region_index--;
					if (other_region_index < regions.get_item_count())
						Logger::error("Cannot add province ", province_identifier, " to region ", identifier, " - it is already part of ", regions.get_item_by_index(other_region_index)->get_identifier());
					else
						Logger::error("Cannot add province ", province_identifier, " to region ", identifier, " - it is already part of an unknown region with index ", other_region_index);
					ret = FAILURE;
				} else if (new_region.add_province(province) != SUCCESS) {
					Logger::error("Failed to add province ", province_identifier, " to region ", identifier);
					ret = FAILURE;
				}
			}
		} else {
			Logger::error("Invalid province identifier ", province_identifier, " for region ", identifier);
			ret = FAILURE;
		}
	}
	new_region.lock();
	if (!new_region.get_province_count()) {
		Logger::error("No valid provinces in list for ", identifier);
		return FAILURE;
	}

	// Used to detect provinces listed in multiple regions, will
	// be corrected once regions is stable (i.e. lock_regions).
	Region* tmp_region_index = reinterpret_cast<Region*>(regions.get_item_count());
	for (Province* province : new_region.get_provinces())
		province->region = tmp_region_index;
	if (regions.add_item(std::move(new_region)) != SUCCESS) ret = FAILURE;
	return ret;
}

void Map::lock_regions() {
	regions.lock();
	for (Region& region : regions.get_items())
		for (Province* province : region.get_provinces())
			province->region = &region;
}

size_t Map::get_province_count() const {
	return provinces.get_item_count();
}

Province* Map::get_province_by_index(Province::index_t index) {
	return index != Province::NULL_INDEX ? provinces.get_item_by_index(index - 1) : nullptr;
}

Province const* Map::get_province_by_index(Province::index_t index) const {
	return index != Province::NULL_INDEX ? provinces.get_item_by_index(index - 1) : nullptr;
}

Province* Map::get_province_by_identifier(const std::string_view identifier) {
	return provinces.get_item_by_identifier(identifier);
}

Province const* Map::get_province_by_identifier(const std::string_view identifier) const {
	return provinces.get_item_by_identifier(identifier);
}

Province::index_t Map::get_index_from_colour(colour_t colour) const {
	const colour_index_map_t::const_iterator it = colour_index_map.find(colour);
	if (it != colour_index_map.end()) return it->second;
	return Province::NULL_INDEX;
}

Province::index_t Map::get_province_index_at(size_t x, size_t y) const {
	if (x < width && y < height) return province_shape_image[x + y * width].index;
	return Province::NULL_INDEX;
}

void Map::set_selected_province(Province::index_t index) {
	if (index > get_province_count()) {
		Logger::error("Trying to set selected province to an invalid index ", index, " (max index is ", get_province_count(), ")");
		selected_province = Province::NULL_INDEX;
	} else {
		selected_province = index;
	}
}

Province::index_t Map::get_selected_province_index() const {
	return selected_province;
}

Province const* Map::get_selected_province() const {
	return get_province_by_index(get_selected_province_index());
}

Region* Map::get_region_by_identifier(const std::string_view identifier) {
	return regions.get_item_by_identifier(identifier);
}

Region const* Map::get_region_by_identifier(const std::string_view identifier) const {
	return regions.get_item_by_identifier(identifier);
}

static colour_t colour_at(uint8_t const* colour_data, int32_t idx) {
	idx *= 3;
	return (colour_data[idx] << 16) | (colour_data[idx + 1] << 8) | colour_data[idx + 2];
}

return_t Map::generate_province_shape_image(size_t new_width, size_t new_height, uint8_t const* colour_data,
	uint8_t const* terrain_data, terrain_variant_map_t const& terrain_variant_map, bool detailed_errors) {
	if (!province_shape_image.empty()) {
		Logger::error("Province index image has already been generated!");
		return FAILURE;
	}
	if (!provinces.is_locked()) {
		Logger::error("Province index image cannot be generated until after provinces are locked!");
		return FAILURE;
	}
	if (new_width < 1 || new_height < 1) {
		Logger::error("Invalid province image dimensions: ", new_width, "x", new_height);
		return FAILURE;
	}
	if (colour_data == nullptr) {
		Logger::error("Province colour data pointer is null!");
		return FAILURE;
	}
	if (terrain_data == nullptr) {
		Logger::error("Province terrain data pointer is null!");
		return FAILURE;
	}
	width = new_width;
	height = new_height;
	province_shape_image.resize(width * height);

	std::vector<bool> province_checklist(provinces.get_item_count());
	return_t ret = SUCCESS;
	std::unordered_set<colour_t> unrecognised_province_colours, unrecognised_terrain_colours;

	for (int32_t y = 0; y < height; ++y) {
		for (int32_t x = 0; x < width; ++x) {
			const int32_t idx = x + y * width;

			const colour_t terrain_colour = colour_at(terrain_data, idx);
			const terrain_variant_map_t::const_iterator it = terrain_variant_map.find(terrain_colour);
			if (it != terrain_variant_map.end()) province_shape_image[idx].terrain = it->second;
			else {
				if (unrecognised_terrain_colours.find(terrain_colour) == unrecognised_terrain_colours.end()) {
					unrecognised_terrain_colours.insert(terrain_colour);
					if (detailed_errors) {
						Logger::error("Unrecognised terrain colour ", Province::colour_to_hex_string(terrain_colour),
							" at (", x, ", ", y, ")");
					}
				}
				province_shape_image[idx].terrain = 0;
			}

			const colour_t province_colour = colour_at(colour_data, idx);
			if (x > 0) {
				const int32_t jdx = idx - 1;
				if (colour_at(colour_data, jdx) == province_colour) {
					province_shape_image[idx].index = province_shape_image[jdx].index;
					continue;
				}
			}
			if (y > 0) {
				const int32_t jdx = idx - width;
				if (colour_at(colour_data, jdx) == province_colour) {
					province_shape_image[idx].index = province_shape_image[jdx].index;
					continue;
				}
			}
			const Province::index_t index = get_index_from_colour(province_colour);
			if (index != Province::NULL_INDEX) {
				province_checklist[index - 1] = true;
				province_shape_image[idx].index = index;
				continue;
			}
			if (unrecognised_province_colours.find(province_colour) == unrecognised_province_colours.end()) {
				unrecognised_province_colours.insert(province_colour);
				if (detailed_errors) {
					Logger::error("Unrecognised province colour ", Province::colour_to_hex_string(province_colour),
						" at (", x, ", ", y, ")");
				}
			}
			province_shape_image[idx].index = Province::NULL_INDEX;
		}
	}
	if (!unrecognised_province_colours.empty()) {
		Logger::error("Province image contains ", unrecognised_province_colours.size(), " unrecognised province colours");
		ret = FAILURE;
	}
	if (!unrecognised_terrain_colours.empty()) {
		Logger::error("Terrain image contains ", unrecognised_terrain_colours.size(), " unrecognised terrain colours");
		ret = FAILURE;
	}

	size_t missing = 0;
	for (size_t idx = 0; idx < province_checklist.size(); ++idx) {
		if (!province_checklist[idx]) {
			if (detailed_errors) {
				Logger::error("Province missing from shape image: ", provinces.get_item_by_index(idx)->to_string());
			}
			missing++;
		}
	}
	if (missing > 0) {
		Logger::error("Province image is missing ", missing, " province colours");
		ret = FAILURE;
	}
	return ret;
}

size_t Map::get_width() const {
	return width;
}

size_t Map::get_height() const {
	return height;
}

std::vector<Map::shape_pixel_t> const& Map::get_province_shape_image() const {
	return province_shape_image;
}

return_t Map::add_mapmode(const std::string_view identifier, Mapmode::colour_func_t colour_func) {
	if (identifier.empty()) {
		Logger::error("Invalid mapmode identifier - empty!");
		return FAILURE;
	}
	if (colour_func == nullptr) {
		Logger::error("Mapmode colour function is null for identifier: ", identifier);
		return FAILURE;
	}
	return mapmodes.add_item({ identifier, mapmodes.get_item_count(), colour_func });
}

void Map::lock_mapmodes() {
	mapmodes.lock();
}

size_t Map::get_mapmode_count() const {
	return mapmodes.get_item_count();
}

Mapmode const* Map::get_mapmode_by_index(size_t index) const {
	return mapmodes.get_item_by_index(index);
}

Mapmode const* Map::get_mapmode_by_identifier(const std::string_view identifier) const {
	return mapmodes.get_item_by_identifier(identifier);
}

return_t Map::generate_mapmode_colours(Mapmode::index_t index, uint8_t* target) const {
	if (target == nullptr) {
		Logger::error("Mapmode colour target pointer is null!");
		return FAILURE;
	}
	return_t ret = SUCCESS;
	Mapmode const* mapmode = mapmodes.get_item_by_index(index);
	if (mapmode == nullptr) {
		// Not an error if mapmodes haven't yet been loaded,
		// e.g. if we want to allocate the province colour
		// texture before mapmodes are loaded.
		if (!(mapmodes.get_item_count() == 0 && index == 0)) {
			Logger::error("Invalid mapmode index: ", index);
			ret = FAILURE;
		}
		mapmode = &Mapmode::ERROR_MAPMODE;
	}
	// Skip past Province::NULL_INDEX
	for (size_t i = 0; i < MAPMODE_COLOUR_SIZE; ++i)
		*target++ = 0;
	for (Province const& province : provinces.get_items()) {
		const colour_t colour = mapmode->get_colour(*this, province);
		*target++ = (colour >> 16) & FULL_COLOUR;
		*target++ = (colour >> 8) & FULL_COLOUR;
		*target++ = colour & FULL_COLOUR;
		*target++ = (colour >> 24) & FULL_COLOUR;
	}
	return ret;
}

void Map::update_highest_province_population() {
	highest_province_population = 0;
	for (Province const& province : provinces.get_items()) {
		highest_province_population = std::max(highest_province_population, province.get_total_population());
	}
}

Pop::pop_size_t Map::get_highest_province_population() const {
	return highest_province_population;
}

void Map::update_total_map_population() {
	total_map_population = 0;
	for (Province const& province : provinces.get_items()) {
		total_map_population += province.get_total_population();
	}
}

Pop::pop_size_t Map::get_total_map_population() const {
	return total_map_population;
}

return_t Map::setup(GoodManager const& good_manager, BuildingManager const& building_manager, PopManager const& pop_manager) {
	return_t ret = SUCCESS;
	for (Province& province : provinces.get_items()) {
		province.clear_pops();
		// Set all land provinces to have an RGO based on their index to test them
		if (!province.is_water() && good_manager.get_good_count() > 0)
			province.rgo = good_manager.get_good_by_index(province.get_index() % good_manager.get_good_count());
		if (building_manager.generate_province_buildings(province) != SUCCESS) ret = FAILURE;
	}
	return ret;
}

void Map::update_state(Date const& today) {
	for (Province& province : provinces.get_items())
		province.update_state(today);
	update_highest_province_population();
	update_total_map_population();
}

void Map::tick(Date const& today) {
	for (Province& province : provinces.get_items())
		province.tick(today);
}
