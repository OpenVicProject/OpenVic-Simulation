#pragma once

#include <filesystem>
#include <functional>
#include <vector>

namespace OpenVic {
	namespace fs = std::filesystem;

	struct GameManager;
	struct PopManager;
	struct Map;

	class Dataloader {
	public:
		using path_vector_t = std::vector<fs::path>;
	private:
		path_vector_t roots;

		bool _load_pop_types(PopManager& pop_manager, fs::path const& pop_type_directory) const;
		bool _load_map_dir(Map& map, fs::path const& map_directory) const;

	public:
		Dataloader() = default;

		/* In reverse-load order, so base defines first and final loaded mod last */
		bool set_roots(path_vector_t new_roots);

		fs::path lookup_file(fs::path const& path) const;
		static const fs::path TXT;
		path_vector_t lookup_files_in_dir(fs::path const& path,
			fs::path const* extension = &TXT) const;
		bool apply_to_files_in_dir(fs::path const& path,
			std::function<bool(fs::path const&)> callback,
			fs::path const* extension = &TXT) const;

		bool load_defines(GameManager& game_manager) const;
		bool load_pop_history(GameManager& game_manager, fs::path const& path) const;
	};
}
