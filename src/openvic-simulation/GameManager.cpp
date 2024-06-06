#include "GameManager.hpp"

using namespace OpenVic;

GameManager::GameManager(
	gamestate_updated_func_t gamestate_updated_callback, SimulationClock::state_changed_function_t clock_state_changed_callback
) : simulation_clock {
		std::bind(&GameManager::tick, this), std::bind(&GameManager::update_gamestate, this), clock_state_changed_callback
	}, gamestate_updated { gamestate_updated_callback ? std::move(gamestate_updated_callback) : []() {} },
	session_start { 0 }, today {}, gamestate_needs_update { false }, currently_updating_gamestate { false },
	map_instance { map_definition } {}

void GameManager::set_gamestate_needs_update() {
	if (!currently_updating_gamestate) {
		gamestate_needs_update = true;
	} else {
		Logger::error("Attempted to queue a gamestate update already updating the gamestate!");
	}
}

void GameManager::update_gamestate() {
	if (!gamestate_needs_update) {
		return;
	}
	currently_updating_gamestate = true;
	Logger::info("Update: ", today);
	map_instance.update_gamestate(today);
	gamestate_updated();
	gamestate_needs_update = false;
	currently_updating_gamestate = false;
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void GameManager::tick() {
	today++;
	Logger::info("Tick: ", today);
	map_instance.tick(today);
	set_gamestate_needs_update();
}

bool GameManager::reset() {
	session_start = time(nullptr);
	simulation_clock.reset();
	today = {};
	economy_manager.get_good_manager().reset_to_defaults();
	bool ret = map_instance.reset(economy_manager.get_building_type_manager());
	set_gamestate_needs_update();
	return ret;
}

bool GameManager::load_bookmark(Bookmark const* new_bookmark) {
	bool ret = reset();

	bookmark = new_bookmark;
	if (bookmark == nullptr) {
		Logger::error("Cannot load bookmark - null!");
		return ret;
	}

	Logger::info("Loading bookmark ", bookmark->get_name(), " with start date ", bookmark->get_date());

	if (!define_manager.in_game_period(bookmark->get_date())) {
		Logger::warning("Bookmark date ", bookmark->get_date(), " is not in the game's time period!");
	}

	today = bookmark->get_date();

	ret &= map_instance.apply_history_to_provinces(
		history_manager.get_province_manager(), today, politics_manager.get_ideology_manager(),
		politics_manager.get_issue_manager(), *country_manager.get_country_by_identifier("ENG")
	);
	ret &= map_instance.get_state_manager().generate_states(map_instance);

	ret &= country_instance_manager.generate_country_instances(country_manager);
	ret &= country_instance_manager.apply_history_to_countries(
		history_manager.get_country_manager(), today, military_manager.get_unit_instance_manager(), map_instance
	);

	return ret;
}

bool GameManager::expand_selected_province_building(size_t building_index) {
	set_gamestate_needs_update();
	ProvinceInstance* province = map_instance.get_selected_province();
	if (province == nullptr) {
		Logger::error("Cannot expand building index ", building_index, " - no province selected!");
		return false;
	}
	if (building_index < 0) {
		Logger::error("Invalid building index ", building_index, " while trying to expand in province ", province);
		return false;
	}
	return province->expand_building(building_index);
}
