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

bool Province::add_pop(Pop&& pop) {
	if (!is_water()) {
		pops.push_back(std::move(pop));
		return true;
	} else {
		Logger::error("Trying to add pop to water province ", get_identifier());
		return false;
	}
}

bool Province::add_pop_vec(std::vector<Pop> const& pop_vec) {
	if (!is_water()) {
		pops.reserve(pops.size() + pop_vec.size());
		for (Pop const& pop : pop_vec) {
			pops.push_back(pop);
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

Province::adjacency_t* Province::get_adjacency_to(Province const* province) {
	const std::vector<adjacency_t>::iterator it = std::find_if(adjacencies.begin(), adjacencies.end(),
		[province](adjacency_t const& adj) -> bool { return adj.get_to() == province; }
	);
	if (it != adjacencies.end()) {
		return &*it;
	} else {
		return nullptr;
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

/* This is called for all adjacent pixel pairs and returns whether or not a new adjacency was add,
 * hence the lack of error messages in the false return cases. */
bool Province::add_standard_adjacency(Province& from, Province& to) {
	if (from == to) {
		return false;
	}

	const bool from_needs_adjacency = !from.is_adjacent_to(&to);
	const bool to_needs_adjacency = !to.is_adjacent_to(&from);

	if (!from_needs_adjacency && !to_needs_adjacency) {
		return false;
	}

	const distance_t distance = calculate_distance_between(from, to);

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

bool Province::add_special_adjacency(
	Province& from, Province& to, adjacency_t::type_t type, Province const* through, adjacency_t::data_t data
) {
	if (from == to) {
		Logger::error("Trying to add ", adjacency_t::get_type_name(type), " adjacency from province ", from, " to itself!");
		return false;
	}

	using enum adjacency_t::type_t;

	/* Check end points */
	switch (type) {
	case LAND:
	case IMPASSABLE:
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

	const distance_t distance = calculate_distance_between(from, to);

	const auto add_adjacency = [distance, type, through, data](Province& from, Province const& to) -> bool {
		adjacency_t* existing_adjacency = from.get_adjacency_to(&to);
		if (existing_adjacency != nullptr) {
			if (type == existing_adjacency->get_type()) {
				Logger::warning(
					"Adjacency from ", from, " to ", to, " already has type ", adjacency_t::get_type_name(type), "!"
				);
				if (type != STRAIT && type != CANAL) {
					/* Straits and canals might change through or data, otherwise we can exit early */
					return true;
				}
			}
			if (type != IMPASSABLE && type != STRAIT && type != CANAL) {
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

fvec2_t Province::get_unit_position() const {
	return positions.unit.value_or(positions.centre);
}

Province::distance_t Province::calculate_distance_between(Province const& from, Province const& to) {
	const fvec2_t distance_vector = to.get_unit_position() - from.get_unit_position();
	return distance_vector.length_squared(); // TODO - replace with length using deterministic fixed point square root
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
