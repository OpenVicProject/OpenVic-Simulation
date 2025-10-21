#include "InstanceManager.hpp"

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/console/ConsoleInstance.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

InstanceManager::InstanceManager(
	GameRulesManager const& new_game_rules_manager,
	DefinitionManager const& new_definition_manager,
	gamestate_updated_func_t gamestate_updated_callback
) : thread_pool { today },
	definition_manager { new_definition_manager },
	game_action_manager { *this },
	game_rules_manager { new_game_rules_manager },
	good_instance_manager {
		new_definition_manager.get_economy_manager().get_good_definition_manager(),
		new_game_rules_manager
	},
	market_instance {
		thread_pool,
		new_definition_manager.get_define_manager().get_country_defines(),
		good_instance_manager
	},
	artisanal_producer_factory_pattern {
		good_instance_manager,
		new_definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		new_definition_manager.get_economy_manager().get_production_type_manager()
	},
	global_flags { "global" },
	country_instance_manager {
		new_definition_manager.get_country_definition_manager(),
		new_definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		new_definition_manager.get_define_manager().get_country_defines(),
		new_definition_manager.get_define_manager().get_pops_defines(),
		definition_manager.get_economy_manager().get_building_type_manager().get_building_types(),
		definition_manager.get_research_manager().get_technology_manager().get_technologies(),
		definition_manager.get_research_manager().get_invention_manager().get_inventions(),
		definition_manager.get_politics_manager().get_issue_manager().get_reform_groups(),
		definition_manager.get_politics_manager().get_government_type_manager().get_government_types(),
		definition_manager.get_crime_manager().get_crime_modifiers(),
		good_instance_manager.get_good_instances(),
		definition_manager.get_military_manager().get_unit_type_manager().get_regiment_types(),
		definition_manager.get_military_manager().get_unit_type_manager().get_ship_types(),
		definition_manager.get_pop_manager().get_stratas(),
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies(),
		new_game_rules_manager,
		country_relation_manager,
		good_instance_manager,
		definition_manager.get_define_manager().get_economy_defines()
	},
	unit_instance_manager {
		new_definition_manager.get_pop_manager().get_culture_manager(),
		new_definition_manager.get_military_manager().get_leader_trait_manager(),
		new_definition_manager.get_define_manager().get_military_defines()
	},
	politics_instance_manager { *this },
	map_instance {
		new_definition_manager.get_map_definition(),
		definition_manager.get_economy_manager().get_building_type_manager(),
		new_game_rules_manager,
		definition_manager.get_modifier_manager().get_modifier_effect_cache(),
		market_instance,
		thread_pool,
		definition_manager.get_pop_manager().get_stratas(),
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies()
	},
	simulation_clock {
		[this]() -> void {
			queue_game_action(game_action_type_t::GAME_ACTION_TICK, {});
		},
		[this]() -> void {
			execute_game_actions();
			update_gamestate();
		}
	},
	console_instance { *this },
	gamestate_updated { gamestate_updated_callback ? std::move(gamestate_updated_callback) : []() {} } {}

void InstanceManager::set_gamestate_needs_update() {
	if (!currently_updating_gamestate) {
		gamestate_needs_update = true;
	} else {
		spdlog::error_s("Attempted to queue a gamestate update already updating the gamestate!");
	}
}

void InstanceManager::update_gamestate() {
	if (currently_updating_gamestate) {
		spdlog::error_s("Attempted to update gamestate while already updating gamestate!");
		return;
	}

	if (!gamestate_needs_update) {
		return;
	}

	currently_updating_gamestate = true;

	SPDLOG_INFO("Update: {}", today);

	update_modifier_sums();

	// Update gamestate...
	map_instance.update_gamestate(*this);
	country_instance_manager.update_gamestate(*this);
	unit_instance_manager.update_gamestate();

	gamestate_updated();
	gamestate_needs_update = false;

	currently_updating_gamestate = false;
}

/* REQUIREMENTS:
 * SS-98, SS-101
 */
void InstanceManager::tick() {

	today++;

	SPDLOG_INFO("Tick: {}", today);

	// Tick...
	country_instance_manager.country_manager_tick_before_map(*this);
	map_instance.map_tick();
	market_instance.execute_orders();
	country_instance_manager.country_manager_tick_after_map(*this);
	unit_instance_manager.tick();

	if (today.is_month_start()) {
		market_instance.record_price_history();
	}
}

void InstanceManager::execute_game_actions() {
	if (currently_executing_game_actions) {
		spdlog::error_s("Attempted to execute game actions while already executing game actions!");
		return;
	}

	if (game_action_queue.empty()) {
		return;
	}

	currently_executing_game_actions = true;

	// Temporary gamestate/UI update trigger, to be replaced with a more sophisticated targeted system
	bool needs_gamestate_update = false;

	for (game_action_t const& game_action : game_action_queue) {
		needs_gamestate_update |= game_action_manager.execute_game_action(game_action);
	}

	game_action_queue.clear();

	if (needs_gamestate_update) {
		set_gamestate_needs_update();
	}

	currently_executing_game_actions = false;
}

bool InstanceManager::setup() {
	if (is_game_instance_setup()) {
		spdlog::error_s("Cannot setup game instance - already set up!");
		return false;
	}

	thread_pool.initialise_threadpool(
		definition_manager.get_define_manager().get_pops_defines(),
		country_instance_manager.get_country_instances(),
		good_instance_manager.get_good_definition_manager().get_good_definitions(),
		definition_manager.get_pop_manager().get_stratas(),
		good_instance_manager.get_good_instances(),
		map_instance.get_province_instances()
	);

	game_instance_setup = true;

	return true;
}

bool InstanceManager::load_bookmark(Bookmark const* new_bookmark) {
	if (is_bookmark_loaded()) {
		spdlog::error_s("Cannot load bookmark - already loaded!");
		return false;
	}

	if (!is_game_instance_setup()) {
		spdlog::error_s("Cannot load bookmark - game instance not set up!");
		return false;
	}

	if (new_bookmark == nullptr) {
		spdlog::critical_s("Cannot load bookmark - null!");
		return false;
	}

	bookmark = new_bookmark;

	SPDLOG_INFO("Loading bookmark {} with start date {}", bookmark->get_name(), bookmark->get_date());

	if (!definition_manager.get_define_manager().in_game_period(bookmark->get_date())) {
		spdlog::warn_s("Bookmark date {} is not in the game's time period!", bookmark->get_date());
	}

	today = bookmark->get_date();

	politics_instance_manager.setup_starting_ideologies();
	bool ret = map_instance.apply_history_to_provinces(
		definition_manager.get_history_manager().get_province_manager(), today,
		country_instance_manager,
		// TODO - the following argument is for generating test pop attributes
		definition_manager.get_politics_manager().get_issue_manager(),
		market_instance,
		artisanal_producer_factory_pattern
	);

	// It is important that province history is applied before country history as province history includes
	// generating pops which then have stats like literacy and consciousness set by country history.

	ret &= country_instance_manager.apply_history_to_countries(*this);

	ret &= map_instance.get_state_manager().generate_states(
		map_instance,
		definition_manager.get_pop_manager().get_stratas(),
		definition_manager.get_pop_manager().get_pop_types(),
		definition_manager.get_politics_manager().get_ideology_manager().get_ideologies()
	);

	bool all_has_state = true;
	for (ProvinceInstance const& province_instance : map_instance.get_province_instances()) {
		if (!province_instance.get_province_definition().is_water() && OV_unlikely(province_instance.get_state() == nullptr)) {
			all_has_state = false;
			spdlog::error_s("Province {} has no state.", province_instance);
			continue;
		}
	}
	OV_ERR_FAIL_COND_V_MSG(!all_has_state, false, "At least one land province has no state");

	update_modifier_sums();
	map_instance.initialise_for_new_game(*this);
	country_instance_manager.update_gamestate(*this);
	market_instance.execute_orders();

	return ret;
}

bool InstanceManager::start_game_session() {
	if (is_game_session_started()) {
		spdlog::error_s("Cannot start game session - already started!");
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
		spdlog::error_s("Cannot update clock - game session not started!");
		return false;
	}

	simulation_clock.conditionally_advance_game();
	return true;
}

void InstanceManager::force_tick_and_update() {
	simulation_clock.force_advance_game();
}

bool InstanceManager::set_today_and_update(Date new_today) {
	if (!is_game_session_started()) {
		spdlog::error_s("Cannot update clock - game session not started!");
		return false;
	}

	today = new_today;
	gamestate_needs_update = true;
	update_gamestate();
	return true;
}

void InstanceManager::update_modifier_sums() {
	// Calculate national country modifier sums first, then local province modifier sums, adding province contributions
	// to controller countries' modifier sums if each province has a controller. This results in every country having a
	// full copy of all the modifiers affecting them in their modifier sum, but provinces only having their directly/locally
	// applied modifiers in their modifier sum, hence requiring owner country modifier effect values to be looked up when
	// determining the value of a global effect on the province.
	country_instance_manager.update_modifier_sums(
		today, definition_manager.get_modifier_manager().get_static_modifier_cache()
	);
	map_instance.update_modifier_sums(
		today, definition_manager.get_modifier_manager().get_static_modifier_cache()
	);
}

bool InstanceManager::queue_game_action(game_action_type_t type, game_action_argument_t&& argument) {
	if (currently_executing_game_actions) {
		spdlog::error_s("Attempted to queue a game action while already executing game actions!");
		return false;
	}

	if (type >= game_action_type_t::MAX_GAME_ACTION) {
		spdlog::critical_s("Invalid game action type {}", static_cast<uint64_t>(type));
		return false;
	}

	game_action_queue.emplace_back(type, std::move(argument));
	return true;
}
