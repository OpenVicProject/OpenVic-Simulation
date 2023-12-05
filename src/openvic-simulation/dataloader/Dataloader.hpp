#pragma once

#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>
#include <unordered_map> //keep this here or mac builds will fail

#include "openvic-simulation/dataloader/NodeTools.hpp"

namespace OpenVic {
	namespace fs = std::filesystem;

	struct GameManager;
	class UIManager;
	struct PopManager;
	struct UnitManager;
	struct ModifierManager;
	struct TechnologyManager;
	struct GoodManager;

	class Dataloader {
	public:
		using path_vector_t = std::vector<fs::path>;

	private:
		path_vector_t roots;

		bool _load_interface_files(UIManager& ui_manager) const;
		bool _load_pop_types(PopManager& pop_manager, UnitManager const& unit_manager, GoodManager const& good_manager) const;
		bool _load_units(GameManager& game_manager) const;
		bool _load_goods(GameManager& game_manager) const;
		bool _load_technologies(GameManager& game_manager) const;
		bool _load_map_dir(GameManager& game_manager) const;
		bool _load_history(GameManager& game_manager, bool unused_history_file_warnings) const;

		/* _DirIterator is fs::directory_iterator or fs::recursive_directory_iterator. _UniqueKey is the type of a callable
		 * which converts a string_view filepath with root removed into a string_view unique key. Any path whose key is empty
		 * or matches an earlier found path's key is discarded, ensuring each looked up path's key is non-empty and unique. */
		template<typename _DirIterator, typename _UniqueKey>
		requires requires (_UniqueKey const& unique_key, std::string_view path) {
			{ unique_key(path) } -> std::convertible_to<std::string_view>;
		}
		path_vector_t _lookup_files_in_dir(
			std::string_view path, fs::path const& extension, _UniqueKey const& unique_key
		) const;

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
		///			2b. If Windows, returns "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Paradox Interactive\Victoria 2"
		///				"path" registry value
		///			2c. If registry value returns an empty string, performs Steam checks below
		///		3. A path to a root Steam install. (eg: C:\Program Files(x86)\Steam, ~/.steam/steam)
		///		4. A path to a root Steam steamapps directory. (eg: C:\Program Files(x86)\Steam\steamapps,
		///			~/.steam/steam/steamapps)
		///		5. A path to the root Steam libraryfolders.vdf, commonly in the root Steam steamapps directory.
		///		6. A path to the Steam library directory that contains Victoria 2.
		///		7. A path to a Steam library's steamapps directory that contains Victoria 2.
		///		8. A path to a Steam library's steamapps/common directory that contains Victoria 2.
		///		9. If any of these checks don't resolve to a valid Victoria 2 game directory when supplied
		///			a non-empty hint_path, performs empty path behavior.
		/// @return fs::path The root directory of a valid Victoria 2 install, or an empty path.
		///
		static fs::path search_for_game_path(fs::path hint_path = {});

		/* In reverse-load order, so base defines first and final loaded mod last */
		bool set_roots(path_vector_t const& new_roots);

		/* REQUIREMENTS:
		 * DAT-24
		 */
		fs::path lookup_file(std::string_view path, bool print_error = true) const;
		/* If the path ends with the extension ".tga", then this function will first try to load the file with the extension
		 * replaced with ".dds", and if that fails it will try the original ".tga" version. Paths not ending with ".tga" will
		 * just be passed to lookup_file. */
		fs::path lookup_image_file(std::string_view path) const;
		path_vector_t lookup_files_in_dir(std::string_view path, fs::path const& extension) const;
		path_vector_t lookup_files_in_dir_recursive(std::string_view path, fs::path const& extension) const;
		path_vector_t lookup_basic_indentifier_prefixed_files_in_dir(std::string_view path, fs::path const& extension) const;
		path_vector_t lookup_basic_indentifier_prefixed_files_in_dir_recursive(
			std::string_view path, fs::path const& extension
		) const;
		bool apply_to_files(path_vector_t const& files, NodeTools::callback_t<fs::path const&> callback) const;

		bool load_defines(GameManager& game_manager) const;
		bool load_pop_history(GameManager& game_manager, std::string_view path) const;

		enum locale_t : size_t {
			English, French, German, Polish, Spanish, Italian, Swedish,
			Czech, Hungarian, Dutch, Portugese, Russian, Finnish, _LocaleCount
		};
		static constexpr char const* locale_names[_LocaleCount] = {
			"en_GB", "fr_FR", "de_DE", "pl_PL", "es_ES", "it_IT", "sv_SE",
			"cs_CZ", "hu_HU", "nl_NL", "pt_PT", "ru_RU", "fi_FI"
		};

		/* Args: key, locale, localisation */
		using localisation_callback_t = NodeTools::callback_t<std::string_view, locale_t, std::string_view>;
		bool load_localisation_files(
			localisation_callback_t callback, std::string_view localisation_dir = "localisation"
		) const;
	};
}
