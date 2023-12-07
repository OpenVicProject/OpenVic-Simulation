#include "Province.hpp"

#include "openvic-simulation/history/ProvinceHistory.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Province::Province(
	std::string_view new_identifier, colour_t new_colour, index_t new_index
) : HasIdentifierAndColour { new_identifier, new_colour, true, false }, index { new_index },
	region { nullptr }, on_map { false }, has_region { false }, water { false }, default_terrain_type { nullptr },
	terrain_type { nullptr }, life_rating { 0 }, colony_status { colony_status_t::STATE }, owner { nullptr },
	controller { nullptr }, slave { false }, crime { nullptr }, rgo { nullptr }, buildings { "buildings", false },
	total_population { 0 } {
	assert(index != NULL_INDEX);
}

std::string Province::to_string() const {
	std::stringstream stream;
	stream << "(#" << std::to_string(index) << ", " << get_identifier() << ", 0x" << colour_to_hex_string() << ")";
	return stream.str();
}

bool Province::load_positions(BuildingTypeManager const& building_type_manager, ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"text_position", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.text)),
		"text_rotation", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(positions.text_rotation)),
		"text_scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(positions.text_scale)),
		"unit", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.unit)),
		"town", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.city)),
		"city", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.city)),
		"factory", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.factory)),
		"building_construction", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.building_construction)),
		"military_construction", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.military_construction)),
		"building_position", ZERO_OR_ONE, building_type_manager.expect_building_type_dictionary(
			[this](BuildingType const& type, ast::NodeCPtr value) -> bool {
				return expect_fvec2([this, &type](fvec2_t position) -> bool {
					positions.building_position.emplace(&type, std::move(position));
					return true;
				})(value);
			}
		),
		"building_rotation", ZERO_OR_ONE, building_type_manager.expect_building_type_decimal_map(
			move_variable_callback(positions.building_rotation)
		),
		/* the below are esoteric clausewitz leftovers that either have no impact or whose functionality is lost to time */
		"spawn_railway_track", ZERO_OR_ONE, success_callback,
		"railroad_visibility", ZERO_OR_ONE, success_callback,
		"building_nudge", ZERO_OR_ONE, success_callback
	)(root);
}

bool Province::expand_building(std::string_view building_type_identifier) {
	BuildingInstance* building = buildings.get_item_by_identifier(building_type_identifier);
	if (building == nullptr) {
		return false;
	}
	return building->expand();
}

bool Province::load_pop_list(PopManager const& pop_manager, ast::NodeCPtr root) {
	return expect_dictionary_reserve_length(pops,
		[this, &pop_manager](std::string_view pop_type_identifier, ast::NodeCPtr pop_node) -> bool {
			return pop_manager.load_pop_into_province(*this, pop_type_identifier, pop_node);
		}
	)(root);
}

bool Province::add_pop(Pop&& pop) {
	if (!get_water()) {
		pops.push_back(std::move(pop));
		return true;
	} else {
		Logger::error("Trying to add pop to water province ", get_identifier());
		return false;
	}
}

size_t Province::get_pop_count() const {
	return pops.size();
}

/* REQUIREMENTS:
 * MAP-65, MAP-68, MAP-70, MAP-234
 */
void Province::update_pops() {
	total_population = 0;
	pop_type_distribution.clear();
	ideology_distribution.clear();
	culture_distribution.clear();
	religion_distribution.clear();
	for (Pop const& pop : pops) {
		total_population += pop.get_size();
		pop_type_distribution[&pop.get_type()] += pop.get_size();
		//ideology_distribution[&pop.get_???()] += pop.get_size();
		culture_distribution[&pop.get_culture()] += pop.get_size();
		religion_distribution[&pop.get_religion()] += pop.get_size();
	}
}

void Province::update_state(Date today) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.update_state(today);
	}
	update_pops();
}

void Province::tick(Date today) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.tick(today);
	}
}

Province::adjacency_t::adjacency_t(Province const* province, distance_t distance, flags_t flags)
	: province { province }, distance { distance }, flags { flags } {
	assert(province != nullptr);
}

bool Province::is_adjacent_to(Province const* province) {
	for (adjacency_t adj : adjacencies) {
		if (adj.province == province) {
			return true;
		}
	}
	return false;
}

bool Province::add_adjacency(Province const* province, distance_t distance, flags_t flags) {
	if (province == nullptr) {
		Logger::error("Tried to create null adjacency province for province ", get_identifier(), "!");
		return false;
	}
	if (is_adjacent_to(province)) {
		return false;
	}
	adjacencies.push_back({ province, distance, flags });
	return true;
}

bool Province::reset(BuildingTypeManager const& building_type_manager) {
	terrain_type = default_terrain_type;
	life_rating = 0;
	colony_status = colony_status_t::STATE;
	owner = nullptr;
	controller = nullptr;
	cores.clear();
	slave = false;
	rgo = nullptr;

	buildings.reset();
	bool ret = true;
	if (!get_water()) {
		if (building_type_manager.building_types_are_locked()) {
			for (BuildingType const& building_type : building_type_manager.get_building_types()) {
				if (building_type.get_in_province()) {
					ret &= buildings.add_item({ building_type });
				}
			}
		} else {
			Logger::error("Cannot generate buildings until building types are locked!");
			ret = false;
		}
	}
	lock_buildings();

	pops.clear();
	update_pops();

	return ret;
}

bool Province::apply_history_to_province(ProvinceHistoryEntry const* entry) {
	if (entry == nullptr) {
		Logger::error("Trying to apply null province history to ", get_identifier());
		return false;
	}
	if (entry->get_life_rating()) life_rating = *entry->get_life_rating();
	if (entry->get_colonial()) colony_status = *entry->get_colonial();
	if (entry->get_rgo()) rgo = *entry->get_rgo();
	if (entry->get_terrain_type()) terrain_type = *entry->get_terrain_type();
	if (entry->get_owner()) owner = *entry->get_owner();
	if (entry->get_controller()) controller = *entry->get_controller();
	if (entry->get_slave()) slave = *entry->get_slave();
	for (Country const* core : entry->get_remove_cores()) {
		const typename decltype(cores)::iterator existing_core = std::find(cores.begin(), cores.end(), core);
		if (existing_core != cores.end()) {
			cores.erase(existing_core);
		} else {
			Logger::warning(
				"Trying to remove non-existent core ", core->get_identifier(), " from province ", get_identifier()
			);
		}
	}
	for (Country const* core : entry->get_add_cores()) {
		const typename decltype(cores)::iterator existing_core = std::find(cores.begin(), cores.end(), core);
		if (existing_core == cores.end()) {
			cores.push_back(core);
		} else {
			Logger::warning(
				"Trying to add already-existing core ", core->get_identifier(), " to province ", get_identifier()
			);
		}
	}
	bool ret = true;
	for (auto const& [building, level] : entry->get_province_buildings()) {
		BuildingInstance* existing_entry = buildings.get_item_by_identifier(building->get_identifier());
		if (existing_entry != nullptr) {
			existing_entry->set_level(level);
		} else {
			Logger::error(
				"Trying to set level of non-existent province building ", building->get_identifier(), " to ", level,
				" in province ", get_identifier()
			);
			ret = false;
		}
	}
	// TODO: load state buildings
	// TODO: party loyalties for each POP when implemented on POP side#
	return ret;
}
