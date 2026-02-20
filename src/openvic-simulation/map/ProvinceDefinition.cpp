#include "ProvinceDefinition.hpp"

#include <fmt/format.h>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

ProvinceDefinition::ProvinceDefinition(
	std::string_view new_identifier,
	colour_t new_colour,
	index_t new_index
) : HasIdentifierAndColour { new_identifier, new_colour, true },
	HasIndex { new_index } {}

memory::string ProvinceDefinition::to_string() const {
	return memory::fmt::format("(#{}, {}, 0x{})", get_province_number(), get_identifier(), get_colour());
}

bool ProvinceDefinition::load_positions(
	MapDefinition const& map_definition, BuildingTypeManager const& building_type_manager, ast::NodeCPtr root
) {
	const fixed_point_t map_height = map_definition.get_height();

	const bool ret = expect_dictionary_keys(
		"text_position", ZERO_OR_ONE,
			expect_fvec2(flip_y_callback(assign_variable_callback(positions.text_position), map_height)),
		"text_rotation", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(positions.text_rotation)),
		"text_scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(positions.text_scale)),

		"unit", ZERO_OR_ONE, expect_fvec2(flip_y_callback(assign_variable_callback(positions.unit), map_height)),
		"town", ZERO_OR_ONE, expect_fvec2(flip_y_callback(assign_variable_callback(positions.town), map_height)),
		"city", ZERO_OR_ONE, expect_fvec2(flip_y_callback(assign_variable_callback(positions.city), map_height)),
		"factory", ZERO_OR_ONE, expect_fvec2(flip_y_callback(assign_variable_callback(positions.factory), map_height)),

		"building_construction", ZERO_OR_ONE,
			expect_fvec2(flip_y_callback(assign_variable_callback(positions.building_construction), map_height)),
		"military_construction", ZERO_OR_ONE,
			expect_fvec2(flip_y_callback(assign_variable_callback(positions.military_construction), map_height)),

		"building_position", ZERO_OR_ONE, building_type_manager.expect_building_type_dictionary_reserve_length(
			positions.building_position,
			[this, map_height](BuildingType const& type, ast::NodeCPtr value) -> bool {
				return expect_fvec2(flip_y_callback(map_callback(positions.building_position, &type), map_height))(value);
			}
		),
		"building_rotation", ZERO_OR_ONE, building_type_manager.expect_building_type_decimal_map(
			move_variable_callback(positions.building_rotation), std::negate {}
		),

		/* the below are esoteric clausewitz leftovers that either have no impact or whose functionality is lost to time */
		"spawn_railway_track", ZERO_OR_ONE, success_callback,
		"railroad_visibility", ZERO_OR_ONE, success_callback,
		"building_nudge", ZERO_OR_ONE, success_callback
	)(root);

	if (coastal) {
		fvec2_t const* port_position = get_building_position(building_type_manager.get_port_building_type());
		if (port_position != nullptr) {
			const fixed_point_t rotation = get_building_rotation(building_type_manager.get_port_building_type());

			/* At 0 rotation the port faces west, as rotation increases the port rotates anti-clockwise. */
			const fvec2_t port_dir { -rotation.cos(), rotation.sin() };
			const ivec2_t port_facing_position = static_cast<ivec2_t>(*port_position + port_dir / 4);

			ProvinceDefinition const* province_ptr = map_definition.get_province_definition_at(port_facing_position);

			if (province_ptr != nullptr) {
				ProvinceDefinition const& province = *province_ptr;
				if (province.is_water() && is_adjacent_to(province)) {
					port = true;
					port_adjacent_province = province_ptr;
				} else {
					/* Expected provinces with invalid ports: 39, 296, 1047, 1406, 2044 */
					spdlog::warn_s(
						"Invalid port for province {}: facing province {} which has: water = {}, adjacent = {}",
						get_identifier(), province, province.is_water(), is_adjacent_to(province)
					);
				}
			} else {
				spdlog::warn_s("Invalid port for province {}: facing null province!", get_identifier());
			}
		}
	}

	return ret;
}

fvec2_t ProvinceDefinition::get_text_position() const {
	return positions.text_position.value_or(centre);
}

fixed_point_t ProvinceDefinition::get_text_rotation() const {
	return positions.text_rotation.value_or(0);
}

fixed_point_t ProvinceDefinition::get_text_scale() const {
	return positions.text_scale.value_or(1);
}

fvec2_t const* ProvinceDefinition::get_building_position(BuildingType const* building_type) const {
	if (building_type != nullptr) {
		const decltype(positions.building_position)::const_iterator it = positions.building_position.find(building_type);

		if (it != positions.building_position.end()) {
			return &it->second;
		}
	}
	return nullptr;
}

fixed_point_t ProvinceDefinition::get_building_rotation(BuildingType const* building_type) const {
	if (building_type != nullptr) {
		const decltype(positions.building_rotation)::const_iterator it = positions.building_rotation.find(building_type);

		if (it != positions.building_rotation.end()) {
			return it->second;
		}
	}
	return 0;
}

std::string_view ProvinceDefinition::adjacency_t::get_type_name(type_t type) {
	switch (type) {
	case type_t::LAND: return "Land";
	case type_t::WATER: return "Water";
	case type_t::COASTAL: return "Coastal";
	case type_t::IMPASSABLE: return "Impassable";
	case type_t::STRAIT: return "Strait";
	case type_t::CANAL: return "Canal";
	default: return "Invalid Adjacency Type";
	}
}

ProvinceDefinition::adjacency_t const* ProvinceDefinition::get_adjacency_to(ProvinceDefinition const& province) const {
	const memory::vector<adjacency_t>::const_iterator it = std::find_if(adjacencies.begin(), adjacencies.end(),
		[&province](adjacency_t const& adj) -> bool { return adj.get_to() == province; }
	);
	if (it != adjacencies.end()) {
		return &*it;
	} else {
		return nullptr;
	}
}

bool ProvinceDefinition::is_adjacent_to(ProvinceDefinition const& province) const {
	return std::any_of(adjacencies.begin(), adjacencies.end(),
		[&province](adjacency_t const& adj) -> bool { return adj.get_to() == province; }
	);
}

memory::vector<ProvinceDefinition::adjacency_t const*> ProvinceDefinition::get_adjacencies_going_through(
	ProvinceDefinition const& province
) const {
	memory::vector<adjacency_t const*> ret;
	for (adjacency_t const& adj : adjacencies) {
		if (adj.get_through() != nullptr && *adj.get_through() == province) {
			ret.push_back(&adj);
		}
	}
	return ret;
}

bool ProvinceDefinition::has_adjacency_going_through(ProvinceDefinition const& province) const {
	return std::any_of(adjacencies.begin(), adjacencies.end(),
		[&province](adjacency_t const& adj) -> bool {
			return adj.get_through() != nullptr && *adj.get_through() == province;
		}
	);
}

fvec2_t ProvinceDefinition::get_unit_position() const {
	return positions.unit.value_or(centre);
}

fvec2_t ProvinceDefinition::get_city_position() const {
	return positions.city.value_or(centre);
}

fvec2_t ProvinceDefinition::get_town_position() const {
	return positions.town.value_or(centre);
}

fvec2_t ProvinceDefinition::get_factory_position() const {
	return positions.factory.value_or(centre);
}

fvec2_t ProvinceDefinition::get_building_construction_position() const {
	return positions.building_construction.value_or(centre);
}

fvec2_t ProvinceDefinition::get_military_construction_position() const {
	return positions.military_construction.value_or(centre);
}
