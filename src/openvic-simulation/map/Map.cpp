#include "Map.hpp"

#include <cstddef>
#include <vector>

#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/BMP.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Map::Map()
  : dims { 0, 0 }, max_provinces { ProvinceDefinition::MAX_INDEX }, selected_province { nullptr },
	highest_province_population { 0 }, total_map_population { 0 } {}

bool Map::add_province_definition(std::string_view identifier, colour_t colour) {
	if (province_definitions.size() >= max_provinces) {
		Logger::error(
			"The map's province list is full - maximum number of provinces is ", max_provinces, " (this can be at most ",
			ProvinceDefinition::MAX_INDEX, ")"
		);
		return false;
	}
	if (identifier.empty()) {
		Logger::error("Invalid province identifier - empty!");
		return false;
	}
	if (!valid_basic_identifier(identifier)) {
		Logger::error(
			"Invalid province identifier: ", identifier, " (can only contain alphanumeric characters and underscores)"
		);
		return false;
	}
	if (colour.is_null()) {
		Logger::error("Invalid province colour for ", identifier, " - null! (", colour, ")");
		return false;
	}
	ProvinceDefinition new_province {
		identifier, colour, static_cast<ProvinceDefinition::index_t>(province_definitions.size() + 1)
	};
	const ProvinceDefinition::index_t index = get_index_from_colour(colour);
	if (index != ProvinceDefinition::NULL_INDEX) {
		Logger::error(
			"Duplicate province colours: ", get_province_definition_by_index(index)->to_string(), " and ",
			new_province.to_string()
		);
		return false;
	}
	colour_index_map[new_province.get_colour()] = new_province.get_index();
	return province_definitions.add_item(std::move(new_province));
}

ProvinceInstance* Map::get_province_instance_from_const(ProvinceDefinition const* province) {
	if (province != nullptr) {
		return get_province_instance_by_index(province->get_index());
	} else {
		return nullptr;
	}
}

ProvinceInstance const* Map::get_province_instance_from_const(ProvinceDefinition const* province) const {
	if (province != nullptr) {
		return get_province_instance_by_index(province->get_index());
	} else {
		return nullptr;
	}
}

ProvinceDefinition::distance_t Map::calculate_distance_between(
	ProvinceDefinition const& from, ProvinceDefinition const& to
) const {
	const fvec2_t to_pos = to.get_unit_position();
	const fvec2_t from_pos = from.get_unit_position();

	const fixed_point_t min_x = std::min(
		(to_pos.x - from_pos.x).abs(),
		std::min(
			(to_pos.x - from_pos.x + get_width()).abs(),
			(to_pos.x - from_pos.x - get_width()).abs()
		)
	);

	return fvec2_t { min_x, to_pos.y - from_pos.y }.length_squared().sqrt();
}

using adjacency_t = ProvinceDefinition::adjacency_t;

/* This is called for all adjacent pixel pairs and returns whether or not a new adjacency was add,
 * hence the lack of error messages in the false return cases. */
bool Map::add_standard_adjacency(ProvinceDefinition& from, ProvinceDefinition& to) const {
	if (from == to) {
		return false;
	}

	const bool from_needs_adjacency = !from.is_adjacent_to(&to);
	const bool to_needs_adjacency = !to.is_adjacent_to(&from);

	if (!from_needs_adjacency && !to_needs_adjacency) {
		return false;
	}

	const ProvinceDefinition::distance_t distance = calculate_distance_between(from, to);

	using enum adjacency_t::type_t;

	/* Default land-to-land adjacency */
	adjacency_t::type_t type = LAND;
	if (from.is_water() != to.is_water()) {
		/* Land-to-water adjacency */
		type = COASTAL;

		/* Mark the land province as coastal */
		from.coastal = !from.is_water();
		to.coastal = !to.is_water();
	} else if (from.is_water()) {
		/* Water-to-water adjacency */
		type = WATER;
	}

	if (from_needs_adjacency) {
		from.adjacencies.emplace_back(&to, distance, type, nullptr, 0);
	}
	if (to_needs_adjacency) {
		to.adjacencies.emplace_back(&from, distance, type, nullptr, 0);
	}
	return true;
}

bool Map::add_special_adjacency(
	ProvinceDefinition& from, ProvinceDefinition& to, adjacency_t::type_t type, ProvinceDefinition const* through,
	adjacency_t::data_t data
) const {
	if (from == to) {
		Logger::error("Trying to add ", adjacency_t::get_type_name(type), " adjacency from province ", from, " to itself!");
		return false;
	}

	using enum adjacency_t::type_t;

	/* Check end points */
	switch (type) {
	case LAND:
	case STRAIT:
		if (from.is_water() || to.is_water()) {
			Logger::error(adjacency_t::get_type_name(type), " adjacency from ", from, " to ", to, " has water endpoint(s)!");
			return false;
		}
		break;
	case WATER:
	case CANAL:
		if (!from.is_water() || !to.is_water()) {
			Logger::error(adjacency_t::get_type_name(type), " adjacency from ", from, " to ", to, " has land endpoint(s)!");
			return false;
		}
		break;
	case COASTAL:
		if (from.is_water() == to.is_water()) {
			Logger::error("Coastal adjacency from ", from, " to ", to, " has both land or water endpoints!");
			return false;
		}
		break;
	case IMPASSABLE:
		/* Impassable is valid for all combinations of land and water:
		 * - land-land = replace existing land adjacency with impassable adjacency (blue borders)
		 * - land-water = delete existing coastal adjacency, preventing armies and navies from moving between the provinces
		 * - water-water = delete existing water adjacency, preventing navies from moving between the provinces */
		break;
	default:
		Logger::error("Invalid adjacency type ", static_cast<uint32_t>(type));
		return false;
	}

	/* Check through province */
	if (type == STRAIT || type == CANAL) {
		const bool water_expected = type == STRAIT;
		if (through == nullptr || through->is_water() != water_expected) {
			Logger::error(
				adjacency_t::get_type_name(type), " adjacency from ", from, " to ", to, " has a ",
				(through == nullptr ? "null" : water_expected ? "land" : "water"), " through province ", through
			);
			return false;
		}
	} else if (through != nullptr) {
		Logger::warning(
			adjacency_t::get_type_name(type), " adjacency from ", from, " to ", to, " has a non-null through province ",
			through
		);
		through = nullptr;
	}

	/* Check canal data */
	if (data != adjacency_t::NO_CANAL && type != CANAL) {
		Logger::warning(
			adjacency_t::get_type_name(type), " adjacency from ", from, " to ", to, " has invalid data ",
			static_cast<uint32_t>(data)
		);
		data = adjacency_t::NO_CANAL;
	}

	const ProvinceDefinition::distance_t distance = calculate_distance_between(from, to);

	const auto add_adjacency = [distance, type, through, data](
		ProvinceDefinition& from, ProvinceDefinition const& to
	) -> bool {
		const std::vector<adjacency_t>::iterator existing_adjacency = std::find_if(
			from.adjacencies.begin(), from.adjacencies.end(),
			[&to](adjacency_t const& adj) -> bool { return adj.get_to() == &to; }
		);
		if (existing_adjacency != from.adjacencies.end()) {
			if (type == existing_adjacency->get_type()) {
				Logger::warning(
					"Adjacency from ", from, " to ", to, " already has type ", adjacency_t::get_type_name(type), "!"
				);
				if (type != STRAIT && type != CANAL) {
					/* Straits and canals might change through or data, otherwise we can exit early */
					return true;
				}
			}
			if (type == IMPASSABLE) {
				if (existing_adjacency->get_type() == WATER || existing_adjacency->get_type() == COASTAL) {
					from.adjacencies.erase(existing_adjacency);
					return true;
				}
			} else {
				if (type != STRAIT && type != CANAL) {
					Logger::error(
						"Provinces ", from, " and ", to, " already have an existing ",
						adjacency_t::get_type_name(existing_adjacency->get_type()), " adjacency, cannot create a ",
						adjacency_t::get_type_name(type), " adjacency!"
					);
					return false;
				}
				if (type != existing_adjacency->get_type() && existing_adjacency->get_type() != (type == CANAL ? WATER : LAND)) {
					Logger::error(
						"Cannot convert ", adjacency_t::get_type_name(existing_adjacency->get_type()), " adjacency from ", from,
						" to ", to, " to type ", adjacency_t::get_type_name(type), "!"
					);
					return false;
				}
			}
			*existing_adjacency = { &to, distance, type, through, data };
			return true;
		} else if (type == IMPASSABLE) {
			Logger::warning(
				"Provinces ", from, " and ", to, " do not have an existing adjacency to make impassable!"
			);
			return true;
		} else {
			from.adjacencies.emplace_back(&to, distance, type, through, data);
			return true;
		}
	};

	return add_adjacency(from, to) & add_adjacency(to, from);
}

bool Map::set_water_province(std::string_view identifier) {
	if (water_provinces.is_locked()) {
		Logger::error("The map's water provinces have already been locked!");
		return false;
	}

	ProvinceDefinition* province = get_province_definition_by_identifier(identifier);

	if (province == nullptr) {
		Logger::error("Unrecognised water province identifier: ", identifier);
		return false;
	}
	if (province->has_region()) {
		Logger::error("Province ", identifier, " cannot be water as it belongs to region \"", province->get_region(), "\"");
		return false;
	}
	if (province->is_water()) {
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
	if (water_provinces.is_locked()) {
		Logger::error("The map's water provinces have already been locked!");
		return false;
	}
	bool ret = true;
	water_provinces.reserve_more(list.size());
	for (std::string_view const& identifier : list) {
		ret &= set_water_province(identifier);
	}
	lock_water_provinces();
	return ret;
}

void Map::lock_water_provinces() {
	water_provinces.lock();
	Logger::info("Locked water provinces after registering ", water_provinces.size());
}

bool Map::add_region(std::string_view identifier, std::vector<ProvinceDefinition const*>&& provinces, colour_t colour) {
	if (identifier.empty()) {
		Logger::error("Invalid region identifier - empty!");
		return false;
	}

	bool ret = true;

	std::erase_if(provinces, [identifier, &ret](ProvinceDefinition const* province) -> bool {
		if (province->is_water()) {
			Logger::error(
				"Province ", province->get_identifier(), " cannot be added to region \"", identifier,
				"\" as it is a water province!"
			);
			ret = false;
			return true;
		} else {
			return false;
		}
	});

	const bool meta = provinces.empty() || std::any_of(
		provinces.begin(), provinces.end(), std::bind_front(&ProvinceDefinition::has_region)
	);

	Region region { identifier, colour, meta };
	ret &= region.add_provinces(provinces);
	region.lock();
	if (regions.add_item(std::move(region))) {
		if (!meta) {
			Region const& last_region = regions.get_items().back();
			for (ProvinceDefinition const* province : last_region.get_provinces()) {
				remove_province_definition_const(province)->region = &last_region;
			}
		}
	} else {
		ret = false;
	}
	return ret;
}

ProvinceDefinition::index_t Map::get_index_from_colour(colour_t colour) const {
	const colour_index_map_t::const_iterator it = colour_index_map.find(colour);
	if (it != colour_index_map.end()) {
		return it->second;
	}
	return ProvinceDefinition::NULL_INDEX;
}

ProvinceDefinition::index_t Map::get_province_index_at(ivec2_t pos) const {
	if (pos.nonnegative() && pos.less_than(dims)) {
		return province_shape_image[get_pixel_index_from_pos(pos)].index;
	}
	return ProvinceDefinition::NULL_INDEX;
}

ProvinceDefinition* Map::get_province_definition_at(ivec2_t pos) {
	return get_province_definition_by_index(get_province_index_at(pos));
}

ProvinceDefinition const* Map::get_province_definition_at(ivec2_t pos) const {
	return get_province_definition_by_index(get_province_index_at(pos));
}

bool Map::set_max_provinces(ProvinceDefinition::index_t new_max_provinces) {
	if (new_max_provinces <= ProvinceDefinition::NULL_INDEX) {
		Logger::error(
			"Trying to set max province count to an invalid value ", new_max_provinces, " (must be greater than ",
			ProvinceDefinition::NULL_INDEX, ")"
		);
		return false;
	}
	if (!province_definitions.empty() || province_definitions.is_locked()) {
		Logger::error(
			"Trying to set max province count to ", new_max_provinces, " after provinces have already been added and/or locked"
		);
		return false;
	}
	max_provinces = new_max_provinces;
	return true;
}

void Map::set_selected_province(ProvinceDefinition::index_t index) {
	if (index == ProvinceDefinition::NULL_INDEX) {
		selected_province = nullptr;
	} else {
		selected_province = get_province_instance_by_index(index);
		if (selected_province == nullptr) {
			Logger::error(
				"Trying to set selected province to an invalid index ", index, " (max index is ",
				get_province_instance_count(), ")"
			);
		}
	}
}

ProvinceInstance* Map::get_selected_province() {
	return selected_province;
}

ProvinceDefinition::index_t Map::get_selected_province_index() const {
	return selected_province != nullptr ? selected_province->get_province_definition().get_index()
		: ProvinceDefinition::NULL_INDEX;
}

bool Map::reset(BuildingTypeManager const& building_type_manager) {
	if (!province_definitions_are_locked()) {
		Logger::error("Cannot reset map - province consts are not locked!");
		return false;
	}

	bool ret = true;

	// TODO - ensure all references to old ProvinceInstances are safely cleared
	state_manager.reset();
	selected_province = nullptr;

	province_instances.reset();

	province_instances.reserve(province_definitions.size());

	for (ProvinceDefinition const& province : province_definitions.get_items()) {
		ret &= province_instances.add_item({ province });
	}

	province_instances.lock();

	for (ProvinceInstance& province : province_instances.get_items()) {
		ret &= province.reset(building_type_manager);
	}

	if (get_province_instance_count() != get_province_definition_count()) {
		Logger::error(
			"ProvinceInstance count (", get_province_instance_count(), ") does not match ProvinceDefinition count (",
			get_province_definition_count(), ")!"
		);
		return false;
	}

	return ret;
}

bool Map::apply_history_to_provinces(
	ProvinceHistoryManager const& history_manager, Date date, IdeologyManager const& ideology_manager,
	IssueManager const& issue_manager, Country const& country
) {
	bool ret = true;

	for (ProvinceInstance& province : province_instances.get_items()) {
		ProvinceDefinition const& province_definition = province.get_province_definition();
		if (!province_definition.is_water()) {
			ProvinceHistoryMap const* history_map = history_manager.get_province_history(&province_definition);

			if (history_map != nullptr) {
				ProvinceHistoryEntry const* pop_history_entry = nullptr;

				for (ProvinceHistoryEntry const* entry : history_map->get_entries_up_to(date)) {
					province.apply_history_to_province(entry);

					if (!entry->get_pops().empty()) {
						pop_history_entry = entry;
					}
				}

				if (pop_history_entry != nullptr) {
					province.add_pop_vec(pop_history_entry->get_pops());

					province.setup_pop_test_values(ideology_manager, issue_manager, country);
				}
			}
		}
	}

	return ret;
}

void Map::update_gamestate(Date today) {
	for (ProvinceInstance& province : province_instances.get_items()) {
		province.update_gamestate(today);
	}
	state_manager.update_gamestate();

	// Update population stats
	highest_province_population = 0;
	total_map_population = 0;

	for (ProvinceInstance const& province : province_instances.get_items()) {
		const Pop::pop_size_t province_population = province.get_total_population();

		if (highest_province_population < province_population) {
			highest_province_population = province_population;
		}

		total_map_population += province_population;
	}
}

void Map::tick(Date today) {
	for (ProvinceInstance& province : province_instances.get_items()) {
		province.tick(today);
	}
}

using namespace ovdl::csv;

static bool _validate_province_definitions_header(LineObject const& header) {
	static const std::vector<std::string> standard_header { "province", "red", "green", "blue" };
	for (size_t i = 0; i < standard_header.size(); ++i) {
		const std::string_view val = header.get_value_for(i);
		if (i == 0 && val.empty()) {
			break;
		}
		if (val != standard_header[i]) {
			return false;
		}
	}
	return true;
}

static bool _parse_province_colour(colour_t& colour, std::array<std::string_view, 3> components) {
	bool ret = true;
	for (size_t i = 0; i < 3; ++i) {
		std::string_view& component = components[i];
		if (component.ends_with('.')) {
			component.remove_suffix(1);
		}
		bool successful = false;
		const uint64_t val = StringUtils::string_to_uint64(component, &successful, 10);
		if (successful && val <= colour_t::max_value) {
			colour[i] = val;
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
		if (!_validate_province_definitions_header(header)) {
			Logger::error(
				"Non-standard province definition file header - make sure this is not a province definition: ", header
			);
		}
	}

	if (lines.size() <= 1) {
		Logger::error("No entries in province definition file!");
		return false;
	}

	reserve_more_province_definitions(lines.size() - 1);

	bool ret = true;
	std::for_each(lines.begin() + 1, lines.end(), [this, &ret](LineObject const& line) -> void {
		const std::string_view identifier = line.get_value_for(0);
		if (!identifier.empty()) {
			colour_t colour = colour_t::null();
			if (!_parse_province_colour(colour, { line.get_value_for(1), line.get_value_for(2), line.get_value_for(3) })) {
				Logger::error("Error reading colour in province definition: ", line);
				ret = false;
			}
			ret &= add_province_definition(identifier, colour);
		}
	});

	lock_province_definitions();

	return ret;
}

bool Map::load_province_positions(BuildingTypeManager const& building_type_manager, ast::NodeCPtr root) {
	return expect_province_definition_dictionary(
		[this, &building_type_manager](ProvinceDefinition& province, ast::NodeCPtr node) -> bool {
			return province.load_positions(*this, building_type_manager, node);
		}
	)(root);
}

bool Map::load_region_colours(ast::NodeCPtr root, std::vector<colour_t>& colours) {
	return expect_dictionary_reserve_length(
		colours,
		[&colours](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key != "color") {
				Logger::error("Invalid key in region colours: \"", key, "\"");
				return false;
			}
			return expect_colour(vector_callback(colours))(value);
	})(root);
}

bool Map::load_region_file(ast::NodeCPtr root, std::vector<colour_t> const& colours) {
	const bool ret = expect_dictionary_reserve_length(
		regions,
		[this, &colours](std::string_view region_identifier, ast::NodeCPtr region_node) -> bool {
			std::vector<ProvinceDefinition const*> provinces;

			bool ret = expect_list_reserve_length(
				provinces, expect_province_definition_identifier(vector_callback_pointer(provinces))
			)(region_node);

			ret &= add_region(region_identifier, std::move(provinces), colours[regions.size() % colours.size()]);

			return ret;
		}
	)(root);

	lock_regions();

	return ret;
}

static constexpr colour_t colour_at(uint8_t const* colour_data, int32_t idx) {
	/* colour_data is filled with BGR byte triplets - to get pixel idx as a
	 * single RGB value, multiply idx by 3 to get the index of the corresponding
	 * triplet, then combine the bytes in reverse order.
	 */
	idx *= 3;
	return { colour_data[idx + 2], colour_data[idx + 1], colour_data[idx] };
}

bool Map::load_map_images(fs::path const& province_path, fs::path const& terrain_path, bool detailed_errors) {
	if (!province_definitions_are_locked()) {
		Logger::error("Province index image cannot be generated until after provinces are locked!");
		return false;
	}
	if (!terrain_type_manager.terrain_type_mappings_are_locked()) {
		Logger::error("Province index image cannot be generated until after terrain type mappings are locked!");
		return false;
	}

	BMP province_bmp;
	if (!(province_bmp.open(province_path) && province_bmp.read_header() && province_bmp.read_pixel_data())) {
		Logger::error("Failed to read BMP for compatibility mode province image: ", province_path);
		return false;
	}
	static constexpr uint16_t expected_province_bpp = 24;
	if (province_bmp.get_bits_per_pixel() != expected_province_bpp) {
		Logger::error(
			"Invalid province BMP bits per pixel: ", province_bmp.get_bits_per_pixel(), " (expected ", expected_province_bpp,
			")"
		);
		return false;
	}

	BMP terrain_bmp;
	if (!(terrain_bmp.open(terrain_path) && terrain_bmp.read_header() && terrain_bmp.read_pixel_data())) {
		Logger::error("Failed to read BMP for compatibility mode terrain image: ", terrain_path);
		return false;
	}
	static constexpr uint16_t expected_terrain_bpp = 8;
	if (terrain_bmp.get_bits_per_pixel() != expected_terrain_bpp) {
		Logger::error(
			"Invalid terrain BMP bits per pixel: ", terrain_bmp.get_bits_per_pixel(), " (expected ", expected_terrain_bpp, ")"
		);
		return false;
	}

	if (province_bmp.get_width() != terrain_bmp.get_width() || province_bmp.get_height() != terrain_bmp.get_height()) {
		Logger::error(
			"Mismatched province and terrain BMP dims: ", province_bmp.get_width(), "x", province_bmp.get_height(), " vs ",
			terrain_bmp.get_width(), "x", terrain_bmp.get_height()
		);
		return false;
	}

	dims.x = province_bmp.get_width();
	dims.y = province_bmp.get_height();
	province_shape_image.resize(dims.x * dims.y);

	uint8_t const* province_data = province_bmp.get_pixel_data().data();
	uint8_t const* terrain_data = terrain_bmp.get_pixel_data().data();

	std::vector<fixed_point_map_t<TerrainType const*>> terrain_type_pixels_list(province_definitions.size());

	bool ret = true;
	ordered_set<colour_t> unrecognised_province_colours;

	std::vector<fixed_point_t> pixels_per_province(province_definitions.size());
	std::vector<fvec2_t> pixel_position_sum_per_province(province_definitions.size());

	for (ivec2_t pos {}; pos.y < get_height(); ++pos.y) {
		for (pos.x = 0; pos.x < get_width(); ++pos.x) {
			const size_t pixel_index = get_pixel_index_from_pos(pos);
			const colour_t province_colour = colour_at(province_data, pixel_index);
			ProvinceDefinition::index_t province_index = ProvinceDefinition::NULL_INDEX;

			if (pos.x > 0) {
				const size_t jdx = pixel_index - 1;
				if (colour_at(province_data, jdx) == province_colour) {
					province_index = province_shape_image[jdx].index;
					goto index_found;
				}
			}

			if (pos.y > 0) {
				const size_t jdx = pixel_index - get_width();
				if (colour_at(province_data, jdx) == province_colour) {
					province_index = province_shape_image[jdx].index;
					goto index_found;
				}
			}

			province_index = get_index_from_colour(province_colour);

			if (province_index == ProvinceDefinition::NULL_INDEX && !unrecognised_province_colours.contains(province_colour)) {
				unrecognised_province_colours.insert(province_colour);
				if (detailed_errors) {
					Logger::warning(
						"Unrecognised province colour ", province_colour, " at ", pos
					);
				}
			}

		index_found:
			province_shape_image[pixel_index].index = province_index;

			if (province_index != ProvinceDefinition::NULL_INDEX) {
				const ProvinceDefinition::index_t array_index = province_index - 1;
				pixels_per_province[array_index]++;
				pixel_position_sum_per_province[array_index] += static_cast<fvec2_t>(pos);
			}

			const TerrainTypeMapping::index_t terrain = terrain_data[pixel_index];
			TerrainTypeMapping const* mapping = terrain_type_manager.get_terrain_type_mapping_for(terrain);
			if (mapping != nullptr) {
				if (province_index != ProvinceDefinition::NULL_INDEX) {
					terrain_type_pixels_list[province_index - 1][&mapping->get_type()]++;
				}
				if (mapping->get_has_texture() && terrain < terrain_type_manager.get_terrain_texture_limit()) {
					province_shape_image[pixel_index].terrain = terrain + 1;
				} else {
					province_shape_image[pixel_index].terrain = 0;
				}
			} else {
				province_shape_image[pixel_index].terrain = 0;
			}
		}
	}

	if (!unrecognised_province_colours.empty()) {
		Logger::warning("Province image contains ", unrecognised_province_colours.size(), " unrecognised province colours");
	}

	size_t missing = 0;
	for (size_t array_index = 0; array_index < province_definitions.size(); ++array_index) {
		ProvinceDefinition* province = province_definitions.get_item_by_index(array_index);

		fixed_point_map_t<TerrainType const*> const& terrain_type_pixels = terrain_type_pixels_list[array_index];
		const fixed_point_map_const_iterator_t<TerrainType const*> largest = get_largest_item(terrain_type_pixels);
		province->default_terrain_type = largest != terrain_type_pixels.end() ? largest->first : nullptr;

		const fixed_point_t pixel_count = pixels_per_province[array_index];
		province->on_map = pixel_count > 0;

		if (province->on_map) {
			province->centre = pixel_position_sum_per_province[array_index] / pixel_count;
		} else {
			if (detailed_errors) {
				Logger::warning("Province missing from shape image: ", province->to_string());
			}
			missing++;
		}
	}
	if (missing > 0) {
		Logger::warning("Province image is missing ", missing, " province colours");
	}

	return ret;
}

/* REQUIREMENTS:
 * MAP-19, MAP-84
 */
bool Map::_generate_standard_province_adjacencies() {
	bool changed = false;

	const auto generate_adjacency = [this](ProvinceDefinition* current, ivec2_t pos) -> bool {
		ProvinceDefinition* neighbour = get_province_definition_at(pos);
		if (neighbour != nullptr) {
			return add_standard_adjacency(*current, *neighbour);
		}
		return false;
	};

	for (ivec2_t pos {}; pos.y < get_height(); ++pos.y) {
		for (pos.x = 0; pos.x < get_width(); ++pos.x) {
			ProvinceDefinition* cur = get_province_definition_at(pos);

			if (cur != nullptr) {
				changed |= generate_adjacency(cur, { (pos.x + 1) % get_width(), pos.y });
				changed |= generate_adjacency(cur, { pos.x, pos.y + 1 });
			}
		}
	}

	return changed;
}

bool Map::generate_and_load_province_adjacencies(std::vector<LineObject> const& additional_adjacencies) {
	bool ret = _generate_standard_province_adjacencies();
	if (!ret) {
		Logger::error("Failed to generate standard province adjacencies!");
	}
	/* Skip first line containing column headers */
	if (additional_adjacencies.size() <= 1) {
		Logger::error("No entries in province adjacencies file!");
		return false;
	}
	std::for_each(
		additional_adjacencies.begin() + 1, additional_adjacencies.end(), [this, &ret](LineObject const& adjacency) -> void {
			const std::string_view from_str = adjacency.get_value_for(0);
			if (from_str.empty() || from_str.front() == '#') {
				return;
			}
			ProvinceDefinition* const from = get_province_definition_by_identifier(from_str);
			if (from == nullptr) {
				Logger::error("Unrecognised adjacency from province identifier: \"", from_str, "\"");
				ret = false;
				return;
			}

			const std::string_view to_str = adjacency.get_value_for(1);
			ProvinceDefinition* const to = get_province_definition_by_identifier(to_str);
			if (to == nullptr) {
				Logger::error("Unrecognised adjacency to province identifier: \"", to_str, "\"");
				ret = false;
				return;
			}

			using enum adjacency_t::type_t;
			static const string_map_t<adjacency_t::type_t> type_map {
				{ "land", LAND }, { "sea", STRAIT }, { "impassable", IMPASSABLE }, { "canal", CANAL }
			};
			const std::string_view type_str = adjacency.get_value_for(2);
			const string_map_t<adjacency_t::type_t>::const_iterator it = type_map.find(type_str);
			if (it == type_map.end()) {
				Logger::error("Invalid adjacency type: \"", type_str, "\"");
				ret = false;
				return;
			}
			const adjacency_t::type_t type = it->second;

			ProvinceDefinition const* const through = get_province_definition_by_identifier(adjacency.get_value_for(3));

			const std::string_view data_str = adjacency.get_value_for(4);
			bool successful = false;
			const uint64_t data_uint = StringUtils::string_to_uint64(data_str, &successful);
			if (!successful || data_uint > std::numeric_limits<adjacency_t::data_t>::max()) {
				Logger::error("Invalid adjacency data: \"", data_str, "\"");
				ret = false;
				return;
			}
			const adjacency_t::data_t data = data_uint;

			ret &= add_special_adjacency(*from, *to, type, through, data);
		}
	);
	return ret;
}

bool Map::load_climate_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	bool ret = expect_dictionary_reserve_length(
		climates,
		[this, &modifier_manager](std::string_view identifier, ast::NodeCPtr node) -> bool {
			if (identifier.empty()) {
				Logger::error("Invalid climate identifier - empty!");
				return false;
			}

			bool ret = true;
			Climate* cur_climate = climates.get_item_by_identifier(identifier);
			if (cur_climate == nullptr) {
				ModifierValue values;
				ret &= modifier_manager.expect_modifier_value(move_variable_callback(values))(node);
				ret &= climates.add_item({ identifier, std::move(values) });
			} else {
				ret &= expect_list_reserve_length(*cur_climate, expect_province_definition_identifier(
					[cur_climate, &identifier](ProvinceDefinition& province) {
						if (province.climate != cur_climate) {
							cur_climate->add_province(&province);
							if (province.climate != nullptr) {
								Climate* old_climate = const_cast<Climate*>(province.climate);
								old_climate->remove_province(&province);
								Logger::warning(
									"Province with id ", province.get_identifier(),
									" found in multiple climates: ", identifier,
									" and ", old_climate->get_identifier()
								);
							}
							province.climate = cur_climate;
						} else {
							Logger::warning(
								"Province with id ", province.get_identifier(),
								" defined twice in climate ", identifier
							);
						}
						return true;
					}
				))(node);
			}
			return ret;
		}
	)(root);

	for (Climate& climate : climates.get_items()) {
		climate.lock();
	}

	lock_climates();

	return ret;
}

bool Map::load_continent_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	bool ret = expect_dictionary_reserve_length(
		continents,
		[this, &modifier_manager](std::string_view identifier, ast::NodeCPtr node) -> bool {
			if (identifier.empty()) {
				Logger::error("Invalid continent identifier - empty!");
				return false;
			}

			ModifierValue values;
			std::vector<ProvinceDefinition const*> prov_list;
			bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(values),
				"provinces", ONE_EXACTLY, expect_list_reserve_length(prov_list, expect_province_definition_identifier(
					[&prov_list](ProvinceDefinition const& province) -> bool {
						if (province.continent == nullptr) {
							prov_list.emplace_back(&province);
						} else {
							Logger::warning("Province ", province, " found in multiple continents");
						}
						return true;
					}
				))
			)(node);

			Continent continent = { identifier, std::move(values) };
			continent.add_provinces(prov_list);
			continent.lock();

			if (continents.add_item(std::move(continent))) {
				Continent const& moved_continent = continents.get_items().back();
				for (ProvinceDefinition const* prov : moved_continent.get_provinces()) {
					remove_province_definition_const(prov)->continent = &moved_continent;
				}
			} else {
				ret = false;
			}

			return ret;
		}
	)(root);

	lock_continents();

	return ret;
}
