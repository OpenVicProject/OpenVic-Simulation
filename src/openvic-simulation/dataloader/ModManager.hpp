#pragma once

#include <string_view>

#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	struct Mod : HasIdentifier {
	private:
		const memory::string PROPERTY(dataloader_root_path);
		const std::optional<memory::string> PROPERTY(user_dir);
		const memory::vector<memory::string> PROPERTY(replace_paths);
		const memory::vector<memory::string> PROPERTY(dependencies);

	public:
		Mod(std::string_view new_identifier, std::string_view new_path, std::optional<std::string_view> new_user_dir, memory::vector<memory::string> new_replace_paths, memory::vector<memory::string> new_dependencies);
		Mod(Mod&&) = default;
	};

	struct ModManager {
	
	private:
		IdentifierRegistry<Mod> IDENTIFIER_REGISTRY(mod);

	public:
		ModManager();

		bool load_mod_file(ast::NodeCPtr root);
	};
}