#include "InstanceManager.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

InstanceManager::InstanceManager(
#if OV_MODIFIER_CALCULATION_TEST
	bool new_ADD_OWNER_CONTRIBUTION,
#endif
	DefinitionManager const& new_definition_manager, gamestate_updated_func_t gamestate_updated_callback,
	SimulationClock::state_changed_function_t clock_state_changed_callback
) : definition_manager { new_definition_manager },
#if OV_MODIFIER_CALCULATION_TEST
	ADD_OWNER_CONTRIBUTION { new_ADD_OWNER_CONTRIBUTION },
#endif
	map_instance { new_definition_manager.get_map_definition() },
	simulation_clock {
		std::bind(&InstanceManager::tick, this), std::bind(&InstanceManager::update_gamestate, this),
		clock_state_changed_callback ? std::move(clock_state_changed_callback) : []() {}
	},
	game_instance_setup { false },
	game_session_started { false },
	session_start { 0 },
	bookmark { nullptr },
	today {},
	gamestate_updated { gamestate_updated_callback ? std::move(gamestate_updated_callback) : []() {} },
	gamestate_needs_update { false },
	currently_updating_gamestate { false } {}

void InstanceManager::set_gamestate_needs_update() {
	if (!currently_updating_gamestate) {
		gamestate_needs_update = true;
	} else {
		Logger::error("Attempted to queue a gamestate update already updating the gamestate!");
	}
}

void InstanceManager::update_gamestate() {
	if (!gamestate_needs_update) {
		return;
	}
	currently_updating_gamestate = true;

	Logger::info("Update: ", today);

#if OV_MODIFIER_CALCULATION_TEST
	if (ADD_OWNER_CONTRIBUTION) {
#else
	if constexpr (ProvinceInstance::ADD_OWNER_CONTRIBUTION) {
#endif
		// Calculate local province modifier sums first, then national country modifier sums, then loop over owned provinces
		// adding their contributions to the owner country's modifier sum and loop over them again to add the country's total
		// (including province contributions) to the provinces' modifier sum. This results in every country and province
		// having a full copy of all the modifiers affecting them in their modifier sum.
		map_instance.update_modifier_sums(
			today, definition_manager.get_modifier_manager().get_static_modifier_cache()
		);
		country_instance_manager.update_modifier_sums(
			today, definition_manager.get_modifier_manager().get_static_modifier_cache()
		);
	} else {
		// Calculate national country modifier sums first, then local province modifier sums, adding province contributions
		// to owner countries' modifier sums if each province has an owner. This results in every country having a full copy
		// of all the modifiers affecting them in their modifier sum, but provinces only having their directly/locally applied
		// modifiers in their modifier sum, hence requiring both province and owner country modifier effect values to be looked
		// up and added together to get the full effect on the province.
		country_instance_manager.update_modifier_sums(
			today, definition_manager.get_modifier_manager().get_static_modifier_cache()
		);
		map_instance.update_modifier_sums(
			today, definition_manager.get_modifier_manager().get_static_modifier_cache()
		);
	}

	// Update gamestate...
	map_instance.update_gamestate(today, definition_manager.get_define_manager());
	country_instance_manager.update_gamestate(
		today, definition_manager.get_define_manager(), definition_manager.get_military_manager().get_unit_type_manager(),
		definition_manager.get_modifier_manager().get_modifier_effect_cache()
	);

	gamestate_updated();
	gamestate_needs_update = false;

	currently_updating_gamestate = false;
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void InstanceManager::tick() {
	today++;

	Logger::info("Tick: ", today);

	// Tick...
	map_instance.tick(today);

	set_gamestate_needs_update();
}

bool InstanceManager::setup() {
	if (is_game_instance_setup()) {
		Logger::error("Cannot setup game instance - already set up!");
		return false;
	}

	bool ret = good_instance_manager.setup(definition_manager.get_economy_manager().get_good_definition_manager());
	ret &= map_instance.setup(
#if OV_MODIFIER_CALCULATION_TEST
		ADD_OWNER_CONTRIBUTION,
#endif
		definition_manager.get_economy_manager().get_building_type_manager(),
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies()
	);
	ret &= country_instance_manager.generate_country_instances(
#if OV_MODIFIER_CALCULATION_TEST
		ADD_OWNER_CONTRIBUTION,
#endif
		definition_manager.get_country_definition_manager(),
		definition_manager.get_economy_manager().get_building_type_manager().get_building_types(),
		definition_manager.get_research_manager().get_technology_manager().get_technologies(),
		definition_manager.get_research_manager().get_invention_manager().get_inventions(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies(),
		definition_manager.get_politics_manager().get_issue_manager().get_reform_groups(),
		definition_manager.get_politics_manager().get_government_type_manager().get_government_types(),
		definition_manager.get_crime_manager().get_crime_modifiers(),
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_military_manager().get_unit_type_manager().get_regiment_types(),
		definition_manager.get_military_manager().get_unit_type_manager().get_ship_types()
	);

	game_instance_setup = true;

	return ret;
}

bool InstanceManager::load_bookmark(Bookmark const* new_bookmark) {
	if (is_bookmark_loaded()) {
		Logger::error("Cannot load bookmark - already loaded!");
		return false;
	}

	if (!is_game_instance_setup()) {
		Logger::error("Cannot load bookmark - game instance not set up!");
		return false;
	}

	if (new_bookmark == nullptr) {
		Logger::error("Cannot load bookmark - null!");
		return false;
	}

	bookmark = new_bookmark;

	Logger::info("Loading bookmark ", bookmark->get_name(), " with start date ", bookmark->get_date());

	if (!definition_manager.get_define_manager().in_game_period(bookmark->get_date())) {
		Logger::warning("Bookmark date ", bookmark->get_date(), " is not in the game's time period!");
	}

	today = bookmark->get_date();

	bool ret = map_instance.apply_history_to_provinces(
		definition_manager.get_history_manager().get_province_manager(), today,
		country_instance_manager,
		// TODO - the following argument is for generating test pop attributes
		definition_manager.get_politics_manager().get_issue_manager()
	);

	ret &= country_instance_manager.apply_history_to_countries(
		definition_manager.get_history_manager().get_country_manager(), today, unit_instance_manager, map_instance
	);

	ret &= map_instance.get_state_manager().generate_states(
		map_instance, definition_manager.get_pop_manager().get_pop_types()
	);

	return ret;
}

bool InstanceManager::start_game_session() {
	if (is_game_session_started()) {
		Logger::error("Cannot start game session - already started!");
		return false;
	}

	session_start = time(nullptr);
	simulation_clock.reset();
	set_gamestate_needs_update();

	game_session_started = true;

	return true;
}

bool InstanceManager::update_clock() {
	if (!is_game_session_started()) {
		Logger::error("Cannot update clock - game session not started!");
		return false;
	}

	simulation_clock.conditionally_advance_game();
	return true;
}

bool InstanceManager::expand_selected_province_building(size_t building_index) {
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
