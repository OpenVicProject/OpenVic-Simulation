#include "ModManager.hpp"

#include <string>
#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Mod::Mod(std::string_view new_identifier, std::string_view new_path, std::optional<std::string_view> new_user_dir, std::vector<std::string> new_replace_paths, std::vector<std::string> new_dependencies)
	: HasIdentifier { new_identifier }, dataloader_root_path { new_path }, user_dir { new_user_dir }, replace_paths { new_replace_paths }, dependencies { new_dependencies } {}

ModManager::ModManager() {}

bool ModManager::load_mod_file(ast::NodeCPtr root) {
	std::string_view identifier;
	std::string_view path;
	std::optional<std::string_view> user_dir;
	std::vector<std::string> replace_paths;
	std::vector<std::string> dependencies;

	bool ret = NodeTools::expect_dictionary_keys_and_default(
		key_value_success_callback,
		"name", ONE_EXACTLY, expect_string(assign_variable_callback(identifier)),
		"path", ONE_EXACTLY, expect_string(assign_variable_callback(path)),
		"user_dir", ZERO_OR_ONE, expect_string(assign_variable_callback_opt(user_dir)),
		"replace_path", ZERO_OR_MORE, expect_string(vector_callback_string(replace_paths)),
		"dependencies", ZERO_OR_ONE, expect_list_reserve_length(dependencies, expect_string(vector_callback_string(dependencies)))
	)(root);

	if (ret) {
		Logger::info("Loaded mod descriptor for \"", identifier, "\"");
		ret &= mods.add_item(
			{ identifier, path, user_dir, std::move(replace_paths), std::move(dependencies) }
		);
	} else {
		Logger::warning("Invalid mod descriptor with identifier \"", identifier, "\" could not be loaded!");
	}

	return true;
}

void ModManager::set_loaded_mods(std::vector<Mod const*>&& new_loaded_mods) {
	OV_ERR_FAIL_COND_MSG(mods_loaded, "set_loaded_mods called twice");

	loaded_mods = std::move(new_loaded_mods);
	mods_loaded = true;
	for (Mod const* mod : loaded_mods) {
		Logger::info("Loading mod \"", mod->get_identifier(), "\" at path ", mod->get_dataloader_root_path());
	}
}

std::vector<Mod const*> const& ModManager::get_loaded_mods() const {
	return loaded_mods;
}

size_t ModManager::get_loaded_mod_count() const {
	return loaded_mods.size();
}
