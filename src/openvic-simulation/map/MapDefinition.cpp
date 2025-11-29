#include "MapDefinition.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <system_error>
#include <vector>

#include <fmt/std.h>

#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/view/adjacent_remove_if.hpp>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/io/BMP.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/OrderedContainersMath.hpp"
#include "openvic-simulation/types/TypedSpan.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/FormatValidate.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/core/string/CharConv.hpp"
#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

MapDefinition::MapDefinition() {}

ProvinceDefinition* MapDefinition::get_province_definition_from_number(
	decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
) {
	return province_definitions.get_item_by_index(ProvinceDefinition::get_index_from_province_number(province_number));
}
ProvinceDefinition const* MapDefinition::get_province_definition_from_number(
	decltype(std::declval<ProvinceDefinition>().get_province_number())province_number
) const {
	return province_definitions.get_item_by_index(ProvinceDefinition::get_index_from_province_number(province_number));
}

RiverSegment::RiverSegment(uint8_t new_size, memory::vector<ivec2_t>&& new_points)
	: size { new_size }, points { std::move(new_points) } {}

//1 entry reserved for null province
static constexpr province_index_t MAX_PROVINCE_COUNT = std::numeric_limits<province_index_t>().max() - 1;

bool MapDefinition::add_province_definition(std::string_view identifier, colour_t colour) {
	if (province_definitions.size() >= type_safe::get(max_provinces)) {
		spdlog::error_s(
			"The map's province list is full - maximum number of provinces is {} (this can be at most {})",
			max_provinces, MAX_PROVINCE_COUNT
		);
		return false;
	}
	if (identifier.empty()) {
		spdlog::error_s("Invalid province identifier - empty!");
		return false;
	}
	// Victoria 2 CTDs on non-numeric province numbers
	if (!ranges::all_of(identifier, [](char c) -> bool { return std::isdigit(c); })) {
		spdlog::error_s(
			"Invalid province identifier: {} (can only contain numeric characters)", identifier
		);
		return false;
	}
	if (colour.is_null()) {
		spdlog::error_s("Invalid province colour for {} - null! ({})", identifier, colour);
		return false;
	}
	const ProvinceDefinition::province_number_t province_number = get_province_number_from_colour(colour);
	if (province_number != ProvinceDefinition::NULL_PROVINCE_NUMBER) {
		spdlog::error_s(
			"Duplicate province colours: {} and {}",
			get_province_definition_from_number(province_number)->to_string(), identifier
		);
		return false;
	}

	if (!province_definitions.emplace_item(
		identifier,
		identifier, colour, province_index_t(get_province_definition_count())
	)) {
		return false;
	}

	ProvinceDefinition const& new_province = province_definitions.back();
	colour_index_map[new_province.get_colour()] = new_province.index;
	return true;
}

ProvinceDefinition::distance_t MapDefinition::calculate_distance_between(
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
bool MapDefinition::add_standard_adjacency(ProvinceDefinition& from, ProvinceDefinition& to) {
	if (from == to) {
		return false;
	}

	const bool from_needs_adjacency = !from.is_adjacent_to(&to);
	const bool to_needs_adjacency = !to.is_adjacent_to(&from);

	if (from_needs_adjacency != to_needs_adjacency) {
		spdlog::error_s("Inconsistent adjacency state between provinces {} and {}", from, to);
		return false;
	}

	if (!from_needs_adjacency) {
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

		if (from.is_water()) {
			path_map_sea.try_add_point(to.get_province_number(), { to.centre.x.round<int>(), to.centre.y.round<int>() });
		} else {
			path_map_sea.try_add_point(from.get_province_number(), { from.centre.x.round<int>(), from.centre.y.round<int>() });
		}
		/* Connect points on pathfinding map */
		path_map_sea.connect_points(from.get_province_number(), to.get_province_number());
		/* Land units can use transports to path on the sea */
		path_map_land.connect_points(from.get_province_number(), to.get_province_number());
		/* Sea points are only valid for land units with a transport */
		if (from.is_water()) {
			path_map_land.set_point_disabled(from.get_province_number());
		} else {
			path_map_land.set_point_disabled(to.get_province_number());
		}
	} else if (from.is_water()) {
		/* Water-to-water adjacency */
		type = WATER;

		/* Connect points on pathfinding map */
		path_map_sea.connect_points(from.get_province_number(), to.get_province_number());
		/* Land units can use transports to path on the sea */
		path_map_land.connect_points(from.get_province_number(), to.get_province_number());
		/* Sea points are only valid for land units with a transport */
		if (from.is_water()) {
			path_map_land.set_point_disabled(from.get_province_number());
		} else {
			path_map_land.set_point_disabled(to.get_province_number());
		}
	} else {
		/* Connect points on pathfinding map */
		path_map_land.connect_points(from.get_province_number(), to.get_province_number());
	}

	if (from_needs_adjacency) {
		from.adjacencies.emplace_back(&to, distance, type, nullptr, 0);
	}
	if (to_needs_adjacency) {
		to.adjacencies.emplace_back(&from, distance, type, nullptr, 0);
	}
	return true;
}

bool MapDefinition::add_special_adjacency(
	ProvinceDefinition& from, ProvinceDefinition& to, adjacency_t::type_t type, ProvinceDefinition const* through,
	adjacency_t::data_t data
) {
	if (from == to) {
		spdlog::error_s(
			"Trying to add {} adjacency from province {} to itself!",
			adjacency_t::get_type_name(type), from
		);
		return false;
	}

	using enum adjacency_t::type_t;

	/* Check end points */
	switch (type) {
	case LAND:
	case STRAIT:
		if (from.is_water() || to.is_water()) {
			spdlog::error_s(
				"{} adjacency from {} to {} has water endpoint(s)!",
				adjacency_t::get_type_name(type), from, to
			);
			return false;
		}
		break;
	case WATER:
	case CANAL:
		if (!from.is_water() || !to.is_water()) {
			spdlog::error_s(
				"{} adjacency from {} to {} has land endpoint(s)!",
				adjacency_t::get_type_name(type), from, to
			);
			return false;
		}
		break;
	case COASTAL:
		if (from.is_water() == to.is_water()) {
			spdlog::error_s("Coastal adjacency from {} to {} has both land or water endpoints!", from, to);
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
		spdlog::error_s("Invalid adjacency type {}", static_cast<uint32_t>(type));
		return false;
	}

	/* Check through province */
	if (type == STRAIT || type == CANAL) {
		const bool water_expected = type == STRAIT;
		if (through == nullptr || through->is_water() != water_expected) {
			spdlog::error_s(
				"{} adjacency from {} to {} has a {} through province {}",
				adjacency_t::get_type_name(type),
				from,
				to,
				through == nullptr ? "null" : (water_expected ? "land" : "water"),
				ovfmt::validate(through)
			);
			return false;
		}
	} else if (through != nullptr) {
		spdlog::warn_s(
			"{} adjacency from {} to {} has a non-null through province {}",
			adjacency_t::get_type_name(type), from, to, *through
		);
		through = nullptr;
	}

	/* Check canal data */
	if (data != adjacency_t::DEFAULT_DATA && type != CANAL) {
		spdlog::warn_s(
			"{} adjacency from {} to {} has invalid data {}",
			adjacency_t::get_type_name(type), from, to, static_cast<uint32_t>(data)
		);
		data = adjacency_t::DEFAULT_DATA;
	}

	const ProvinceDefinition::distance_t distance = calculate_distance_between(from, to);

	const auto add_adjacency = [this, distance, type, through, data](
		ProvinceDefinition& from, ProvinceDefinition const& to
	) -> bool {
		const memory::vector<adjacency_t>::iterator existing_adjacency = std::find_if(
			from.adjacencies.begin(), from.adjacencies.end(),
			[&to](adjacency_t const& adj) -> bool { return adj.get_to() == &to; }
		);
		if (existing_adjacency != from.adjacencies.end()) {
			if (type == existing_adjacency->get_type()) {
				spdlog::warn_s(
					"Adjacency from {} to {} already has type {}!",
					from, to, adjacency_t::get_type_name(type)
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
					spdlog::error_s(
						"Provinces {} and {} already have an existing {} adjacency, cannot create a {} adjacency!",
						from, to, adjacency_t::get_type_name(existing_adjacency->get_type()), adjacency_t::get_type_name(type)
					);
					return false;
				}
				if (
					type != existing_adjacency->get_type() && existing_adjacency->get_type() != (type == CANAL ? WATER : LAND)
				) {
					spdlog::error_s(
						"Cannot convert {} adjacency from {} to {} to type {}!",
						adjacency_t::get_type_name(existing_adjacency->get_type()), from, to, adjacency_t::get_type_name(type)
					);
					return false;
				}
			}
			*existing_adjacency = { &to, distance, type, through, data };
			if (from.is_water() && to.is_water()) {
				path_map_sea.connect_points(from.get_province_number(), to.get_province_number());
			} else {
				path_map_land.connect_points(from.get_province_number(), to.get_province_number());
			}

			if (from.is_water() || to.is_water()) {
				path_map_land.connect_points(from.get_province_number(), to.get_province_number());
				if (from.is_water()) {
					path_map_land.set_point_disabled(from.get_province_number());
				} else {
					path_map_land.set_point_disabled(to.get_province_number());
				}
			}
			return true;
		} else if (type == IMPASSABLE) {
			spdlog::warn_s(
				"Provinces {} and {} do not have an existing adjacency to make impassable!",
				from, to
			);
			return true;
		} else {
			from.adjacencies.emplace_back(&to, distance, type, through, data);
			if (from.is_water() && to.is_water()) {
				path_map_sea.connect_points(from.get_province_number(), to.get_province_number());
			} else {
				path_map_land.connect_points(from.get_province_number(), to.get_province_number());
			}

			if (from.is_water() || to.is_water()) {
				path_map_land.connect_points(from.get_province_number(), to.get_province_number());
				if (from.is_water()) {
					path_map_land.set_point_disabled(from.get_province_number());
				} else {
					path_map_land.set_point_disabled(to.get_province_number());
				}
			}
			return true;
		}
	};

	return add_adjacency(from, to) & add_adjacency(to, from);
}

bool MapDefinition::set_water_province(std::string_view identifier) {
	if (water_provinces.is_locked()) {
		spdlog::error_s("The map's water provinces have already been locked!");
		return false;
	}

	ProvinceDefinition* province = get_province_definition_by_identifier(identifier);

	if (province == nullptr) {
		spdlog::error_s("Unrecognised water province identifier: {}", identifier);
		return false;
	}
	if (province->is_water()) {
		spdlog::warn_s("Province {} is already a water province!", identifier);
		return true;
	}
	if (!water_provinces.add_province(province)) {
		spdlog::error_s("Failed to add province {} to water province set!", identifier);
		return false;
	}
	province->water = true;
	path_map_sea.try_add_point(province->get_province_number(), path_map_land.get_point_position(province->get_province_number()));
	return true;
}

bool MapDefinition::set_water_province_list(memory::vector<std::string_view> const& list) {
	if (water_provinces.is_locked()) {
		spdlog::error_s("The map's water provinces have already been locked!");
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

void MapDefinition::lock_water_provinces() {
	water_provinces.lock();
	SPDLOG_INFO("Locked water provinces after registering {}", water_provinces.size());
}

size_t MapDefinition::get_land_province_count() const {
	return get_province_definition_count() - get_water_province_count();
}

size_t MapDefinition::get_water_province_count() const {
	return water_provinces.size();
}

bool MapDefinition::add_region(std::string_view identifier, memory::vector<ProvinceDefinition const*>&& provinces, colour_t colour) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid region identifier - empty!");
		return false;
	}

	bool ret = true;

	// mods like DoD + TGC use meta regions of water provinces
	bool is_meta = provinces.empty();
	size_t valid_provinces_count { provinces.size() };
	for (ProvinceDefinition const* const province_definition_ptr : provinces) {
		ProvinceDefinition const& province_definition = *province_definition_ptr;
		if (OV_unlikely(province_definition.has_region())) {
			spdlog::warn_s(
				"Province {} is assigned to multiple regions, including {} and {}. First defined region wins.",
				province_definition,
				*province_definition.get_region(),
				identifier
			);
			valid_provinces_count--;
		}

		is_meta |= province_definition.is_water();
	}

	if (!regions.emplace_item(
		identifier,
		identifier, colour, is_meta
	)) {
		return false;
	}

	Region& region = regions.back();

	if (OV_likely(valid_provinces_count == provinces.size())) {
		ret &= region.add_provinces(provinces);
	} else {
		region.reserve(valid_provinces_count);
		for (ProvinceDefinition const* const province_definition_ptr : provinces) {
			if (!province_definition_ptr->has_region()) {
				region.add_province(province_definition_ptr);
			}
		}
	}

	region.lock();
	if (OV_unlikely(is_meta)) {
		SPDLOG_INFO("Region {} is meta.", identifier);
	} else {
		for (ProvinceDefinition const* province_definition : region.get_provinces()) {
			remove_province_definition_const(province_definition)->region = &region;
		}
	}
	return ret;
}

ProvinceDefinition::province_number_t MapDefinition::get_province_number_from_colour(colour_t colour) const {
	const colour_index_map_t::const_iterator it = colour_index_map.find(colour);
	if (it != colour_index_map.end()) {
		return ProvinceDefinition::get_province_number_from_index(it.value());
	}
	return ProvinceDefinition::NULL_PROVINCE_NUMBER;
}

ProvinceDefinition::province_number_t MapDefinition::get_province_number_at(ivec2_t pos) const {
	if (pos.nonnegative() && pos.is_within_bound(dims)) {
		return province_shape_image[get_pixel_index_from_pos(pos)].province_number;
	}
	return ProvinceDefinition::NULL_PROVINCE_NUMBER;
}

ProvinceDefinition* MapDefinition::get_province_definition_at(ivec2_t pos) {
	return get_province_definition_from_number(get_province_number_at(pos));
}

ProvinceDefinition const* MapDefinition::get_province_definition_at(ivec2_t pos) const {
	return get_province_definition_from_number(get_province_number_at(pos));
}

bool MapDefinition::set_max_provinces(province_index_t new_max_provinces) {
	if (new_max_provinces <= province_index_t(0)) {
		spdlog::error_s(
			"Trying to set max province count to an invalid value {} (must be greater than 0)",
			new_max_provinces
		);
		return false;
	}
	if (new_max_provinces > MAX_PROVINCE_COUNT) {
		spdlog::error_s(
			"Trying to set max province count to an invalid value {} (must <= {})",
			new_max_provinces,
			MAX_PROVINCE_COUNT
		);
		return false;
	}
	if (!province_definitions.empty() || province_definitions.is_locked()) {
		spdlog::error_s(
			"Trying to set max province count to {} after provinces have already been added and/or locked", new_max_provinces
		);
		return false;
	}
	max_provinces = new_max_provinces;
	path_map_land.reserve_space(type_safe::get(max_provinces));
	path_map_sea.reserve_space(type_safe::get(max_provinces));
	return true;
}

using namespace ovdl::csv;

static bool _validate_province_definitions_header(LineObject const& header) {
	static const memory::vector<memory::string> standard_header { "province", "red", "green", "blue" };
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
		uint64_t val;
		std::from_chars_result result = string_to_uint64(component, val);
		successful = result.ec == std::errc{};
		if (successful && val <= colour_t::max_value) {
			colour[i] = val;
		} else {
			ret = false;
		}
	}
	return ret;
}

bool MapDefinition::load_province_definitions(std::span<const LineObject> lines) {
	bool ret = true;

	if (lines.empty()) {
		spdlog::error_s("No header or entries in province definition file!");
		ret = false;
	} else {
		LineObject const& header = lines.front();
		if (!_validate_province_definitions_header(header)) {
			spdlog::error_s(
				"Non-standard province definition file header - make sure this is not a province definition: {}",
				header
			);
		}

		if (lines.size() <= 1) {
			spdlog::error_s("No entries in province definition file!");
			ret = false;
		} else {
			reserve_more_province_definitions(lines.size() - 1);
			colour_index_map.reserve(lines.size() - 1);

			std::for_each(lines.begin() + 1, lines.end(), [this, &ret](LineObject const& line) -> void {
				const std::string_view identifier = line.get_value_for(0);
				if (!identifier.empty()) {
					colour_t colour = colour_t::null();
					if (!_parse_province_colour(colour, { line.get_value_for(1), line.get_value_for(2), line.get_value_for(3) })) {
						spdlog::error_s(
							"Error reading colour in province definition: {}",
							line
						);
						ret = false;
					}
					ret &= add_province_definition(identifier, colour);
					if (!ret) {
						return;
					}

					ProvinceDefinition const& definition = province_definitions.back();
					ret &= path_map_land.try_add_point(definition.get_province_number(), { definition.centre.x.round<int>(), definition.centre.y.round<int>() });
					if (!ret) {
						spdlog::error_s("Province {} could not be added to " _OV_STR(path_map_land), identifier);
					}
				}
			});

			colour_index_map.shrink_to_fit();
		}
	}

	lock_province_definitions();

	return ret;
}

bool MapDefinition::load_province_positions(BuildingTypeManager const& building_type_manager, ast::NodeCPtr root) {
	return expect_province_definition_dictionary(
		[this, &building_type_manager](ProvinceDefinition& province, ast::NodeCPtr node) -> bool {
			return province.load_positions(*this, building_type_manager, node);
		}
	)(root);
}

bool MapDefinition::load_region_colours(ast::NodeCPtr root, memory::vector<colour_t>& colours) {
	return expect_dictionary_reserve_length(
		colours,
		[&colours](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key != "color") {
				spdlog::error_s("Invalid key in region colours: \"{}\"", key);
				return false;
			}
			return expect_colour(vector_callback(colours))(value);
	})(root);
}

bool MapDefinition::load_region_file(ast::NodeCPtr root, std::span<const colour_t> colours) {
	const bool ret = expect_dictionary_reserve_length(
		regions,
		[this, &colours](std::string_view region_identifier, ast::NodeCPtr region_node) -> bool {
			memory::vector<ProvinceDefinition const*> provinces;

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

// Constants in the River BMP Palette
static constexpr uint8_t START_COLOUR = 0;
static constexpr uint8_t MERGE_COLOUR = 1;
static constexpr uint8_t RIVER_SIZE_MIN = 2;
static constexpr uint8_t RIVER_SIZE_MAX = 11;

static constexpr size_t RIVER_RECURSION_LIMIT = 4096;

void MapDefinition::_trace_river(BMP& rivers_bmp, ivec2_t start, river_t& river) {
	enum struct direction_t : uint8_t { START, UP, DOWN, LEFT, RIGHT };
	using enum direction_t;
	struct TraceSegment {
		ivec2_t point;
		direction_t direction;
	};

	uint8_t const* river_data = rivers_bmp.get_pixel_data().data();

	memory::stack<TraceSegment> stack;
	stack.push({ start, START });

	while (!stack.empty()) {
		TraceSegment segment = stack.top();
		stack.pop();

		thread_local memory::vector<ivec2_t> points;
		points.emplace_back(segment.point);

		uint8_t size = river_data[segment.point.x + segment.point.y * rivers_bmp.get_width()] - 1; // determine river size by colour
		bool river_complete = false;
		size_t recursion_limit = 0;

		while(true) {
			++recursion_limit;
			if (recursion_limit == RIVER_RECURSION_LIMIT) {
				spdlog::error_s(
					"River segment starting @ ({}, {}) exceeded length limit of 4096 pixels. Check for misplaced pixel or circular river.",
					points.front().x, points.front().y
				);
				break;
			}

			size_t idx = segment.point.x + segment.point.y * rivers_bmp.get_width();

			ivec2_t merge_point;
			bool merge_found = false;

			ivec2_t new_segment_point;
			direction_t new_segment_direction;
			bool new_segment_found = false;

			struct Neighbour {
				ivec2_t offset;
				direction_t old_direction;
				direction_t new_direction;
			};
			static constexpr std::array<Neighbour, 4> neighbours = {{
				{ {0, -1}, UP, DOWN },  // Down
				{ {1,  0}, LEFT, RIGHT },  // Right
				{ {0,  1}, DOWN, UP },  // Up
				{ {-1, 0}, RIGHT, LEFT },  // Left
			}};

			for (const auto& neighbour : neighbours) {
				ivec2_t neighbour_pos = { segment.point.x + neighbour.offset.x, segment.point.y + neighbour.offset.y };
				if (neighbour_pos.x < 0 || neighbour_pos.y < 0 || neighbour_pos.x >= rivers_bmp.get_width() ||
					neighbour_pos.y >= rivers_bmp.get_height() || segment.direction == neighbour.old_direction) {
					continue;
				}

				uint8_t neighbour_color = river_data[neighbour_pos.x + neighbour_pos.y * rivers_bmp.get_width()];
				if (neighbour_color == size + 1) {
					points.emplace_back(neighbour_pos);
					segment.point = neighbour_pos;
					segment.direction = neighbour.new_direction;
					new_segment_found = false;
					break;
				} else if (neighbour_color == MERGE_COLOUR) {
					merge_found = true;
					merge_point = neighbour_pos;
				} else if (neighbour_color > 1 && neighbour_color < 12) {
					new_segment_point = neighbour_pos;
					new_segment_direction = neighbour.new_direction;
					new_segment_found = true;
				}
			}

			if (merge_found) {
				points.emplace_back(merge_point);
				river_complete = true;
				break;
			}

			if (new_segment_found) {
				stack.push({ new_segment_point, new_segment_direction });
				break;
			}

			if (!new_segment_found) {
				break; // no neighbours left to check, end segment
			}
		}

		// simplify points to only include first, last, and corners
		const auto is_corner_point = [](ivec2_t previous, ivec2_t current, ivec2_t next) -> bool {
			return ((current.x - previous.x) * (next.y - current.y)) != ((current.y - previous.y) * (next.x - current.x)); //slope is fun!
		};
		memory::vector<ivec2_t> simplified_points;
		simplified_points.emplace_back(points.front());
		if (points.size() != 1) {
			for (int i = 1; i < points.size() - 1; ++i) {
				if (is_corner_point(points[i - 1], points[i], points[i + 1])) {
					simplified_points.emplace_back(points[i]);
				}
			}
			simplified_points.emplace_back(points.back());
		}

		river.push_back({ size, std::move(simplified_points) });
	}
}

bool MapDefinition::load_map_images(fs::path const& province_path, fs::path const& terrain_path, fs::path const& rivers_path, bool detailed_errors) {
	if (!province_definitions_are_locked()) {
		spdlog::error_s("Province index image cannot be generated until after provinces are locked!");
		return false;
	}
	if (!terrain_type_manager.terrain_type_mappings_are_locked()) {
		spdlog::error_s("Province index image cannot be generated until after terrain type mappings are locked!");
		return false;
	}

	static constexpr uint16_t expected_province_bpp = 24;
	static constexpr uint16_t expected_terrain_rivers_bpp = 8;

	BMP province_bmp;
	if (!(province_bmp.open(province_path) && province_bmp.read_header() && province_bmp.read_pixel_data())) {
		spdlog::error_s("Failed to read BMP for compatibility mode province image: {}", province_path);
		return false;
	}
	if (province_bmp.get_bits_per_pixel() != expected_province_bpp) {
		spdlog::error_s(
			"Invalid province BMP bits per pixel: {} (expected {})",
			province_bmp.get_bits_per_pixel(), expected_province_bpp
		);
		return false;
	}

	BMP terrain_bmp;
	if (!(terrain_bmp.open(terrain_path) && terrain_bmp.read_header() && terrain_bmp.read_pixel_data())) {
		spdlog::error_s("Failed to read BMP for compatibility mode terrain image: {}", terrain_path);
		return false;
	}
	if (terrain_bmp.get_bits_per_pixel() != expected_terrain_rivers_bpp) {
		spdlog::error_s(
			"Invalid terrain BMP bits per pixel: {} (expected {})",
			terrain_bmp.get_bits_per_pixel(), expected_terrain_rivers_bpp
		);
		return false;
	}

	BMP rivers_bmp;
	if (!(rivers_bmp.open(rivers_path) && rivers_bmp.read_header() && rivers_bmp.read_pixel_data())) {
		spdlog::error_s("Failed to read BMP for compatibility mode river image: {}", rivers_path);
		return false;
	}
	if (rivers_bmp.get_bits_per_pixel() != expected_terrain_rivers_bpp) {
		spdlog::error_s(
			"Invalid rivers BMP bits per pixel: {} (expected {})",
			rivers_bmp.get_bits_per_pixel(), expected_terrain_rivers_bpp
		);
		return false;
	}

	if (province_bmp.get_width() != terrain_bmp.get_width() ||
		province_bmp.get_height() != terrain_bmp.get_height() ||
		province_bmp.get_width() != rivers_bmp.get_width() ||
		province_bmp.get_height() != rivers_bmp.get_height()
	) {
		spdlog::error_s(
			"Mismatched map BMP dims: provinces:{}x{}, terrain: {}x{}, rivers: {}x{}",
			province_bmp.get_width(), province_bmp.get_height(),
			terrain_bmp.get_width(), terrain_bmp.get_height(),
			rivers_bmp.get_width(), rivers_bmp.get_height()
		);
		return false;
	}

	dims.x = province_bmp.get_width();
	dims.y = province_bmp.get_height();
	province_shape_image.resize(dims.x * dims.y);

	uint8_t const* province_data = province_bmp.get_pixel_data().data();
	uint8_t const* terrain_data = terrain_bmp.get_pixel_data().data();

	memory::FixedVector<fixed_point_map_t<TerrainType const*>> _terrain_type_pixels_list(
		province_definitions.size(),
		[](const size_t i) { return fixed_point_map_t<TerrainType const*>{}; }
	);
	TypedSpan<province_index_t, fixed_point_map_t<TerrainType const*>> terrain_type_pixels_list { _terrain_type_pixels_list };

	bool ret = true;
	ordered_set<colour_t> unrecognised_province_colours;

	memory::FixedVector<fixed_point_t> _pixels_per_province(
		province_definitions.size(),
		[](const size_t i) { return fixed_point_t::_0; }
	);
	TypedSpan<province_index_t, fixed_point_t> pixels_per_province { _pixels_per_province };
	memory::FixedVector<fvec2_t> _pixel_position_sum_per_province(
		province_definitions.size(),
		[](const size_t i) { return fvec2_t{}; }
	);
	TypedSpan<province_index_t, fvec2_t> pixel_position_sum_per_province { _pixel_position_sum_per_province };

	for (ivec2_t pos {}; pos.y < get_height(); ++pos.y) {
		for (pos.x = 0; pos.x < get_width(); ++pos.x) {
			const size_t pixel_index = get_pixel_index_from_pos(pos);
			const colour_t province_colour = colour_at(province_data, pixel_index);
			ProvinceDefinition::province_number_t province_number = ProvinceDefinition::NULL_PROVINCE_NUMBER;

			if (pos.x > 0) {
				const size_t jdx = pixel_index - 1;
				if (colour_at(province_data, jdx) == province_colour) {
					province_number = province_shape_image[jdx].province_number;
					goto index_found;
				}
			}

			if (pos.y > 0) {
				const size_t jdx = pixel_index - get_width();
				if (colour_at(province_data, jdx) == province_colour) {
					province_number = province_shape_image[jdx].province_number;
					goto index_found;
				}
			}

			province_number = get_province_number_from_colour(province_colour);

			if (province_number == ProvinceDefinition::NULL_PROVINCE_NUMBER && !unrecognised_province_colours.contains(province_colour)) {
				unrecognised_province_colours.insert(province_colour);
				if (detailed_errors) {
					spdlog::warn_s(
						"Unrecognised province colour {} at {}",
						province_colour, pos
					);
				}
			}

		index_found:
			province_shape_image[pixel_index].province_number = province_number;

			if (province_number != ProvinceDefinition::NULL_PROVINCE_NUMBER) {
				const province_index_t province_index = ProvinceDefinition::get_index_from_province_number(province_number);
				pixels_per_province[province_index]++;
				pixel_position_sum_per_province[province_index] += static_cast<fvec2_t>(pos);
			}

			const TerrainTypeMapping::index_t terrain = terrain_data[pixel_index];
			TerrainTypeMapping const* mapping = terrain_type_manager.get_terrain_type_mapping_for(terrain);
			if (mapping != nullptr) {
				if (province_number != ProvinceDefinition::NULL_PROVINCE_NUMBER) {
					const province_index_t province_index = ProvinceDefinition::get_index_from_province_number(province_number);
					terrain_type_pixels_list[province_index][&mapping->type]++;
				}
				if (mapping->has_texture && terrain < terrain_type_manager.get_terrain_texture_limit()) {
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
		spdlog::warn_s("Province image contains {} unrecognised province colours", unrecognised_province_colours.size());
	}

	size_t missing = 0;
	for (ProvinceDefinition& province : province_definitions.get_items()) {
		const province_index_t province_index = province.index;
		fixed_point_map_t<TerrainType const*> const& terrain_type_pixels = terrain_type_pixels_list[province_index];
		const fixed_point_map_const_iterator_t<TerrainType const*> largest = get_largest_item(terrain_type_pixels);
		province.default_terrain_type = largest != terrain_type_pixels.end() ? largest->first : nullptr;

		const fixed_point_t pixel_count = pixels_per_province[province_index];
		province.on_map = pixel_count > 0;

		if (province.on_map) {
			province.centre = pixel_position_sum_per_province[province_index] / pixel_count;
		} else {
			if (detailed_errors) {
				spdlog::warn_s("Province missing from shape image: {}", province.to_string());
			}
			missing++;
		}
	}
	if (missing > 0) {
		spdlog::warn_s("Province image is missing {} province colours", missing);
	}

	/** Generating River Segments
		1. check pixels up, right, down, and left from last_segment_end for a colour <12
		2. add first point
		3. set size of segment based on color value at first point
		4. loop, adding adjacent points until the colour value changes - no backtracking
				last_segment_direction:
				0 -> start, ignore nothing
				1 -> ignore up
				2 -> ignore down
				3 -> ignore left
				4 -> ignore right
		5. if the colour value changes to MERGE_COLOUR, add the point & finish the segment
		6. if there is no further point, finish the segment
		7. if the colour value changes to a different river size (>1 && <12), add a new stack frame with this segment
	*/

	uint8_t const* river_data = rivers_bmp.get_pixel_data().data();

	// find every river source and then run the segment algorithm.
	for (int y = 0; y < rivers_bmp.get_height(); ++y) {
		for (int x = 0; x < rivers_bmp.get_width(); ++x) {
			if (river_data[x + y * rivers_bmp.get_width()] == START_COLOUR) { // start of a river
				river_t river;
				_trace_river(rivers_bmp, { x, y }, river);
				rivers.emplace_back(std::move(river));
			}
		}
	}

	SPDLOG_INFO("Generated {} rivers.", rivers.size());

	return ret;
}

/* REQUIREMENTS:
 * MAP-19, MAP-84
 */
bool MapDefinition::_generate_standard_province_adjacencies() {
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

bool MapDefinition::generate_and_load_province_adjacencies(std::span<const LineObject> additional_adjacencies) {
	bool ret = _generate_standard_province_adjacencies();
	if (!ret) {
		spdlog::error_s("Failed to generate standard province adjacencies!");
	}
	/* Skip first line containing column headers */
	if (additional_adjacencies.size() <= 1) {
		spdlog::error_s("No entries in province adjacencies file!");
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
				spdlog::error_s("Unrecognised adjacency from province identifier: \"{}\"", from_str);
				ret = false;
				return;
			}

			const std::string_view to_str = adjacency.get_value_for(1);
			ProvinceDefinition* const to = get_province_definition_by_identifier(to_str);
			if (to == nullptr) {
				spdlog::error_s("Unrecognised adjacency to province identifier: \"{}\"", to_str);
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
				spdlog::error_s("Invalid adjacency type: \"{}\"", type_str);
				ret = false;
				return;
			}
			const adjacency_t::type_t type = it->second;

			ProvinceDefinition const* const through = get_province_definition_by_identifier(adjacency.get_value_for(3));

			const std::string_view data_str = adjacency.get_value_for(4);
			bool successful = false;
			uint64_t data_uint;
			std::from_chars_result result = string_to_uint64(data_str, data_uint);
			successful = result.ec == std::errc{};
			if (!successful || data_uint > std::numeric_limits<adjacency_t::data_t>::max()) {
				spdlog::error_s("Invalid adjacency data: \"{}\"", data_str);
				ret = false;
				return;
			}
			const adjacency_t::data_t data = data_uint;

			ret &= add_special_adjacency(*from, *to, type, through, data);
		}
	);

	path_map_land.shrink_to_fit();
	path_map_sea.shrink_to_fit();
	return ret;
}

bool MapDefinition::load_climate_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	bool ret = expect_dictionary_reserve_length(
		climates,
		[this, &modifier_manager](std::string_view identifier, ast::NodeCPtr node) -> bool {
			if (identifier.empty()) {
				spdlog::error_s("Invalid climate identifier - empty!");
				return false;
			}

			bool ret = true;
			Climate* cur_climate = climates.get_item_by_identifier(identifier);
			if (cur_climate == nullptr) {
				ModifierValue values;

				ret &= NodeTools::expect_dictionary(
					modifier_manager.expect_base_province_modifier(values)
				)(node);

				ret &= climates.emplace_item(
					identifier,
					identifier, std::move(values), Modifier::modifier_type_t::CLIMATE
				);
			} else {
				ret &= expect_list_reserve_length(*cur_climate, expect_province_definition_identifier(
					[cur_climate, &identifier](ProvinceDefinition& province) {
						if (province.climate != cur_climate) {
							cur_climate->add_province(&province);
							if (province.climate != nullptr) {
								Climate* old_climate = const_cast<Climate*>(province.climate);
								old_climate->remove_province(&province);
								spdlog::warn_s(
									"Province with id {} found in multiple climates: {} and {}",
									province, identifier, *old_climate
								);
							}
							province.climate = cur_climate;
						} else {
							spdlog::warn_s(
								"Province with id {} defined twice in climate {}",
								province, identifier
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

bool MapDefinition::load_continent_file(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	bool ret = expect_dictionary_reserve_length(
		continents,
		[this, &modifier_manager](std::string_view identifier, ast::NodeCPtr node) -> bool {

			if (identifier.empty()) {
				spdlog::error_s("Invalid continent identifier - empty!");
				return false;
			}

			ModifierValue values;
			memory::vector<ProvinceDefinition const*> prov_list;
			bool ret = NodeTools::expect_dictionary_keys_and_default(
				modifier_manager.expect_base_province_modifier(values),
				"provinces", ONE_EXACTLY, expect_list_reserve_length(prov_list, expect_province_definition_identifier(
					[&prov_list](ProvinceDefinition const& province) -> bool {
						if (province.continent == nullptr) {
							prov_list.emplace_back(&province);
						} else {
							spdlog::warn_s("Province {} found in multiple continents", province);
						}
						return true;
					}
				))
			)(node);

			if (!continents.emplace_item(
				identifier,
				identifier, std::move(values), Modifier::modifier_type_t::CONTINENT
			)) {
				return false;
			}

			Continent& continent = continents.back();
			continent.add_provinces(prov_list);
			continent.lock();

			for (ProvinceDefinition const* prov : continent.get_provinces()) {
				remove_province_definition_const(prov)->continent = &continent;
			}

			return ret;
		}
	)(root);

	lock_continents();

	return ret;
}
