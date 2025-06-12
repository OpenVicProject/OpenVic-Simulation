#include "GameManager.hpp"

#include <chrono>
#include <string_view>

#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/HostManager.hpp"

using namespace OpenVic;

static std::chrono::time_point elapsed_time_begin = std::chrono::high_resolution_clock::now();

GameManager::elapsed_time_getter_func_t GameManager::get_elapsed_usec_time_callback = []() -> uint64_t {
	std::chrono::time_point current = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(current - elapsed_time_begin).count();
};

GameManager::elapsed_time_getter_func_t GameManager::get_elapsed_msec_time_callback = []() -> uint64_t {
	std::chrono::time_point current = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(current - elapsed_time_begin).count();
};

GameManager::GameManager(
	InstanceManager::gamestate_updated_func_t new_gamestate_updated_callback,
	elapsed_time_getter_func_t new_get_elapsed_usec_callback,
	elapsed_time_getter_func_t new_get_elapsed_msec_callback
) : gamestate_updated_callback {
		new_gamestate_updated_callback ? std::move(new_gamestate_updated_callback) : []() {}
	}, definitions_loaded { false }, mod_descriptors_loaded { false } {
	if (new_get_elapsed_usec_callback) {
		get_elapsed_usec_time_callback = { std::move(new_get_elapsed_usec_callback) };
	}
	if (new_get_elapsed_msec_callback) {
		get_elapsed_msec_time_callback = { std::move(new_get_elapsed_msec_callback) };
	}

	if (bool(new_get_elapsed_usec_callback) != bool(new_get_elapsed_msec_callback)) {
		Logger::warning("Only one of the elapsed time callbacks was set.");
	}
}

bool GameManager::load_mod_descriptors(std::span<const memory::string> descriptors) {
	if (mod_descriptors_loaded) {
		Logger::error("Cannot load mod descriptors - already loaded!");
		return false;
	}

	if (!dataloader.load_mod_descriptors(descriptors, mod_manager)) {
		Logger::error("Failed to load mod descriptors!");
		return false;
	}
	return true;
}

bool GameManager::set_roots(Dataloader::path_span_t roots, Dataloader::path_span_t replace_paths) {
	if (!dataloader.set_roots(roots, replace_paths)) {
		Logger::error("Failed to set dataloader roots!");
		return false;
	}
	return true;
}

bool GameManager::load_definitions(Dataloader::localisation_callback_t localisation_callback) {
	if (definitions_loaded) {
		Logger::error("Cannot load definitions - already loaded!");
		return false;
	}

	bool ret = true;

	if (!dataloader.load_defines(game_rules_manager, definition_manager)) {
		Logger::error("Failed to load defines!");
		ret = false;
	}

	if (!dataloader.load_localisation_files(localisation_callback)) {
		Logger::error("Failed to load localisation!");
		ret = false;
	}

	definitions_loaded = true;

	return ret;
}

bool GameManager::setup_instance(Bookmark const* bookmark) {
	if (instance_manager) {
		Logger::info("Resetting existing game instance.");
	} else {
		Logger::info("Setting up first game instance.");
	}

	instance_manager.emplace(
		game_rules_manager,
		definition_manager,
		gamestate_updated_callback
	);

	bool ret = instance_manager->setup();
	ret &= instance_manager->load_bookmark(bookmark);

	return ret;
}

bool GameManager::start_game_session() {
	if (!instance_manager || !instance_manager->is_game_instance_setup()) {
		Logger::error("Cannot start game session - instance manager not set up!");
		return false;
	}

	if (instance_manager->is_game_session_started()) {
		Logger::error("Cannot start game session - session already started!");
		return false;
	}

	if (!instance_manager->is_bookmark_loaded()) {
		Logger::warning("Starting game session with no bookmark loaded!");
	}

	return instance_manager->start_game_session();
}

bool GameManager::update_clock() {
	if (!instance_manager) {
		Logger::error("Cannot update clock - instance manager uninitialised!");
		return false;
	}

	return instance_manager->update_clock();
}

void GameManager::create_client() {
	client_manager = memory::make_unique<ClientManager>(this);
}

void GameManager::create_host(std::string_view session_name) {
	host_manager = memory::make_unique<HostManager>(this);
	if (!session_name.empty()) {
		host_manager->get_host_session().set_game_name(memory::string { session_name });
	}
}

uint64_t GameManager::get_elapsed_microseconds() {
	return get_elapsed_usec_time_callback();
}

uint64_t GameManager::get_elapsed_milliseconds() {
	return get_elapsed_msec_time_callback();
}
