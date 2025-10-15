#include "GameManager.hpp"

#include <chrono>
#include <cstddef>
#include <string_view>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/multiplayer/ClientManager.hpp"
#include "openvic-simulation/multiplayer/HostManager.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/utility/Logger.hpp"

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

GameManager::~GameManager() {
	if (instance_manager) {
		Logger::error(
			"Destroying GameManager with an active InstanceManager! "
			"Calling end_game_session() to ensure proper cleanup and execute any required special end-of-session actions."
		);

		end_game_session();
	}
}

bool GameManager::load_mod_descriptors() {
	if (mod_descriptors_loaded) {
		Logger::error("Cannot load mod descriptors - already loaded!");
		return false;
	}

	if (!dataloader.load_mod_descriptors(mod_manager)) {
		Logger::error("Failed to load mod descriptors!");
		return false;
	}
	return true;
}

bool GameManager::load_mods(
	Dataloader::path_vector_t& roots,
	Dataloader::path_vector_t& replace_paths,
	utility::forwardable_span<const memory::string> requested_mods
) {
	if (requested_mods.empty()) {
		return true;
	}

	bool ret = true;

	vector_ordered_set<Mod const*> load_list;

	/* Check loaded mod descriptors for requested mods, using either full name or user directory name
	 * (Historical Project Mod 0.4.6 or HPM both valid, for example), and load them plus their dependencies.
	 */
	for (std::string_view requested_mod : requested_mods) {
		auto it = std::find_if(
			mod_manager.get_mods().begin(),
			mod_manager.get_mods().end(),
			[&requested_mod](Mod const& mod) -> bool {
				return mod.get_identifier() == requested_mod || mod.get_user_dir() == requested_mod;
			}
		);

		if (it == mod_manager.get_mods().end()) {
			Logger::error("Requested mod \"", requested_mod, "\" does not exist!");
			ret = false;
			continue;
		}

		Mod const* mod_ptr = &*it;
		vector_ordered_set<Mod const*> dependencies = mod_ptr->generate_dependency_list(&ret);
		if(!ret) {
			continue;
		}

		/* Add mod plus dependencies to load_list in proper order. */
		if (load_list.empty()) {
			load_list = std::move(dependencies);
		} else {
			for (Mod const* dep : dependencies) {
				if (!load_list.contains(dep)) {
					load_list.emplace(dep);
				}
			}
		}

		if (!load_list.contains(mod_ptr)) {
			load_list.emplace(mod_ptr);
		}
	}

	/* Actually registers all roots and replace paths to be loaded by the game. */
	for (Mod const* mod : load_list) {
		roots.emplace_back(roots[0] / mod->get_dataloader_root_path());
		for (std::string_view path : mod->get_replace_paths()) {
			if (std::find(replace_paths.begin(), replace_paths.end(), path) == replace_paths.end()) {
				replace_paths.emplace_back(path);
			}
		}
	}

	/* Load only vanilla and push an error if mod loading failed. */
	if (ret) {
		mod_manager.set_loaded_mods(std::move(load_list.release()));
	} else {
		mod_manager.set_loaded_mods({});
		replace_paths.clear();
		roots.erase(roots.begin()+1, roots.end());
		Logger::error("Mod loading failed, loading base only!");
	}

	if (!dataloader.set_roots(roots, replace_paths, false)) {
		Logger::error("Failed to set dataloader roots!");
		ret = false;
	}

	return ret;
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
		Logger::error("Trying to setup a new game instance while one is already setup!");
		return false;
	}

	Logger::info("Initialising new game instance.");

	instance_manager.emplace(
		game_rules_manager,
		definition_manager,
		gamestate_updated_callback
	);

	Logger::info("Setting up new game instance.");

	bool ret = instance_manager->setup();

	Logger::info("Loading bookmark \"", bookmark, "\" for new game instance.");

	ret &= instance_manager->load_bookmark(bookmark);

	return ret;
}

bool GameManager::is_game_instance_setup() const {
	return instance_manager && instance_manager->is_game_instance_setup();
}

bool GameManager::is_bookmark_loaded() const {
	return instance_manager && instance_manager->is_bookmark_loaded();
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

	Logger::info("Starting game session.");

	return instance_manager->start_game_session();
}

bool GameManager::end_game_session() {
	if (!instance_manager) {
		Logger::error("Cannot end game session - instance manager not initialised!");
		return false;
	}

	if (!instance_manager->is_game_instance_setup() || !instance_manager->is_game_session_started()) {
		Logger::warning("Trying to end game session that hasn't yet finished setting up and/or actually started!");
	}

	Logger::info("Ending game session.");

	instance_manager.reset();

	return true;
}

bool GameManager::is_game_session_active() const {
	return instance_manager && instance_manager->is_game_session_started();
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
	chat_manager = memory::make_unique<ChatManager>(client_manager.get());
}

void GameManager::create_host(memory::string session_name) {
	host_manager = memory::make_unique<HostManager>(this);
	if (!session_name.empty()) {
		host_manager->get_host_session().set_game_name(session_name);
	}
}

void GameManager::threaded_poll_network() {
	const int MAX_POLL_WAIT_MSEC = 100;
	const int SLEEP_DURATION_USEC = 1000;

	if (host_manager) {
		const uint64_t time = GameManager::get_elapsed_milliseconds();
		while (!(host_manager->poll() > 0) && (GameManager::get_elapsed_milliseconds() - time) < MAX_POLL_WAIT_MSEC) {
			std::this_thread::sleep_for(std::chrono::microseconds { SLEEP_DURATION_USEC });
		}
	}

	if (client_manager) {
		const uint64_t time = GameManager::get_elapsed_milliseconds();
		while (!(client_manager->poll() > 0) && (GameManager::get_elapsed_milliseconds() - time) < MAX_POLL_WAIT_MSEC) {
			std::this_thread::sleep_for(std::chrono::microseconds { SLEEP_DURATION_USEC });
		}

		if (host_manager) {
			// TODO: create local ClientManager that doesn't send network data to HostManager
			// In the case that client_manager sends something, host_manager may handle it
			const uint64_t time = GameManager::get_elapsed_milliseconds();
			while (!(host_manager->poll() > 0) && (GameManager::get_elapsed_milliseconds() - time) < MAX_POLL_WAIT_MSEC / 4) {
				std::this_thread::sleep_for(std::chrono::microseconds { SLEEP_DURATION_USEC / 4 });
			}
		}
	}
}

void GameManager::delete_client() {
	client_manager.reset();
}

void GameManager::delete_host() {
	host_manager.reset();
}

uint64_t GameManager::get_elapsed_microseconds() {
	return get_elapsed_usec_time_callback();
}

uint64_t GameManager::get_elapsed_milliseconds() {
	return get_elapsed_msec_time_callback();
}
