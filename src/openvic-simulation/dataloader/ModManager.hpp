#pragma once

#include <cstddef>
#include <string_view>

#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	struct ModManager;

	struct Mod : HasIdentifier {
	private:
		ModManager const& mod_manager;
		const memory::string PROPERTY(dataloader_root_path);
		const std::optional<memory::string> PROPERTY(user_dir);
		const memory::vector<memory::string> PROPERTY(replace_paths);
		const memory::vector<memory::string> PROPERTY(dependencies);

	public:
		Mod(
			ModManager const& manager,
			std::string_view new_identifier,
			std::string_view new_path,
			std::optional<std::string_view> new_user_dir,
			memory::vector<memory::string> new_replace_paths,
			memory::vector<memory::string> new_dependencies
		);
		Mod(Mod&&) = default;

		vector_ordered_set<Mod const*> generate_dependency_list(bool* success = nullptr) const;
	};

	struct ModManager {

	private:
		IdentifierRegistry<Mod> IDENTIFIER_REGISTRY(mod);
		memory::vector<Mod const*> loaded_mods;
		bool mods_loaded = false;

	public:
		ModManager();

		bool load_mod_file(ast::NodeCPtr root);
		void set_loaded_mods(memory::vector<Mod const*>&& new_loaded_mods);
		memory::vector<Mod const*> const& get_loaded_mods() const;
		size_t get_loaded_mod_count() const;
	};
}