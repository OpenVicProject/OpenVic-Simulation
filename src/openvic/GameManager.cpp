#include "GameManager.hpp"

#include "utility/Logger.hpp"

using namespace OpenVic;

GameManager::GameManager(state_updated_func_t state_updated_callback)
	: clock { [this]() { tick(); }, [this]() { update_state(); } },
	  state_updated { state_updated_callback } {}

void GameManager::set_needs_update() {
	needs_update = true;
}

void GameManager::update_state() {
	if (needs_update) {
		Logger::info("Update: ", today);
		map.update_state(today);
		if (state_updated) state_updated();
		needs_update = false;
	}
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void GameManager::tick() {
	today++;
	Logger::info("Tick: ", today);
	map.tick(today);
	set_needs_update();
}

return_t GameManager::setup() {
	session_start = time(nullptr);
	clock.reset();
	today = { 1836 };
	good_manager.reset_to_defaults();
	return_t ret = map.setup(good_manager, building_manager);
	set_needs_update();
	return ret;
}

Date const& GameManager::get_today() const {
	return today;
}

return_t GameManager::expand_building(index_t province_index, std::string const& building_type_identifier) {
	set_needs_update();
	Province* province = map.get_province_by_index(province_index);
	if (province == nullptr) return FAILURE;
	return province->expand_building(building_type_identifier);
}
