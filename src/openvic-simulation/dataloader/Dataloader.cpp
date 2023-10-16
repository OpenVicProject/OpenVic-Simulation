#include "Dataloader.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>

#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/detail/CallbackOStream.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <lexy-vdf/KeyValues.hpp>
#include <lexy-vdf/Parser.hpp>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/utility/ConstexprIntToStr.hpp"
#include "openvic-simulation/utility/Logger.hpp"

#ifdef _WIN32
#include <Windows.h>
#include "Dataloader_Windows.hpp"
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#endif

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using namespace ovdl;

// Windows and Mac by default act like case insensitive filesystems
static constexpr bool path_equals(std::string_view lhs, std::string_view rhs) {
#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
	constexpr auto ichar_equals = [](unsigned char l, unsigned char r) {
		return std::tolower(l) == std::tolower(r);
	};
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), ichar_equals);
#else
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
#endif
}

template<typename LT, typename RT>
static bool filename_equals(const LT& lhs, const RT& rhs) {
	std::string left, right;
	if constexpr (std::same_as<LT, std::filesystem::path>)
		left = lhs.filename().string();
	else left = lhs;

	if constexpr (std::same_as<RT, std::filesystem::path>)
		right = rhs.filename().string();
	else right = rhs;

	return path_equals(left, right);
}

static fs::path _search_for_game_path(fs::path hint_path = {}) {
	// Apparently max amount of steam libraries is 8, if incorrect please correct it to the correct max amount
	constexpr int max_amount_of_steam_libraries = 8;
	constexpr std::string_view Victoria_2_folder = "Victoria 2";
	constexpr std::string_view v2_game_exe = "v2game.exe";
	constexpr std::string_view steamapps = "steamapps";
	constexpr std::string_view libraryfolders = "libraryfolders.vdf";
	constexpr std::string_view vic2_appmanifest = "appmanifest_42960.acf";
	constexpr std::string_view common_folder = "common";

	std::error_code error_code;

	// Don't waste time trying to search for Victoria 2 when supplied a valid looking Victoria 2 game directory
	if (filename_equals(Victoria_2_folder, hint_path)) {
		if (fs::is_regular_file(hint_path / v2_game_exe, error_code))
			return hint_path;
	}

	const bool hint_path_was_empty = hint_path.empty();
	if (hint_path_was_empty) {
#if defined(_WIN32)
		static const fs::path registry_path = Windows::ReadRegValue<char>(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Paradox Interactive\\Victoria 2", "path");

		if (!registry_path.empty())
			return registry_path;

		#pragma warning(push)
		#pragma warning(disable: 4996)
		static const fs::path prog_files = std::string(std::getenv("ProgramFiles"));
		hint_path = prog_files / "Steam";
		if (!fs::is_directory(hint_path, error_code)) {
			static const fs::path prog_files_x86 = std::string(std::getenv("ProgramFiles(x86)"));
			hint_path = prog_files_x86 / "Steam";
			if (!fs::is_directory(hint_path, error_code)) {
				Logger::warning("Could not find path for Steam installation on Windows.");
				return "";
			}
		}
		#pragma warning(pop)
// Cannot support Android
// Only FreeBSD currently unofficially supports emulating Linux
#elif (defined(__linux__) && !defined(__ANDROID__)) || defined(__FreeBSD__)
		static const fs::path home = std::getenv("HOME");
		hint_path = home / ".steam" / "steam";
		if (fs::is_symlink(hint_path, error_code))
			hint_path = fs::read_symlink(hint_path, error_code);
		else if (!fs::is_directory(hint_path, error_code)) {
			hint_path = home / ".local" / "share" / "Steam";
			if (!fs::is_directory(hint_path, error_code)) {
#ifdef __FreeBSD__
				Logger::warning("Could not find path for Steam installation on FreeBSD.");
#else
				Logger::warning("Could not find path for Steam installation on Linux.");
#endif
				return "";
			}
		}
// Support only Mac, cannot support iOS
#elif (defined(__APPLE__) && defined(__MACH__)) && TARGET_OS_MAC == 1
		static const fs::path home = std::getenv("HOME");
		hint_path = home / "Library" / "Application Support" / "Steam";
		if (!fs::is_directory(hint_path, error_code)) {
			Logger::warning("Could not find path for Steam installation on Mac.");
			return "";
		}
// All platforms that reach this point do not seem to even have unofficial Steam support
#else
		Logger::warning("Could not find path for Steam installation on unsupported platform.");
#endif
	}

	// Could not determine Steam install on platform
	if (hint_path.empty()) return "";

	// Supplied path was useless, ignore hint_path
	if (!hint_path_was_empty && !fs::exists(hint_path, error_code))
		return _search_for_game_path();

	// Steam Library's directory that will contain Victoria 2
	fs::path vic2_steam_lib_directory;
	fs::path current_path = hint_path;

	// If hinted path is directory that contains steamapps
	bool is_steamapps = false;
	if (fs::is_directory(current_path / steamapps, error_code)) {
		current_path /= steamapps;
		is_steamapps = true;
	}

	// If hinted path is steamapps directory
	bool is_libraryfolders_vdf = false;
	if (is_steamapps || (filename_equals(steamapps, current_path) && fs::is_directory(current_path, error_code))) {
		current_path /= libraryfolders;
		is_libraryfolders_vdf = true;
	}

	bool vic2_install_confirmed = false;
	// if current_path is not a regular file, this is a non-default Steam Library, skip this parser evaluation
	if (fs::is_regular_file(current_path, error_code) && (is_libraryfolders_vdf || filename_equals(libraryfolders, current_path))) {
		lexy_vdf::Parser parser;

		std::string buffer;
		auto error_log_stream = detail::CallbackStream {
			[](void const* s, std::streamsize n, void* user_data) -> std::streamsize {
				if (s != nullptr && n > 0 && user_data != nullptr) {
					static_cast<std::string*>(user_data)->append(static_cast<char const*>(s), n);
					return n;
				} else {
					Logger::warning("Invalid input to parser error log callback: ", s, " / ", n, " / ", user_data);
					return 0;
				}
			},
			&buffer
		};
		parser.set_error_log_to(error_log_stream);

		parser.load_from_file(current_path);
		if (!parser.parse()) {
			// Could not find or load libraryfolders.vdf, report error as warning
			if (!buffer.empty())
				Logger::warning _(buffer);
			return "";
		}
		std::optional current_node = *(parser.get_key_values());

		// check "libraryfolders" list
		auto it = current_node.value().find("libraryfolders");
		if (it == current_node.value().end()) {
			Logger::warning("Expected libraryfolders.vdf to contain a libraryfolders key.");
			return "";
		}

		static constexpr auto visit_node = [](auto&& arg) -> std::optional<lexy_vdf::KeyValues> {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, lexy_vdf::KeyValues>) {
				return arg;
			} else {
				return std::nullopt;
			}
		};

		current_node = std::visit(visit_node, it->second);

		if (!current_node.has_value()) {
			Logger::warning("Expected libraryfolders.vdf's libraryfolders key to be a KeyValue dictionary.");
			return "";
		}

		// Array of strings contain "0" to std::to_string(max_amount_of_steam_libraries - 1)
		static constexpr auto library_indexes = OpenVic::ConstexprIntToStr::make_itosv_array<max_amount_of_steam_libraries>();

		for (const auto& index : library_indexes) {
			decltype(current_node) node = std::nullopt;

			auto it = current_node.value().find(index);
			if (it != current_node.value().end()) {
				node = std::visit(visit_node, it->second);
			}

			// check "apps" list
			decltype(node) apps_node = std::nullopt;
			if (node.has_value()) {
				it = node.value().find("apps");
				if (it != node.value().end()) {
					apps_node = std::visit(visit_node, it->second);
				}
			}

			bool lib_contains_victoria_2 = false;
			if (apps_node.has_value()) {
				lib_contains_victoria_2 = apps_node.value().find("42960") != node.value().end();
			}

			if (lib_contains_victoria_2) {
				it = node.value().find("path");
				if (it != node.value().end()) {
					vic2_steam_lib_directory = std::visit(
						[](auto&& arg) -> std::string_view {
							using T = std::decay_t<decltype(arg)>;
							if constexpr (std::is_same_v<T, std::string>) {
								return arg;
							} else {
								return "";
							}
						},
						it->second
					);
					vic2_install_confirmed = true;
					break;
				}
			}
		}

		if (vic2_steam_lib_directory.empty()) {
			Logger::info("Steam installation appears not to contain Victoria 2.");
			return "";
		}
	}

	// If current_path points to steamapps/libraryfolders.vdf
	if (vic2_steam_lib_directory.empty()) {
		if (is_libraryfolders_vdf || filename_equals(libraryfolders, current_path)) {
			vic2_steam_lib_directory = current_path.parent_path() / vic2_appmanifest;
		} else if (filename_equals(vic2_appmanifest, current_path)) {
			vic2_steam_lib_directory = current_path;
		}
	}

	// If we could not confirm Victoria 2 was installed via the default Steam installation
	bool is_common_folder = false;
	if (!vic2_install_confirmed) {
		auto parser = lexy_vdf::Parser::from_file(vic2_steam_lib_directory);
		if (!parser.parse()) {
			// Could not find or load appmanifest_42960.acf, report error as warning
			for(auto& error : parser.get_errors()) {
				Logger::warning(error.message);
			}
			return "";
		}

		// we can pretty much assume the Victoria 2 directory on Steam is valid from here
		vic2_steam_lib_directory /= common_folder;
		is_common_folder = true;
	} else if (fs::is_directory(vic2_steam_lib_directory / steamapps, error_code)) {
		vic2_steam_lib_directory /= fs::path(steamapps) / common_folder;
		is_common_folder = true;
	}

	bool is_Victoria_2_folder = false;
	if ((is_common_folder || filename_equals(common_folder, vic2_steam_lib_directory)) &&
		fs::is_directory(vic2_steam_lib_directory, error_code)) {
		vic2_steam_lib_directory /= Victoria_2_folder;
		is_Victoria_2_folder = true;
	}
	if ((is_Victoria_2_folder || filename_equals(Victoria_2_folder, vic2_steam_lib_directory)) &&
		fs::is_regular_file(vic2_steam_lib_directory / v2_game_exe, error_code)) {
		return vic2_steam_lib_directory;
	}

	// Hail Mary check ignoring the hint_path
	if (!hint_path_was_empty)
		return _search_for_game_path();

	Logger::warning("Could not find Victoria 2 game path, this requires manually supplying one.");
	return ""; // The supplied path fits literally none of the criteria
}

fs::path Dataloader::search_for_game_path(fs::path hint_path) {
	auto it = _cached_paths.find(hint_path);
	if (it != _cached_paths.end())
		return it->second;

	return _cached_paths[hint_path] = _search_for_game_path(hint_path);
}

bool Dataloader::set_roots(path_vector_t const& new_roots) {
	if (!roots.empty()) {
		Logger::error("Overriding existing dataloader roots!");
		roots.clear();
	}
	bool ret = true;
	for (std::reverse_iterator<path_vector_t::const_iterator> it = new_roots.crbegin(); it != new_roots.crend(); ++it) {
		if (std::find(roots.begin(), roots.end(), *it) == roots.end()) {
			if (fs::is_directory(*it)) {
				Logger::info("Adding dataloader root: ", *it);
				roots.push_back(*it);
			} else {
				Logger::error("Invalid dataloader root (must be an existing directory): ", *it);
				ret = false;
			}
		} else {
			Logger::error("Duplicate dataloader root: ", *it);
			ret = false;
		}
	}
	if (roots.empty()) {
		Logger::error("Dataloader has no roots after attempting to add ", new_roots.size());
		ret = false;
	}
	return ret;
}

fs::path Dataloader::lookup_file(fs::path const& path) const {
	for (fs::path const& root : roots) {
		const fs::path composed = root / path;
		if (fs::is_regular_file(composed)) {
			return composed;
		}
	}
	Logger::error("Lookup for ", path, " failed!");
	return {};
}

static bool contains_file_with_name(Dataloader::path_vector_t const& paths, fs::path const& name) {
	for (fs::path const& path : paths) {
		if (path.filename() == name) return true;
	}
	return false;
}

Dataloader::path_vector_t Dataloader::lookup_files_in_dir(fs::path const& path, fs::path const& extension) const {
	path_vector_t ret;
	for (fs::path const& root : roots) {
		const fs::path composed = root / path;
		std::error_code ec;
		for (fs::directory_entry const& entry : fs::directory_iterator { composed, ec }) {
			if (entry.is_regular_file()) {
				const fs::path file = entry;
				if (extension.empty() || file.extension() == extension) {
					if (!contains_file_with_name(ret, file.filename())) {
						ret.push_back(file);
					}
				}
			}
		}
	}
	return ret;
}

bool Dataloader::apply_to_files_in_dir(fs::path const& path, fs::path const& extension, callback_t<fs::path const&> callback) const {
	bool ret = true;
	for (fs::path const& file : lookup_files_in_dir(path, extension)) {
		if (!callback(file)) {
			Logger::error("Callback failed for file: ", file);
			ret = false;
		}
	}
	return ret;
}

template<std::derived_from<detail::BasicParser> Parser, bool (*parse_func)(Parser&)>
static Parser _run_ovdl_parser(fs::path const& path) {
	Parser parser;
	std::string buffer;
	auto error_log_stream = detail::CallbackStream {
		[](void const* s, std::streamsize n, void* user_data) -> std::streamsize {
			if (s != nullptr && n > 0 && user_data != nullptr) {
				static_cast<std::string*>(user_data)->append(static_cast<char const*>(s), n);
				return n;
			} else {
				Logger::error("Invalid input to parser error log callback: ", s, " / ", n, " / ", user_data);
				return 0;
			}
		},
		&buffer
	};
	parser.set_error_log_to(error_log_stream);
	parser.load_from_file(path);
	if (!buffer.empty()) {
		Logger::error("Parser load errors for ", path, ":\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error()) {
		Logger::error("Parser errors while loading ", path);
		return parser;
	}
	if (!parse_func(parser)) {
		Logger::error("Parse function returned false for ", path, "!");
	}
	if (!buffer.empty()) {
		Logger::error("Parser parse errors for ", path, ":\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error()) {
		Logger::error("Parser errors while parsing ", path);
	}
	return parser;
}

static bool _v2script_parse(v2script::Parser& parser) {
	return parser.simple_parse();
}

v2script::Parser Dataloader::parse_defines(fs::path const& path) {
	return _run_ovdl_parser<v2script::Parser, &_v2script_parse>(path);
}

static bool _lua_parse(v2script::Parser& parser) {
	return parser.lua_defines_parse();
}

static ovdl::v2script::Parser parse_lua_defines(fs::path const& path) {
	return _run_ovdl_parser<v2script::Parser, &_lua_parse>(path);
}

static bool _csv_parse(csv::Windows1252Parser& parser) {
	return parser.parse_csv();
}

csv::Windows1252Parser Dataloader::parse_csv(fs::path const& path) {
	return _run_ovdl_parser<csv::Windows1252Parser, &_csv_parse>(path);
}

bool Dataloader::_load_pop_types(PopManager& pop_manager, UnitManager const& unit_manager, GoodManager const& good_manager, fs::path const& pop_type_directory) const {
	const bool ret = apply_to_files_in_dir(pop_type_directory, ".txt",
		[&pop_manager, &unit_manager, &good_manager](fs::path const& file) -> bool {
			return pop_manager.load_pop_type_file(file.stem().string(), unit_manager, good_manager, parse_defines(file).get_file_node());
		}
	);
	pop_manager.lock_pop_types();
	return ret;
}

bool Dataloader::_load_units(UnitManager& unit_manager, GoodManager const& good_manager, fs::path const& units_directory) const {
	const bool ret = apply_to_files_in_dir(units_directory, ".txt",
		[&unit_manager, &good_manager](fs::path const& file) -> bool {
			return unit_manager.load_unit_file(good_manager, parse_defines(file).get_file_node());
		}
	);
	unit_manager.lock_units();
	return ret;
}

bool Dataloader::_load_countries(GameManager& game_manager, fs::path const& countries_file, ast::NodeCPtr root) const {
	bool is_dynamic = false;

	bool ret = expect_dictionary(
		[this, &game_manager, &is_dynamic, &countries_file](std::string_view key, ast::NodeCPtr value) -> bool {
			if (key == "dynamic_tags") {
				return expect_bool(assign_variable_callback(is_dynamic))(value);
			}

			std::string_view data_path;

			if (!expect_string(assign_variable_callback(data_path))(value)) {
				return false;
			}

			return game_manager.get_country_manager().load_country_data_file(game_manager, key, is_dynamic, Dataloader::parse_defines(lookup_file(countries_file.parent_path() / data_path)).get_file_node());
		}
	)(root);
	game_manager.get_country_manager().lock_countries();
	return ret;
}

bool Dataloader::_load_map_dir(GameManager& game_manager, fs::path const& map_directory) const {
	Map& map = game_manager.get_map();

	static const fs::path defaults_filename = "default.map";
	static const std::string default_definitions = "definition.csv";
	static const std::string default_provinces = "provinces.bmp";
	static const std::string default_positions = "positions.txt";
	static const std::string default_terrain = "terrain.bmp";
	static const std::string default_rivers = "rivers.bmp";
	static const std::string default_terrain_definition = "terrain.txt";
	static const std::string default_tree_definition = "trees.txt";
	static const std::string default_continent = "continent.txt";
	static const std::string default_adjacencies = "adjacencies.csv";
	static const std::string default_region = "region.txt";
	static const std::string default_region_sea = "region_sea.txt";
	static const std::string default_province_flag_sprite = "province_flag_sprites";

	const v2script::Parser parser = parse_defines(lookup_file(map_directory / defaults_filename));

	std::vector<std::string_view> water_province_identifiers;

#define APPLY_TO_MAP_PATHS(F) \
	F(definitions) F(provinces) F(positions) F(terrain) F(rivers) \
	F(terrain_definition) F(tree_definition) F(continent) F(adjacencies) \
	F(region) F(region_sea) F(province_flag_sprite)

#define MAP_PATH_VAR(X) std::string_view X = default_##X;
	APPLY_TO_MAP_PATHS(MAP_PATH_VAR)
#undef MAP_PATH_VAR

	bool ret = expect_dictionary_keys(
		"max_provinces", ONE_EXACTLY,
		expect_uint<Province::index_t>(
			std::bind(&Map::set_max_provinces, &map, std::placeholders::_1)
		),
		"sea_starts", ONE_EXACTLY,
		expect_list_reserve_length(
			water_province_identifiers,
			expect_identifier(
				[&water_province_identifiers](std::string_view identifier) -> bool {
					water_province_identifiers.push_back(identifier);
					return true;
				}
			)
		),

#define MAP_PATH_DICT_ENTRY(X) \
		#X, ONE_EXACTLY, expect_string(assign_variable_callback(X)),
		APPLY_TO_MAP_PATHS(MAP_PATH_DICT_ENTRY)
#undef MAP_PATH_DICT_ENTRY

#undef APPLY_TO_MAP_PATHS

		"border_heights", ZERO_OR_ONE, success_callback,
		"terrain_sheet_heights", ZERO_OR_ONE, success_callback,
		"tree", ZERO_OR_ONE, success_callback,
		"border_cutoff", ZERO_OR_ONE, success_callback
	)(parser.get_file_node());

	if (!ret) {
		Logger::error("Failed to load map default file!");
	}

	if (!map.load_province_definitions(
		parse_csv(lookup_file(map_directory / definitions)).get_lines())) {
		Logger::error("Failed to load province definitions file!");
		ret = false;
	}

	if (!map.load_province_positions(
		game_manager.get_economy_manager().get_building_manager(), parse_defines(lookup_file(map_directory / positions)).get_file_node())) {
		Logger::error("Failed to load province positions file!");
		ret = false;
	}

	if (!map.load_region_file(
		parse_defines(lookup_file(map_directory / region)).get_file_node())) {
		Logger::error("Failed to load region file!");
		ret = false;
	}

	if (!map.set_water_province_list(water_province_identifiers)) {
		Logger::error("Failed to set water provinces!");
		ret = false;
	}

	if (!map.get_terrain_type_manager().load_terrain_types(
		game_manager.get_modifier_manager(),
		parse_defines(lookup_file(map_directory / terrain_definition)).get_file_node())) {
		Logger::error("Failed to load terrain types!");
		ret = false;
	}

	if (!map.load_map_images(
		lookup_file(map_directory / provinces),
		lookup_file(map_directory / terrain), false)) {
		Logger::error("Failed to load map images!");
		ret = false;
	}

	if (!map.generate_and_load_province_adjacencies(
		parse_csv(lookup_file(map_directory / adjacencies)).get_lines())) {
		Logger::error("Failed to generate and load province adjacencies!");
		ret = false;
	}

	return ret;
}

bool Dataloader::load_defines(GameManager& game_manager) const {
	static const fs::path map_directory = "map";
	static const fs::path pop_type_directory = "poptypes";
	static const fs::path units_directory = "units";

	static const fs::path buildings_file = "common/buildings.txt";
	static const fs::path bookmark_file = "common/bookmarks.txt";
	static const fs::path culture_file = "common/cultures.txt";
	static const fs::path goods_file = "common/goods.txt";
	static const fs::path governments_file = "common/governments.txt";
	static const fs::path graphical_culture_type_file = "common/graphicalculturetype.txt";
	static const fs::path ideology_file = "common/ideologies.txt";
	static const fs::path issues_file = "common/issues.txt";
	static const fs::path production_types_file = "common/production_types.txt";
	static const fs::path religion_file = "common/religion.txt";
	static const fs::path leader_traits_file = "common/traits.txt";
	static const fs::path countries_file = "common/countries.txt";

	bool ret = true;

	if (!game_manager.get_modifier_manager().setup_modifier_effects()) {
		Logger::error("Failed to set up modifier effects!");
		ret = false;
	}
	if (!game_manager.get_economy_manager().get_good_manager().load_goods_file(
		parse_defines(lookup_file(goods_file)).get_file_node())) {
		Logger::error("Failed to load goods!");
		ret = false;
	}
	if (!_load_units(game_manager.get_military_manager().get_unit_manager(),
		game_manager.get_economy_manager().get_good_manager(), units_directory)) {
		Logger::error("Failed to load units!");
		ret = false;
	}
	if (!_load_pop_types(game_manager.get_pop_manager(),
		game_manager.get_military_manager().get_unit_manager(),
		game_manager.get_economy_manager().get_good_manager(),
		pop_type_directory)) {
		Logger::error("Failed to load pop types!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_culture_manager().load_graphical_culture_type_file(
		parse_defines(lookup_file(graphical_culture_type_file)).get_file_node())) {
		Logger::error("Failed to load graphical culture types!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_culture_manager().load_culture_file(
		parse_defines(lookup_file(culture_file)).get_file_node())) {
		Logger::error("Failed to load cultures!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_religion_manager().load_religion_file(
		parse_defines(lookup_file(religion_file)).get_file_node())) {
		Logger::error("Failed to load religions!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().get_ideology_manager().load_ideology_file(
		parse_defines(lookup_file(ideology_file)).get_file_node())) {
		Logger::error("Failed to load ideologies!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().load_government_types_file(
		parse_defines(lookup_file(governments_file)).get_file_node())) {
		Logger::error("Failed to load government types!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().get_issue_manager().load_issues_file(
		parse_defines(lookup_file(issues_file)).get_file_node())) {
		Logger::error("Failed to load issues!");
		ret = false;
	}
	if (!_load_countries(game_manager, countries_file, parse_defines(lookup_file(countries_file)).get_file_node())) {
		Logger::error("Failed to load countries!");
		ret = false;
	}
	if (!game_manager.get_economy_manager().load_production_types_file(
			game_manager.get_pop_manager(),
			parse_defines(lookup_file(production_types_file)).get_file_node())) {
		Logger::error("Failed to load production types!");
		ret = false;
	}
	if (!game_manager.get_economy_manager().load_buildings_file(
		game_manager.get_modifier_manager(),
		parse_defines(lookup_file(buildings_file)).get_file_node())) {
		Logger::error("Failed to load buildings!");
		ret = false;
	}
	if (!_load_map_dir(game_manager, map_directory)) {
		Logger::error("Failed to load map!");
		ret = false;
	}
	if (!game_manager.get_military_manager().get_leader_trait_manager().load_leader_traits_file(game_manager.get_modifier_manager(), parse_defines(lookup_file(leader_traits_file)).get_file_node())) {
		Logger::error("Failed to load leader traits!");
		ret = false;
	}
	if (!game_manager.get_history_manager().load_bookmark_file(parse_defines(lookup_file(bookmark_file)).get_file_node())) {
		Logger::error("Failed to load bookmarks!");
		ret = false;
	}

	return ret;
}

bool Dataloader::load_pop_history(GameManager& game_manager, fs::path const& path) const {
	return apply_to_files_in_dir(path, ".txt",
		[&game_manager](fs::path const& file) -> bool {
			return game_manager.get_map().expect_province_dictionary(
				[&game_manager](Province& province, ast::NodeCPtr value) -> bool {
					return province.load_pop_list(game_manager.get_pop_manager(), value);
				}
			)(parse_defines(file).get_file_node());
		}
	);
}

static bool _load_localisation_file(Dataloader::localisation_callback_t callback, std::vector<csv::LineObject> const& lines) {
	bool ret = true;
	for (csv::LineObject const& line : lines) {
		const std::string_view key = line.get_value_for(0);
		if (!key.empty()) {
			const size_t max_entry = std::min<size_t>(line.value_count() - 1, Dataloader::_LocaleCount);
			for (size_t i = 0; i < max_entry; ++i) {
				const std::string_view entry = line.get_value_for(i + 1);
				if (!entry.empty()) {
					ret &= callback(key, static_cast<Dataloader::locale_t>(i), entry);
				}
			}
		}
	}
	return ret;
}

bool Dataloader::load_localisation_files(localisation_callback_t callback, fs::path const& localisation_dir) const {
	return apply_to_files_in_dir(localisation_dir, ".csv",
		[callback](fs::path path) -> bool {
			return _load_localisation_file(callback, parse_csv(path).get_lines());
		}
	);
}
