#include "ModManager.hpp"

#include <string_view>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Mod::Mod(std::string_view new_identifier, std::string_view new_path, std::optional<std::string_view> new_user_dir, memory::vector<memory::string> new_replace_paths, memory::vector<memory::string> new_dependencies)
	: HasIdentifier { new_identifier }, dataloader_root_path { new_path }, user_dir { new_user_dir }, replace_paths { new_replace_paths }, dependencies { new_dependencies } {}

ModManager::ModManager() {}

bool ModManager::load_mod_file(ast::NodeCPtr root) {
	std::string_view identifier;
	std::string_view path;
	std::optional<std::string_view> user_dir;
	memory::vector<memory::string> replace_paths;
	memory::vector<memory::string> dependencies;

	bool ret = NodeTools::expect_dictionary_keys(
		"name", ONE_EXACTLY, expect_string(assign_variable_callback(identifier)),
		"path", ONE_EXACTLY, expect_string(assign_variable_callback(path)),
		"user_dir", ZERO_OR_ONE, expect_string(assign_variable_callback_opt(user_dir)),
		"replace_path", ZERO_OR_MORE, expect_string(vector_callback_string(replace_paths)),
		"dependencies", ZERO_OR_ONE, expect_list_reserve_length(dependencies, expect_string(vector_callback_string(dependencies)))
	)(root);

	memory::vector<std::string_view> previous_mods = mods.get_item_identifiers();
	for (std::string_view dependency : dependencies) {
		if (std::find(previous_mods.begin(), previous_mods.end(), dependency) == previous_mods.end()) {
			ret = false;
			Logger::error("Mod ", identifier, " has unmet dependency ", dependency);
		}
	}

	if (ret) {
		ret &= mods.emplace_item(
			identifier, identifier, path, user_dir, std::move(replace_paths), std::move(dependencies)
		);
	}

	return ret;
}