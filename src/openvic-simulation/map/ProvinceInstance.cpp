#include "ProvinceInstance.hpp"

#include "openvic-simulation/country/Country.hpp"
#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/military/UnitInstance.hpp"

using namespace OpenVic;

ProvinceInstance::ProvinceInstance(ProvinceDefinition const& new_province_definition)
  : HasIdentifierAndColour { new_province_definition },
	province_definition { new_province_definition },
	terrain_type { nullptr },
	life_rating { 0 },
	colony_status { colony_status_t::STATE },
	state { nullptr },
	owner { nullptr },
	controller { nullptr },
	cores {},
	slave { false },
	crime { nullptr },
	rgo { nullptr },
	buildings { "buildings", false },
	armies {},
	navies {},
	pops {},
	total_population { 0 },
	pop_type_distribution {},
	ideology_distribution {},
	culture_distribution {},
	religion_distribution {} {}

bool ProvinceInstance::expand_building(size_t building_index) {
	BuildingInstance* building = buildings.get_item_by_index(building_index);
	if (building == nullptr) {
		Logger::error("Trying to expand non-existent building index ", building_index, " in province ", get_identifier());
		return false;
	}
	return building->expand();
}

void ProvinceInstance::_add_pop(Pop pop) {
	pop.set_location(this);
	pops.push_back(std::move(pop));
}

bool ProvinceInstance::add_pop(Pop&& pop) {
	if (!province_definition.is_water()) {
		_add_pop(std::move(pop));
		return true;
	} else {
		Logger::error("Trying to add pop to water province ", get_identifier());
		return false;
	}
}

bool ProvinceInstance::add_pop_vec(std::vector<Pop> const& pop_vec) {
	if (!province_definition.is_water()) {
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

size_t ProvinceInstance::get_pop_count() const {
	return pops.size();
}

/* REQUIREMENTS:
 * MAP-65, MAP-68, MAP-70, MAP-234
 */
void ProvinceInstance::_update_pops() {
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

void ProvinceInstance::update_gamestate(Date today) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.update_gamestate(today);
	}
	_update_pops();
}

void ProvinceInstance::tick(Date today) {
	for (BuildingInstance& building : buildings.get_items()) {
		building.tick(today);
	}
}

bool ProvinceInstance::add_army(ArmyInstance& army) {
	if (armies.emplace(&army).second) {
		return true;
	} else {
		Logger::error("Trying to add already-existing army ", army.get_name(), " to province ", get_identifier());
		return false;
	}
}

bool ProvinceInstance::remove_army(ArmyInstance& army) {
	if (armies.erase(&army) > 0) {
		return true;
	} else {
		Logger::error("Trying to remove non-existent army ", army.get_name(), " from province ", get_identifier());
		return false;
	}
}

bool ProvinceInstance::add_navy(NavyInstance& navy) {
	if (navies.emplace(&navy).second) {
		return true;
	} else {
		Logger::error("Trying to add already-existing navy ", navy.get_name(), " to province ", get_identifier());
		return false;
	}
}

bool ProvinceInstance::remove_navy(NavyInstance& navy) {
	if (navies.erase(&navy) > 0) {
		return true;
	} else {
		Logger::error("Trying to remove non-existent navy ", navy.get_name(), " from province ", get_identifier());
		return false;
	}
}

bool ProvinceInstance::reset(BuildingTypeManager const& building_type_manager) {
	terrain_type = province_definition.get_default_terrain_type();
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
	if (!province_definition.is_water()) {
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
	_update_pops();

	return ret;
}

bool ProvinceInstance::apply_history_to_province(ProvinceHistoryEntry const* entry) {
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

void ProvinceInstance::setup_pop_test_values(
	IdeologyManager const& ideology_manager, IssueManager const& issue_manager, Country const& country
) {
	for (Pop& pop : pops) {
		pop.setup_pop_test_values(ideology_manager, issue_manager, country);
	}
}
