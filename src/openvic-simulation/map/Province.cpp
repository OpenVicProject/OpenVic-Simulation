#include "Province.hpp"

#include "openvic-simulation/history/ProvinceHistory.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Province::Province(
	std::string_view new_identifier, colour_t new_colour, index_t new_index
) : HasIdentifierAndColour { new_identifier, new_colour, true }, index { new_index }, region { nullptr },
	climate { nullptr }, continent { nullptr }, on_map { false }, has_region { false }, water { false }, coastal { false },
	port { false }, default_terrain_type { nullptr }, positions {}, terrain_type { nullptr }, life_rating { 0 },
	colony_status { colony_status_t::STATE }, state { nullptr }, owner { nullptr }, controller { nullptr }, slave { false },
	crime { nullptr }, rgo { nullptr }, buildings { "buildings", false }, total_population { 0 } {
	assert(index != NULL_INDEX);
}

bool Province::operator==(Province const& other) const {
	return this == &other;
}

std::string Province::to_string() const {
	std::stringstream stream;
	stream << "(#" << std::to_string(index) << ", " << get_identifier() << ", 0x" << get_colour() << ")";
	return stream.str();
}

bool Province::load_positions(BuildingTypeManager const& building_type_manager, ast::NodeCPtr root) {
	const bool ret = expect_dictionary_keys(
		"text_position", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.text)),
		"text_rotation", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(positions.text_rotation)),
		"text_scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(positions.text_scale)),
		"unit", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.unit)),
		"town", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.city)),
		"city", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.city)),
		"factory", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.factory)),
		"building_construction", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.building_construction)),
		"military_construction", ZERO_OR_ONE, expect_fvec2(assign_variable_callback(positions.military_construction)),
		"building_position", ZERO_OR_ONE, building_type_manager.expect_building_type_dictionary_reserve_length(
			positions.building_position,
			[this](BuildingType const& type, ast::NodeCPtr value) -> bool {
				return expect_fvec2(map_callback(positions.building_position, &type))(value);
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

	port = coastal && positions.building_position.contains(building_type_manager.get_port_building_type());

	return ret;
}

bool Province::expand_building(size_t building_index) {
	BuildingInstance* building = buildings.get_item_by_index(building_index);
	if (building == nullptr) {
		Logger::error("Trying to expand non-existent building index ", building_index, " in province ", get_identifier());
		return false;
	}
	return building->expand();
}

void Province::_add_pop(Pop pop) {
	pop.set_location(this);
	pops.push_back(std::move(pop));
}

bool Province::add_pop(Pop&& pop) {
	if (!is_water()) {
		_add_pop(std::move(pop));
		return true;
	} else {
		Logger::error("Trying to add pop to water province ", get_identifier());
		return false;
	}
}

bool Province::add_pop_vec(std::vector<Pop> const& pop_vec) {
	if (!is_water()) {
		reserve_more(pops, pop_vec.size());
		for (Pop const& pop : pop_vec) {
			_add_pop(pop);
		}
		return true;
	} else {
		Logger::error("Trying to add pop vector to water province ", get_identifier());
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
		ideology_distribution += pop.get_ideologies();
		culture_distribution[&pop.get_culture()] += pop.get_size();
		religion_distribution[&pop.get_religion()] += pop.get_size();
	}
}

void Province::update_gamestate(Date today) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.update_gamestate(today);
	}
	update_pops();
}

void Province::tick(Date today) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.tick(today);
	}
}

Province::adjacency_t::adjacency_t(
	Province const* new_to, distance_t new_distance, type_t new_type, Province const* new_through, data_t new_data
) : to { new_to }, distance { new_distance }, type { new_type }, through { new_through }, data { new_data } {}

std::string_view Province::adjacency_t::get_type_name(type_t type) {
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

Province::adjacency_t const* Province::get_adjacency_to(Province const* province) const {
	const std::vector<adjacency_t>::const_iterator it = std::find_if(adjacencies.begin(), adjacencies.end(),
		[province](adjacency_t const& adj) -> bool { return adj.get_to() == province; }
	);
	if (it != adjacencies.end()) {
		return &*it;
	} else {
		return nullptr;
	}
}

bool Province::is_adjacent_to(Province const* province) const {
	return province != nullptr && std::any_of(adjacencies.begin(), adjacencies.end(),
		[province](adjacency_t const& adj) -> bool { return adj.get_to() == province; }
	);
}

std::vector<Province::adjacency_t const*> Province::get_adjacencies_going_through(Province const* province) const {
	std::vector<adjacency_t const*> ret;
	for (adjacency_t const& adj : adjacencies) {
		if (adj.get_through() == province) {
			ret.push_back(&adj);
		}
	}
	return ret;
}

bool Province::has_adjacency_going_through(Province const* province) const {
	return province != nullptr && std::any_of(adjacencies.begin(), adjacencies.end(),
		[province](adjacency_t const& adj) -> bool { return adj.get_through() == province; }
	);
}

fvec2_t Province::get_unit_position() const {
	return positions.unit.value_or(positions.centre);
}

bool Province::reset(BuildingTypeManager const& building_type_manager) {
	terrain_type = default_terrain_type;
	life_rating = 0;
	colony_status = colony_status_t::STATE;
	state = nullptr;
	owner = nullptr;
	controller = nullptr;
	cores.clear();
	slave = false;
	crime = nullptr;
	rgo = nullptr;

	buildings.reset();
	bool ret = true;
	if (!is_water()) {
		if (building_type_manager.building_types_are_locked()) {
			for (BuildingType const* building_type : building_type_manager.get_province_building_types()) {
				ret &= buildings.add_item({ *building_type });
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
	// TODO: party loyalties for each POP when implemented on POP side
	return ret;
}

void Province::setup_pop_test_values(
	IdeologyManager const& ideology_manager, IssueManager const& issue_manager, Country const& country
) {
	for (Pop& pop : pops) {
		pop.setup_pop_test_values(ideology_manager, issue_manager, country);
	}
}
