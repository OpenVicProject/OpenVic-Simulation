#include "Map.hpp"

#include <cassert>
#include <unordered_set>

#include "openvic-simulation/economy/Good.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

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

bool Map::add_province(const std::string_view identifier, colour_t colour) {
	if (provinces.size() >= max_provinces) {
		Logger::error("The map's province list is full - maximum number of provinces is ", max_provinces, " (this can be at most ", Province::MAX_INDEX, ")");
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid province identifier - empty!");
		return false;
	}
	if (colour == NULL_COLOUR || colour > MAX_COLOUR_RGB) {
		Logger::error("Invalid province colour for ", identifier, ": ", colour_to_hex_string(colour));
		return false;
	}
	Province new_province { identifier, colour, static_cast<Province::index_t>(provinces.size() + 1) };
	const Province::index_t index = get_index_from_colour(colour);
	if (index != Province::NULL_INDEX) {
		Logger::error("Duplicate province colours: ", get_province_by_index(index)->to_string(), " and ", new_province.to_string());
		return false;
	}
	colour_index_map[new_province.get_colour()] = new_province.get_index();
	return provinces.add_item(std::move(new_province));
}

bool Map::set_water_province(const std::string_view identifier) {
	if (water_provinces.is_locked()) {
		Logger::error("The map's water provinces have already been locked!");
		return false;
	}
	Province* province = get_province_by_identifier(identifier);
	if (province == nullptr) {
		Logger::error("Unrecognised water province identifier: ", identifier);
		return false;
	}
	if (province->get_water()) {
		Logger::warning("Province ", identifier, " is already a water province!");
		return true;
	}
	if (!water_provinces.add_province(province)) {
		Logger::error("Failed to add province ", identifier, " to water province set!");
		return false;
	}
	province->water = true;
	return true;
}

bool Map::set_water_province_list(std::vector<std::string_view> const& list) {
	bool ret = true;
	water_provinces.reserve(water_provinces.size() + list.size());
	for (std::string_view const& identifier : list) {
		ret &= set_water_province(identifier);
	}
	return ret;
}

void Map::lock_water_provinces() {
	water_provinces.lock();
	Logger::info("Locked water provinces after registering ", water_provinces.size());
}

bool Map::add_region(const std::string_view identifier, std::vector<std::string_view> const& province_identifiers) {
	if (identifier.empty()) {
		Logger::error("Invalid region identifier - empty!");
		return false;
	}
	Region::provinces_t provinces;
	provinces.reserve(province_identifiers.size());
	bool meta = false, ret = true;
	for (const std::string_view province_identifier : province_identifiers) {
		Province* province = get_province_by_identifier(province_identifier);
		if (province != nullptr) {
			if (std::find(provinces.begin(), provinces.end(), province) != provinces.end()) {
				Logger::error("Duplicate province identifier ", province_identifier, " in region ", identifier);
				ret = false;
			} else {
				if (province->get_has_region()) {
					meta = true;
				}
				provinces.push_back(province);
			}
		} else {
			Logger::error("Invalid province identifier ", province_identifier, " for region ", identifier);
			ret = false;
		}
	}
	if (provinces.empty()) {
		Logger::warning("No valid provinces in list for ", identifier);
		return ret;
	}

	if (!meta) {
		for (Province* province : provinces) {
			province->has_region = true;
		}
	}
	ret &= regions.add_item({ identifier, std::move(provinces), meta });
	return ret;
}

Province* Map::get_province_by_index(Province::index_t index) {
	return index != Province::NULL_INDEX ? provinces.get_item_by_index(index - 1) : nullptr;
}

Province const* Map::get_province_by_index(Province::index_t index) const {
	return index != Province::NULL_INDEX ? provinces.get_item_by_index(index - 1) : nullptr;
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

bool Map::set_max_provinces(Province::index_t new_max_provinces) {
	if (new_max_provinces <= Province::NULL_INDEX) {
		Logger::error("Trying to set max province count to an invalid value ", new_max_provinces, " (must be greater than ", Province::NULL_INDEX, ")");
		return false;
	}
	if (!provinces.empty() || provinces.is_locked()) {
		Logger::error("Trying to set max province count to ", new_max_provinces, " after provinces have already been added and/or locked");
		return false;
	}
	max_provinces = new_max_provinces;
	return true;
}

Province::index_t Map::get_max_provinces() const {
	return max_provinces;
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

static colour_t colour_at(uint8_t const* colour_data, int32_t idx) {
	idx *= 3;
	return (colour_data[idx] << 16) | (colour_data[idx + 1] << 8) | colour_data[idx + 2];
}

bool Map::generate_province_shape_image(size_t new_width, size_t new_height, uint8_t const* colour_data,
	uint8_t const* terrain_data, terrain_variant_map_t const& terrain_variant_map, bool detailed_errors) {
	if (!province_shape_image.empty()) {
		Logger::error("Province index image has already been generated!");
		return false;
	}
	if (!provinces.is_locked()) {
		Logger::error("Province index image cannot be generated until after provinces are locked!");
		return false;
	}
	if (new_width < 1 || new_height < 1) {
		Logger::error("Invalid province image dimensions: ", new_width, "x", new_height);
		return false;
	}
	if (colour_data == nullptr) {
		Logger::error("Province colour data pointer is null!");
		return false;
	}
	if (terrain_data == nullptr) {
		Logger::error("Province terrain data pointer is null!");
		return false;
	}
	width = new_width;
	height = new_height;
	province_shape_image.resize(width * height);

	std::vector<bool> province_checklist(provinces.size());
	bool ret = true;
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
						Logger::warning("Unrecognised terrain colour ", colour_to_hex_string(terrain_colour),
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
					Logger::warning("Unrecognised province colour ", colour_to_hex_string(province_colour),
						" at (", x, ", ", y, ")");
				}
			}
			province_shape_image[idx].index = Province::NULL_INDEX;
		}
	}
	if (!unrecognised_province_colours.empty()) {
		Logger::warning("Province image contains ", unrecognised_province_colours.size(), " unrecognised province colours");
	}
	if (!unrecognised_terrain_colours.empty()) {
		Logger::warning("Terrain image contains ", unrecognised_terrain_colours.size(), " unrecognised terrain colours");
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
		ret = false;
	}

	ret &= _generate_province_adjacencies();

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

bool Map::add_mapmode(const std::string_view identifier, Mapmode::colour_func_t colour_func) {
	if (identifier.empty()) {
		Logger::error("Invalid mapmode identifier - empty!");
		return false;
	}
	if (colour_func == nullptr) {
		Logger::error("Mapmode colour function is null for identifier: ", identifier);
		return false;
	}
	return mapmodes.add_item({ identifier, mapmodes.size(), colour_func });
}

bool Map::generate_mapmode_colours(Mapmode::index_t index, uint8_t* target) const {
	if (target == nullptr) {
		Logger::error("Mapmode colour target pointer is null!");
		return false;
	}
	bool ret = true;
	Mapmode const* mapmode = mapmodes.get_item_by_index(index);
	if (mapmode == nullptr) {
		// Not an error if mapmodes haven't yet been loaded,
		// e.g. if we want to allocate the province colour
		// texture before mapmodes are loaded.
		if (!(mapmodes.empty() && index == 0)) {
			Logger::error("Invalid mapmode index: ", index);
			ret = false;
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

bool Map::setup(GoodManager const& good_manager, BuildingManager const& building_manager, PopManager const& pop_manager) {
	bool ret = true;
	for (Province& province : provinces.get_items()) {
		province.clear_pops();
		ret &= building_manager.generate_province_buildings(province);
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

using namespace ovdl::csv;

static bool validate_province_definitions_header(LineObject const& header) {
	static const std::vector<std::string> standard_header { "province", "red", "green", "blue" };
	for (size_t i = 0; i < standard_header.size(); ++i) {
		const std::string_view val = header.get_value_for(i);
		if (i == 0 && val.empty()) break;
		if (val != standard_header[i]) return false;
	}
	return true;
}

static bool parse_province_colour(colour_t& colour, std::array<std::string_view, 3> components) {
	bool ret = true;
	colour = NULL_COLOUR;
	for (std::string_view& c : components) {
		colour <<= 8;
		if (c.ends_with('.')) c.remove_suffix(1);
		bool successful = false;
		uint64_t val = StringUtils::string_to_uint64(c, &successful, 10);
		if (successful && val <= 255) {
			colour |= val;
		} else {
			ret = false;
		}
	}
	return ret;
}

bool Map::load_province_definitions(std::vector<LineObject> const& lines) {
	if (lines.empty()) {
		Logger::error("No header or entries in province definition file!");
		return false;
	}
	{
		LineObject const& header = lines.front();
		if (!validate_province_definitions_header(header)) {
			Logger::error("Non-standard province definition file header - make sure this is not a province definition: ", header);
		}
	}
	if (lines.size() <= 1) {
		Logger::error("No entries in province definition file!");
		return false;
	}
	provinces.reserve(lines.size() - 1);
	bool ret = true;
	std::for_each(lines.begin() + 1, lines.end(),
		[this, &ret](LineObject const& line) -> void {
			const std::string_view identifier = line.get_value_for(0);
			if (!identifier.empty()) {
				colour_t colour;
				if (!parse_province_colour(colour,
					{ line.get_value_for(1), line.get_value_for(2), line.get_value_for(3) }
				)) {
					Logger::error("Error reading colour in province definition: ", line);
					ret = false;
				}
				ret &= add_province(identifier, colour);
			}
		}
	);
	lock_provinces();
	return ret;
}

bool Map::load_province_positions(BuildingManager const& building_manager, ast::NodeCPtr root) {
	return expect_province_dictionary(
		[&building_manager](Province& province, ast::NodeCPtr node) -> bool {
			return province.load_positions(building_manager, node);
		}
	)(root);
}

bool Map::load_region_file(ast::NodeCPtr root) {
	const bool ret = expect_dictionary_reserve_length(
		regions,
		[this](std::string_view region_identifier, ast::NodeCPtr region_node) -> bool {
			std::vector<std::string_view> province_identifiers;
			bool ret = expect_list_reserve_length(
				province_identifiers,
				expect_identifier([&province_identifiers](std::string_view identifier) -> bool {
					province_identifiers.push_back(identifier);
					return true;
				})
			)(region_node);
			ret &= add_region(region_identifier, province_identifiers);
			return ret;
		}
	)(root);
	lock_regions();
	for (Region& region : regions.get_items()) {
		if (!region.meta) {
			for (Province* province : region.get_provinces()) {
				if (!province->get_has_region()) {
					Logger::error("Province in non-meta region without has_region set: ", province->get_identifier());
					province->has_region = true;
				}
				province->region = &region;
			}
		}
	}
	for (Province& province : provinces.get_items()) {
		const bool region_null = province.get_region() == nullptr;
		if (province.get_has_region() == region_null) {
			Logger::error("Province has_region / region mismatch: has_region = ", province.get_has_region(), ", region = ", province.get_region());
			province.has_region = !region_null;
		}
	}
	return ret;
}

bool Map::_generate_province_adjacencies() {
	bool changed = false;

	auto generate_adjacency = [&] (shape_pixel_t cur_pixel, size_t x, size_t y) -> bool {
		size_t idx = x + y * width;
		shape_pixel_t neighbour_pixel = province_shape_image[idx];
		if (cur_pixel.index != neighbour_pixel.index) {
			Province* cur = get_province_by_index(cur_pixel.index);
			Province* neighbour = get_province_by_index(neighbour_pixel.index);
			if(cur != nullptr && neighbour != nullptr) {
				cur->add_adjacency(neighbour, 0, 0);
				neighbour->add_adjacency(cur, 0, 0);
				return true;
			} else Logger::error(
				"Couldn't find province(s): current = ", cur, " (", cur_pixel.index,
				"), neighbour = ", neighbour, " (", neighbour_pixel.index, ")"
			);
		}
		return false;
	};
	
	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			shape_pixel_t cur = province_shape_image[x + y * width];
			if(x + 1 < width)
				changed |= generate_adjacency(cur, x + 1, y);
			if(y + 1 < height)
				changed |= generate_adjacency(cur, x, y + 1);
		}
	}

	return changed;
}
