#pragma once

#include <string_view>
#include <vector>

#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {

	struct Mod : HasIdentifier {
		friend struct ModManager;

	private:
		const std::string PROPERTY(dataloader_root_path);
		const std::optional<std::string> PROPERTY(user_dir);
		const std::vector<std::string> PROPERTY(replace_paths);
		const std::vector<std::string> PROPERTY(dependencies);

		Mod(std::string_view new_identifier, std::string_view new_path, std::optional<std::string_view> new_user_dir, std::vector<std::string> new_replace_paths, std::vector<std::string> new_dependencies);

	public:
		Mod(Mod&&) = default;
	};

	struct ModManager {
	
	private:
		IdentifierRegistry<Mod> IDENTIFIER_REGISTRY(mod);

	public:
		ModManager();

		bool load_mod_file(ast::NodeCPtr root);
	};
} // namespace OpenVic