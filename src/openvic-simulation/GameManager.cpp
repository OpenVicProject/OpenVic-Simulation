#include "GameManager.hpp"

#include <chrono>
#include <string_view>

#include <range/v3/algorithm/contains.hpp>
#include <range/v3/algorithm/find_if.hpp>

#include <spdlog/spdlog.h>

#include "openvic-simulation/core/memory/OrderedSet.hpp"
#include "openvic-simulation/dataloader/Dataloader.hpp"
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
		spdlog::warn_s("Only one of the elapsed time callbacks was set.");
	}
}

GameManager::~GameManager() {
	if (instance_manager) {
		spdlog::error_s(
			"Destroying GameManager with an active InstanceManager! "
			"Calling end_game_session() to ensure proper cleanup and execute any required special end-of-session actions."
		);

		end_game_session();
	}
}

bool GameManager::load_mod_descriptors() {
	if (mod_descriptors_loaded) {
		spdlog::error_s("Cannot load mod descriptors - already loaded!");
		return false;
	}

	if (!dataloader.load_mod_descriptors(mod_manager)) {
		spdlog::critical_s("Failed to load mod descriptors!");
		return false;
	}
	return true;
}

bool GameManager::load_mods(memory::vector<memory::string> const& mods_to_find) {
	if (mods_to_find.empty()) {
		return true;
	}

	bool ret = true;

	memory::vector_ordered_set<Mod const*> load_list;

	/* Check loaded mod descriptors for requested mods, using either full name or user directory name
	 * (Historical Project Mod 0.4.6 or HPM both valid, for example), and load them plus their dependencies.
	 */
	for (std::string_view requested_mod : mods_to_find) {
		memory::vector<Mod>::const_iterator it = ranges::find_if(mod_manager.get_mods(),
			[&requested_mod](Mod const& mod) -> bool {
				return mod.get_identifier() == requested_mod || mod.get_user_dir() == requested_mod;
			}
		);

		if (it == mod_manager.get_mods().end()) {
			spdlog::warn_s("Requested mod \"{}\" does not exist!", requested_mod);
			ret = false;
			continue;
		}

		Mod const* mod_ptr = &*it;
		memory::vector_ordered_set<Mod const*> dependencies = mod_ptr->generate_dependency_list(&ret);
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

	Dataloader::path_vector_t roots = dataloader.get_roots();
	roots.reserve(load_list.size() + roots.size());

	vector_ordered_set<fs::path> replace_paths;

	/* Actually registers all roots and replace paths to be loaded by the game. */
	for (Mod const* mod : load_list) {
		roots.emplace_back(roots[0] / mod->get_dataloader_root_path());
		for (std::string_view path : mod->get_replace_paths()) {
			replace_paths.emplace(path);
		}
	}

	/* Load only vanilla and push an error if mod loading failed. */
	if (ret) {
		mod_manager.set_loaded_mods(load_list.release());
	} else {
		mod_manager.set_loaded_mods({});
		spdlog::error_s("Mod loading failed, loading base only!");
	}

	if (!dataloader.set_roots(roots, replace_paths.values_container(), false)) {
		spdlog::critical_s("Failed to set dataloader roots!");
		ret = false;
	}

	return ret;
}

bool GameManager::load_definitions(Dataloader::localisation_callback_t localisation_callback) {
	if (definitions_loaded) {
		spdlog::error_s("Cannot load definitions - already loaded!");
		return false;
	}

	bool ret = true;

	if (!dataloader.load_defines(game_rules_manager, definition_manager)) {
		spdlog::critical_s("Failed to load defines!");
		ret = false;
	}

	if (!dataloader.load_localisation_files(localisation_callback)) {
		spdlog::error_s("Failed to load localisation!");
		ret = false;
	}

	definitions_loaded = true;

	return ret;
}

bool GameManager::setup_instance(Bookmark const& bookmark) {
	if (instance_manager) {
		spdlog::error_s("Trying to setup a new game instance while one is already setup!");
		return false;
	}

	SPDLOG_INFO("Initialising new game instance.");

	instance_manager.emplace(
		game_rules_manager,
		definition_manager,
		gamestate_updated_callback
	);

	SPDLOG_INFO("Setting up new game instance.");

	bool ret = instance_manager->setup();

	SPDLOG_INFO("Loading bookmark \"{}\" for new game instance.", bookmark.get_identifier());

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
		spdlog::error_s("Cannot start game session - instance manager not set up!");
		return false;
	}

	if (instance_manager->is_game_session_started()) {
		spdlog::error_s("Cannot start game session - session already started!");
		return false;
	}

	if (!instance_manager->is_bookmark_loaded()) {
		spdlog::warn_s("Starting game session with no bookmark loaded!");
	}

	SPDLOG_INFO("Starting game session.");

	return instance_manager->start_game_session();
}

bool GameManager::end_game_session() {
	if (!instance_manager) {
		spdlog::error_s("Cannot end game session - instance manager not initialised!");
		return false;
	}

	if (!instance_manager->is_game_instance_setup() || !instance_manager->is_game_session_started()) {
		spdlog::warn_s("Trying to end game session that hasn't yet finished setting up and/or actually started!");
	}

	SPDLOG_INFO("Ending game session.");

	instance_manager.reset();

	return true;
}

bool GameManager::is_game_session_active() const {
	return instance_manager && instance_manager->is_game_session_started();
}

bool GameManager::update_clock() {
	if (!instance_manager) {
		spdlog::error_s("Cannot update clock - instance manager uninitialised!");
		return false;
	}

	return instance_manager->update_clock();
}

uint64_t GameManager::get_elapsed_microseconds() {
	return get_elapsed_usec_time_callback();
}

uint64_t GameManager::get_elapsed_milliseconds() {
	return get_elapsed_msec_time_callback();
}
