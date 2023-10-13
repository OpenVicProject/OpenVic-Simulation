#pragma once

#include <filesystem>
#include <unordered_map>

#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	namespace fs = std::filesystem;

	struct GameManager;
	struct PopManager;
	struct UnitManager;
	struct GoodManager;

	class Dataloader {
	public:
		using path_vector_t = std::vector<fs::path>;

	private:
		path_vector_t roots;

		bool _load_pop_types(PopManager& pop_manager, fs::path const& pop_type_directory) const;
		bool _load_units(UnitManager& unit_manager, GoodManager const& good_manager, fs::path const& units_directory) const;
		bool _load_map_dir(GameManager& game_manager, fs::path const& map_directory) const;

	public:
		static ovdl::v2script::Parser parse_defines(fs::path const& path);
		static ovdl::v2script::Parser parse_lua_defines(fs::path const& path);
		static ovdl::csv::Windows1252Parser parse_csv(fs::path const& path);

		Dataloader() = default;

		/// @brief Searches for the Victoria 2 install directory
		///
		/// @param hint_path A path to indicate a hint to assist in searching for the Victoria 2 install directory
		///	Supports being supplied:
		///		1. A valid Victoria 2 game directory (Victoria 2 directory that contains a v2game.exe file)
		///		2. An Empty path: assumes a common Steam install structure per platform.
		///			2b. If Windows, returns "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Paradox Interactive\Victoria 2" "path" registry value
		///			2c. If registry value returns an empty string, performs Steam checks below
		///		3. A path to a root Steam install. (eg: C:\Program Files(x86)\Steam, ~/.steam/steam)
		///		4. A path to a root Steam steamapps directory. (eg: C:\Program Files(x86)\Steam\steamapps, ~/.steam/steam/steamapps)
		///		5. A path to the root Steam libraryfolders.vdf, commonly in the root Steam steamapps directory.
		///		6. A path to the Steam library directory that contains Victoria 2.
		///		7. A path to a Steam library's steamapps directory that contains Victoria 2.
		///		8. A path to a Steam library's steamapps/common directory that contains Victoria 2.
		///		9. If any of these checks don't resolve to a valid Victoria 2 game directory when supplied a non-empty hint_path, performs empty path behavior.
		/// @return fs::path The root directory of a valid Victoria 2 install, or an empty path.
		///
		static fs::path search_for_game_path(fs::path hint_path = {});

		/* In reverse-load order, so base defines first and final loaded mod last */
		bool set_roots(path_vector_t new_roots);

		/* REQUIREMENTS:
		 * DAT-24
		 */
		fs::path lookup_file(fs::path const& path) const;
		path_vector_t lookup_files_in_dir(fs::path const& path, fs::path const& extension) const;
		bool apply_to_files_in_dir(fs::path const& path, fs::path const& extension,
			NodeTools::callback_t<fs::path const&> callback) const;

		bool load_defines(GameManager& game_manager) const;
		bool load_pop_history(GameManager& game_manager, fs::path const& path) const;

		enum locale_t : size_t {
			English, French, German, Polish, Spanish, Italian, Swedish, Czech, Hungarian, Dutch, Portugese, Russian, Finnish, _LocaleCount
		};
		static constexpr char const* locale_names[_LocaleCount] = {
			"en_GB", "fr_FR", "de_DE", "pl_PL", "es_ES", "it_IT", "sv_SE", "cs_CZ", "hu_HU", "nl_NL", "pt_PT", "ru_RU", "fi_FI"
		};

		/* Args: key, locale, localisation */
		using localisation_callback_t = NodeTools::callback_t<std::string_view, locale_t, std::string_view>;
		bool load_localisation_files(localisation_callback_t callback, fs::path const& localisation_dir = "localisation");

	private:
		struct fshash
		{
			size_t operator()(const std::filesystem::path& p) const noexcept {
				return std::filesystem::hash_value(p);
			}
		};

		using hint_path_t = fs::path;
		using game_path_t = fs::path;
		inline static std::unordered_map<hint_path_t, game_path_t, fshash> _cached_paths;
	};
}
