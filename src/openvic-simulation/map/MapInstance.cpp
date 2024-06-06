#include "MapInstance.hpp"

#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MapInstance::MapInstance(MapDefinition const& new_map_definition)
  : map_definition { new_map_definition }, selected_province { nullptr }, highest_province_population { 0 },
	total_map_population { 0 } {}

ProvinceInstance* MapInstance::get_province_instance_from_const(ProvinceDefinition const* province) {
	if (province != nullptr) {
		return get_province_instance_by_index(province->get_index());
	} else {
		return nullptr;
	}
}

ProvinceInstance const* MapInstance::get_province_instance_from_const(ProvinceDefinition const* province) const {
	if (province != nullptr) {
		return get_province_instance_by_index(province->get_index());
	} else {
		return nullptr;
	}
}

void MapInstance::set_selected_province(ProvinceDefinition::index_t index) {
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

ProvinceInstance* MapInstance::get_selected_province() {
	return selected_province;
}

ProvinceDefinition::index_t MapInstance::get_selected_province_index() const {
	return selected_province != nullptr ? selected_province->get_province_definition().get_index()
		: ProvinceDefinition::NULL_INDEX;
}

bool MapInstance::reset(BuildingTypeManager const& building_type_manager) {
	if (!map_definition.province_definitions_are_locked()) {
		Logger::error("Cannot reset map - province consts are not locked!");
		return false;
	}

	bool ret = true;

	// TODO - ensure all references to old ProvinceInstances are safely cleared
	state_manager.reset();
	selected_province = nullptr;

	province_instances.reset();

	province_instances.reserve(map_definition.get_province_definition_count());

	for (ProvinceDefinition const& province : map_definition.get_province_definitions()) {
		ret &= province_instances.add_item({ province });
	}

	province_instances.lock();

	for (ProvinceInstance& province : province_instances.get_items()) {
		ret &= province.reset(building_type_manager);
	}

	if (get_province_instance_count() != map_definition.get_province_definition_count()) {
		Logger::error(
			"ProvinceInstance count (", get_province_instance_count(), ") does not match ProvinceDefinition count (",
			map_definition.get_province_definition_count(), ")!"
		);
		return false;
	}

	return ret;
}

bool MapInstance::apply_history_to_provinces(
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

void MapInstance::update_gamestate(Date today) {
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

void MapInstance::tick(Date today) {
	for (ProvinceInstance& province : province_instances.get_items()) {
		province.tick(today);
	}
}
