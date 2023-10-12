#pragma once

#include <filesystem>

#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"

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
		bool _load_units(GameManager& unit_manager, fs::path const& units_directory) const;
		bool _load_map_dir(GameManager& game_manager, fs::path const& map_directory) const;

	public:
		static ovdl::v2script::Parser parse_defines(fs::path const& path);
		static ovdl::csv::Windows1252Parser parse_csv(fs::path const& path);

		Dataloader() = default;

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
	};
}
