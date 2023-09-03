#pragma once

#include <filesystem>

#include "openvic/types/Return.hpp"

#include "openvic-dataloader/v2script/Parser.hpp"

namespace OpenVic {
	struct GameManager;
	struct PopManager;

	class Dataloader {
		std::vector<std::filesystem::path> roots;

		static ovdl::v2script::Parser parse_defines(std::filesystem::path const& path);
		ovdl::v2script::Parser parse_defines_lookup(std::filesystem::path const& path) const;

		return_t _load_pop_types(PopManager& pop_manager, std::filesystem::path const& pop_type_directory) const;

	public:
		Dataloader() = default;

		/* In reverse-load order, so base defines first and final loaded mod last */
		return_t set_roots(std::vector<std::filesystem::path> new_roots);

		std::filesystem::path lookup_file(std::filesystem::path const& path) const;
		static const std::filesystem::path TXT;
		std::vector<std::filesystem::path> lookup_files_in_dir(std::filesystem::path const& path,
			std::filesystem::path const* extension = &TXT) const;
		return_t apply_to_files_in_dir(std::filesystem::path const& path,
			std::function<return_t(std::filesystem::path const&)> callback,
			std::filesystem::path const* extension = &TXT) const;

		return_t load_defines(GameManager& game_manager) const;
		return_t load_pop_history(GameManager& game_manager, std::filesystem::path const& path) const;
	};
}