#include "GameManager.hpp"

#include <cstddef>
#include <string_view>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

GameManager::GameManager(
	InstanceManager::gamestate_updated_func_t new_gamestate_updated_callback
) : gamestate_updated_callback {
		new_gamestate_updated_callback ? std::move(new_gamestate_updated_callback) : []() {}
	}, definitions_loaded { false }, mod_descriptors_loaded { false } {}

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

bool GameManager::_get_mod_dependencies(Mod const* mod, memory::vector<Mod const*>& dep_list) {
	static constexpr size_t MAX_RECURSE = 16;
	size_t current_recurse = 0;

	static auto dep_cycle = [this, &current_recurse](auto self, Mod const* mod, memory::vector<Mod const*>& dep_list) -> bool {
		bool ret = true;
		for (std::string_view dep_identifier : mod->get_dependencies()) {
			if (!mod_manager.has_mod_identifier(dep_identifier)) {
				Logger::error("Mod \"", mod->get_identifier(), "\" has unmet dependency \"", dep_identifier, "\" and cannot be loaded!");
				return false;
			}
			Mod const* dep = mod_manager.get_mod_by_identifier(dep_identifier);
			/* The poor man's cycle checking (cycles should be very rare and hard to accomplish with vic2 modding, this is a failsafe) */
			if (current_recurse == MAX_RECURSE) {
				Logger::error("Mod \"", mod->get_identifier(), "\" has cyclical or broken dependency chain and cannot be loaded!");
				return false;
			} else {
				current_recurse++;
				ret &= self(self, dep, dep_list); /* recursively search for mod dependencies */
			}
			if (std::find(dep_list.begin(), dep_list.end(), dep) == dep_list.end()) {
				dep_list.emplace_back(dep);
			}
		}
		return ret;
	};
	return dep_cycle(dep_cycle, mod, dep_list);
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

	memory::vector<Mod const*> load_list;

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
		memory::vector<Mod const*> dependencies;
		if(!_get_mod_dependencies(mod_ptr, dependencies)) {
			ret = false;
			continue;
		}

		/* Add mod plus dependencies to load_list in proper order. */
		load_list.reserve(1 + dependencies.size());
		for (Mod const* dep : dependencies) {
			if (ret && std::find(load_list.begin(), load_list.end(), dep) == load_list.end()) {
				load_list.emplace_back(dep);
			}
		}
		if (ret && std::find(load_list.begin(), load_list.end(), mod_ptr) == load_list.end()) {
			load_list.emplace_back(mod_ptr);
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
		mod_manager.set_loaded_mods(std::move(load_list));
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
