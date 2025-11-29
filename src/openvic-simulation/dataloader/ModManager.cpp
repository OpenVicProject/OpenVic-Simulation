#include "ModManager.hpp"

#include <string_view>

#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/core/FormatValidate.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Mod::Mod(
	ModManager const& manager,
	std::string_view new_identifier,
	std::string_view new_path,
	std::optional<std::string_view> new_user_dir,
	memory::vector<memory::string> new_replace_paths,
	memory::vector<memory::string> new_dependencies
)
	: HasIdentifier { new_identifier },
	mod_manager { manager },
	dataloader_root_path { new_path },
	user_dir { new_user_dir },
	replace_paths { new_replace_paths },
	dependencies { new_dependencies } {}

vector_ordered_set<Mod const*> Mod::generate_dependency_list(bool* success) const {
	static constexpr size_t MAX_RECURSE = 16;
	size_t current_recurse = 0;

	vector_ordered_set<Mod const*> result;

	auto dep_cycle = [this, &current_recurse](auto self, Mod const* mod, vector_ordered_set<Mod const*>& dep_list) -> bool {
		bool ret = true;
		for (std::string_view dep_identifier : mod->get_dependencies()) {
			if (!mod_manager.has_mod_identifier(dep_identifier)) {
				spdlog::error_s(
					"Mod \"{}\" has unmet dependency \"{}\" and cannot be loaded!",
					ovfmt::validate(mod), dep_identifier
				);
				return false;
			}
			Mod const* dep = mod_manager.get_mod_by_identifier(dep_identifier);
			/* The poor man's cycle checking (cycles should be very rare and hard to accomplish with vic2 modding, this is a failsafe) */
			if (current_recurse == MAX_RECURSE) {
				spdlog::error_s(
					"Mod \"{}\" has cyclical or broken dependency chain and cannot be loaded!",
					ovfmt::validate(mod)
				);
				return false;
			} else {
				current_recurse++;
				ret &= self(self, dep, dep_list); /* recursively search for mod dependencies */
			}
			if (!dep_list.contains(dep)) {
				dep_list.emplace(dep);
			}
		}
		return ret;
	};

	bool loaded_deps = dep_cycle(dep_cycle, this, result);
	if (success) {
		*success = loaded_deps;
	}
	return result;
}

ModManager::ModManager() {}

bool ModManager::load_mod_file(ast::NodeCPtr root) {
	std::string_view identifier;
	std::string_view path;
	std::optional<std::string_view> user_dir;
	memory::vector<memory::string> replace_paths;
	memory::vector<memory::string> dependencies;

	bool ret = NodeTools::expect_dictionary_keys_and_default_map(
		map_key_value_ignore_invalid_callback<template_key_map_t<StringMapCaseSensitive>>,
		"name", ONE_EXACTLY, expect_string(assign_variable_callback(identifier)),
		"path", ONE_EXACTLY, expect_string(assign_variable_callback(path)),
		"user_dir", ZERO_OR_ONE, expect_string(assign_variable_callback_opt(user_dir)),
		"replace_path", ZERO_OR_MORE, expect_string(vector_callback_string(replace_paths)),
		"dependencies", ZERO_OR_ONE, expect_list_reserve_length(dependencies, expect_string(vector_callback_string(dependencies)))
	)(root);

	if (!ret) {
		//NodeTools already logs and an invalid (unloaded) mod won't stop the game.
		return true;
	}

	SPDLOG_INFO("Loaded mod descriptor for \"{}\"", identifier);
	mods.emplace_item(
		identifier,
		*this, identifier, path, user_dir, std::move(replace_paths), std::move(dependencies)
	);
	return true;
}

void ModManager::set_loaded_mods(memory::vector<Mod const*>&& new_loaded_mods) {
	OV_ERR_FAIL_COND_MSG(mods_loaded, "set_loaded_mods called twice");

	loaded_mods = std::move(new_loaded_mods);
	mods_loaded = true;
	for (Mod const* mod : loaded_mods) {
		SPDLOG_INFO("Loading mod \"{}\" at path {}", *mod, mod->get_dataloader_root_path());
	}
}

memory::vector<Mod const*> const& ModManager::get_loaded_mods() const {
	return loaded_mods;
}

size_t ModManager::get_loaded_mod_count() const {
	return loaded_mods.size();
}
