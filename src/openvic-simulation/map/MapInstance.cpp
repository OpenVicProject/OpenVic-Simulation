#include "MapInstance.hpp"

#include "openvic-simulation/history/ProvinceHistory.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MapInstance::MapInstance(MapDefinition const& new_map_definition)
  : map_definition { new_map_definition }, selected_province { nullptr }, highest_province_population { 0 },
	total_map_population { 0 } {}

ProvinceInstance& MapInstance::get_province_instance_from_definition(ProvinceDefinition const& province) {
	return province_instances.get_items()[province.get_index() - 1];
}

ProvinceInstance const& MapInstance::get_province_instance_from_definition(ProvinceDefinition const& province) const {
	return province_instances.get_items()[province.get_index() - 1];
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

bool MapInstance::setup(
#if OV_MODIFIER_CALCULATION_TEST
	bool ADD_OWNER_CONTRIBUTION,
#endif
	BuildingTypeManager const& building_type_manager,
	decltype(ProvinceInstance::pop_type_distribution)::keys_t const& pop_type_keys,
	decltype(ProvinceInstance::ideology_distribution)::keys_t const& ideology_keys
) {
	if (province_instances_are_locked()) {
		Logger::error("Cannot setup map - province instances are locked!");
		return false;
	}
	if (!map_definition.province_definitions_are_locked()) {
		Logger::error("Cannot setup map - province consts are not locked!");
		return false;
	}

	bool ret = true;

	province_instances.reserve(map_definition.get_province_definition_count());

	for (ProvinceDefinition const& province : map_definition.get_province_definitions()) {
		ret &= province_instances.add_item({
#if OV_MODIFIER_CALCULATION_TEST
			ADD_OWNER_CONTRIBUTION,
#endif
			province, pop_type_keys, ideology_keys
		});
	}

	province_instances.lock();

	for (ProvinceInstance& province : province_instances.get_items()) {
		ret &= province.setup(building_type_manager);
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
	ProvinceHistoryManager const& history_manager, Date date, CountryInstanceManager& country_manager,
	IssueManager const& issue_manager
) {
	bool ret = true;

	for (ProvinceInstance& province : province_instances.get_items()) {
		ProvinceDefinition const& province_definition = province.get_province_definition();
		if (!province_definition.is_water()) {
			ProvinceHistoryMap const* history_map = history_manager.get_province_history(&province_definition);

			if (history_map != nullptr) {
				ProvinceHistoryEntry const* pop_history_entry = nullptr;

				for (auto const& [entry_date, entry] : history_map->get_entries()) {
					if (entry_date > date) {
						break;
					}

					province.apply_history_to_province(*entry, country_manager);

					if (!entry->get_pops().empty()) {
						pop_history_entry = entry.get();
					}
				}

				if (pop_history_entry != nullptr) {
					province.add_pop_vec(pop_history_entry->get_pops());

					province.setup_pop_test_values(issue_manager);
				}
			}
		}
	}

	return ret;
}

void MapInstance::update_modifier_sums(Date today, StaticModifierCache const& static_modifier_cache) {
	for (ProvinceInstance& province : province_instances.get_items()) {
		province.update_modifier_sum(today, static_modifier_cache);
	}
}

void MapInstance::update_gamestate(Date today, DefineManager const& define_manager) {
	for (ProvinceInstance& province : province_instances.get_items()) {
		province.update_gamestate(today, define_manager);
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
