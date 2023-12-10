#include "Dataloader.hpp"

#include <filesystem>
#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/detail/CallbackOStream.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <lexy-vdf/KeyValues.hpp>
#include <lexy-vdf/Parser.hpp>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/utility/ConstexprIntToStr.hpp"
#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using namespace ovdl;

using StringUtils::append_string_views;

#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__))
#define FILESYSTEM_CASE_INSENSITIVE
#endif

#if !defined(_WIN32)
#define FILESYSTEM_NEEDS_FORWARD_SLASHES
#endif

static constexpr bool path_equals_case_insensitive(std::string_view lhs, std::string_view rhs) {
	constexpr auto ichar_equals = [](unsigned char l, unsigned char r) {
		return std::tolower(l) == std::tolower(r);
	};
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), ichar_equals);
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

fs::path Dataloader::lookup_file(std::string_view path, bool print_error) const {
#if defined(FILESYSTEM_NEEDS_FORWARD_SLASHES)
	/* Back-slashes need to be converted into forward-slashes */
	const std::string forward_slash_path { StringUtils::make_forward_slash_path(StringUtils::remove_leading_slashes(path)) };
	path = forward_slash_path;
#endif

	const fs::path filepath { path };

#if defined(FILESYSTEM_CASE_INSENSITIVE)
	/* Case-insensitive filesystem */
	for (fs::path const& root : roots) {
		const fs::path composed = root / filepath;
		if (fs::is_regular_file(composed)) {
			return composed;
		}
	}
#else
	/* Case-sensitive filesystem */
	const std::string_view filename = StringUtils::get_filename(path);
	for (fs::path const& root : roots) {
		const fs::path composed = root / filepath;
		if (fs::is_regular_file(composed)) {
			return composed;
		}
		std::error_code ec;
		for (fs::directory_entry const& entry : fs::directory_iterator { composed.parent_path(), ec }) {
			if (entry.is_regular_file()) {
				const fs::path file = entry;
				if (path_equals_case_insensitive(file.filename().string(), filename)) {
					return file;
				}
			}
		}
	}
#endif

	if (print_error) {
		Logger::error("Lookup for \"", path, "\" failed!");
	}
	return {};
}

fs::path Dataloader::lookup_image_file(std::string_view path) const {
	const std::string_view path_without_extension = StringUtils::remove_extension(path);
	if (path.substr(path_without_extension.size()) == ".tga") {
		const fs::path ret = lookup_file(append_string_views(path_without_extension, ".dds"), false);
		if (!ret.empty()) {
			return ret;
		}
	}
	return lookup_file(path);
}

template<typename _DirIterator, typename _UniqueKey>
requires requires (_UniqueKey const& unique_key, std::string_view path) {
	{ unique_key(path) } -> std::convertible_to<std::string_view>;
}
Dataloader::path_vector_t Dataloader::_lookup_files_in_dir(
	std::string_view path, fs::path const& extension, _UniqueKey const& unique_key
) const {
#if defined(FILESYSTEM_NEEDS_FORWARD_SLASHES)
	/* Back-slashes need to be converted into forward-slashes */
	const std::string forward_slash_path { StringUtils::make_forward_slash_path(StringUtils::remove_leading_slashes(path)) };
	path = forward_slash_path;
#endif
	const fs::path dirpath { path };
	path_vector_t ret;
	struct file_entry_t {
		fs::path file;
		fs::path const* root;
	};
	string_map_t<file_entry_t> found_files;
	for (fs::path const& root : roots) {
		const size_t root_len = root.string().size();
		const fs::path composed = root / dirpath;
		std::error_code ec;
		for (fs::directory_entry const& entry : _DirIterator { composed, ec }) {
			if (entry.is_regular_file()) {
				fs::path file = entry;
				if ((extension.empty() || file.extension() == extension)) {
					const std::string full_path = file.string();
					std::string_view relative_path = full_path;
					relative_path.remove_prefix(root_len);
					relative_path = StringUtils::remove_leading_slashes(relative_path);
					const std::string_view key = unique_key(relative_path);
					if (!key.empty()) {
						const typename decltype(found_files)::const_iterator it = found_files.find(key);
						if (it == found_files.end()) {
							found_files.emplace(key, file_entry_t { file, &root });
							ret.emplace_back(std::move(file));
						} else if (it->second.root == &root) {
							Logger::warning(
								"Files in the same directory with conflicting keys: ", it->first, " - ", it->second.file,
								" (accepted) and ", key, " - ", file, " (rejected)"
							);
						}
					}
				}
			}
		}
	}
	return ret;
}

Dataloader::path_vector_t Dataloader::lookup_files_in_dir(std::string_view path, fs::path const& extension) const {
	return _lookup_files_in_dir<fs::directory_iterator>(path, extension, std::identity {});
}

Dataloader::path_vector_t Dataloader::lookup_files_in_dir_recursive(std::string_view path, fs::path const& extension) const {
	return _lookup_files_in_dir<fs::recursive_directory_iterator>(path, extension, std::identity {});
}

static std::string_view _extract_basic_identifier_prefix_from_path(std::string_view path) {
	return extract_basic_identifier_prefix(StringUtils::get_filename(path));
};

Dataloader::path_vector_t Dataloader::lookup_basic_indentifier_prefixed_files_in_dir(
	std::string_view path, fs::path const& extension
) const {
	return _lookup_files_in_dir<fs::directory_iterator>(path, extension, _extract_basic_identifier_prefix_from_path);
}

Dataloader::path_vector_t Dataloader::lookup_basic_indentifier_prefixed_files_in_dir_recursive(
	std::string_view path, fs::path const& extension
) const {
	return _lookup_files_in_dir<fs::recursive_directory_iterator>(path, extension, _extract_basic_identifier_prefix_from_path);
}

bool Dataloader::apply_to_files(path_vector_t const& files, callback_t<fs::path const&> callback) const {
	bool ret = true;
	for (fs::path const& file : files) {
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

ovdl::v2script::Parser Dataloader::parse_lua_defines(fs::path const& path) {
	return _run_ovdl_parser<v2script::Parser, &_lua_parse>(path);
}

static bool _csv_parse(csv::Windows1252Parser& parser) {
	return parser.parse_csv();
}

csv::Windows1252Parser Dataloader::parse_csv(fs::path const& path) {
	return _run_ovdl_parser<csv::Windows1252Parser, &_csv_parse>(path);
}

bool Dataloader::_load_interface_files(UIManager& ui_manager) const {
	static constexpr std::string_view interface_directory = "interface/";

	bool ret = apply_to_files(
		lookup_files_in_dir(interface_directory, ".gfx"),
		[&ui_manager](fs::path const& file) -> bool {
			return ui_manager.load_gfx_file(parse_defines(file).get_file_node());
		}
	);
	ui_manager.lock_sprites();
	ui_manager.lock_fonts();

	// Hard-coded example until the mechanism for requesting them from GDScript is fleshed out
	static const std::vector<std::string_view> gui_files {
		"province_interface.gui", "topbar.gui"
	};
	for (std::string_view const& gui_file : gui_files) {
		if (!ui_manager.load_gui_file(
			gui_file,
			parse_defines(lookup_file(append_string_views(interface_directory, gui_file))).get_file_node()
		)) {
			Logger::error("Failed to load interface gui file: ", gui_file);
			ret = false;
		}
	}
	ui_manager.lock_scenes();

	return ret;
}

bool Dataloader::_load_pop_types(
	PopManager& pop_manager, UnitManager const& unit_manager, GoodManager const& good_manager
) const {
	static constexpr std::string_view pop_type_directory = "poptypes";
	const bool ret = apply_to_files(
		lookup_files_in_dir(pop_type_directory, ".txt"),
		[&pop_manager, &unit_manager, &good_manager](fs::path const& file) -> bool {
			return pop_manager.load_pop_type_file(
				file.stem().string(), unit_manager, good_manager, parse_defines(file).get_file_node()
			);
		}
	);
	pop_manager.lock_pop_types();
	return ret;
}

bool Dataloader::_load_units(GameManager& game_manager) const {
	static constexpr std::string_view units_directory = "units";

	UnitManager& unit_manager = game_manager.get_military_manager().get_unit_manager();
	bool ret = apply_to_files(
		lookup_files_in_dir(units_directory, ".txt"),
		[&game_manager, &unit_manager](fs::path const& file) -> bool {
			return unit_manager.load_unit_file(game_manager.get_economy_manager().get_good_manager(), parse_defines(file).get_file_node());
		}
	);

	unit_manager.lock_units();

	if(!unit_manager.generate_modifiers(game_manager.get_modifier_manager())) {
		Logger::error("Failed to generate unit-based modifiers!");
		ret = false;
	}

	return ret;
}

bool Dataloader::_load_goods(GameManager& game_manager) const {
	static constexpr std::string_view goods_file = "common/goods.txt";

	GoodManager& good_manager = game_manager.get_economy_manager().get_good_manager();
	bool ret = good_manager.load_goods_file(parse_defines(lookup_file(goods_file)).get_file_node());

	if(!good_manager.generate_modifiers(game_manager.get_modifier_manager())) {
		Logger::error("Failed to generate good-based modifiers!");
		ret = false;
	}

	return ret;
}

bool Dataloader::_load_rebel_types(GameManager& game_manager) const {
	static constexpr std::string_view rebel_types_file = "common/rebel_types.txt";

	PoliticsManager& politics_manager = game_manager.get_politics_manager();
	RebelManager& rebel_manager = politics_manager.get_rebel_manager();

	bool ret = politics_manager.load_rebels_file(parse_defines(lookup_file(rebel_types_file)).get_file_node());

	if(!rebel_manager.generate_modifiers(game_manager.get_modifier_manager())) {
		Logger::error("Failed to generate rebel type-based modifiers!");
		ret &= false;
	}

	return ret;
}

bool Dataloader::_load_technologies(GameManager& game_manager) const {
	static constexpr std::string_view technology_file = "common/technology.txt";

	TechnologyManager& technology_manager = game_manager.get_research_manager().get_technology_manager();

	bool ret = true;

	const v2script::Parser technology_file_parser = parse_defines(lookup_file(technology_file));

	if(!technology_manager.load_technology_file_areas(technology_file_parser.get_file_node())) {
		Logger::error("Failed to load technology areas and folders!");
		ret = false;
	}

	ModifierManager& modifier_manager = game_manager.get_modifier_manager();

	if(!technology_manager.generate_modifiers(modifier_manager)) {
		Logger::error("Failed to generate technollogy-based modifiers!");
		ret = false;
	}

	if(!technology_manager.load_technology_file_schools(modifier_manager, technology_file_parser.get_file_node())) {
		Logger::error("Failed to load technology schools!");
		ret = false;
	}

	static constexpr std::string_view technologies_directory = "technologies";
	if(!apply_to_files(
		lookup_files_in_dir(technologies_directory, ".txt"),
		[&game_manager, &technology_manager, &modifier_manager](fs::path const& file) -> bool {
			return technology_manager.load_technologies_file(
				modifier_manager,
				game_manager.get_military_manager().get_unit_manager(),
				game_manager.get_economy_manager().get_building_type_manager(),
				parse_defines(file).get_file_node()
			);
		}
	)) {
		Logger::error("Failed to load technologies!");
		ret = false;
	}

	technology_manager.lock_technologies();
	return ret;
}

bool Dataloader::_load_inventions(GameManager& game_manager) const {
	static constexpr std::string_view inventions_directory = "inventions";

	InventionManager& invention_manager = game_manager.get_research_manager().get_invention_manager();

	bool ret = apply_to_files(
		lookup_files_in_dir(inventions_directory, ".txt"),
		[&game_manager, &invention_manager](fs::path const& file) -> bool {
			return invention_manager.load_inventions_file(
				game_manager.get_modifier_manager(),
				game_manager.get_military_manager().get_unit_manager(),
				game_manager.get_economy_manager().get_building_type_manager(),
				game_manager.get_crime_manager(),
				parse_defines(file).get_file_node()
			);
		}
	);

	invention_manager.lock_inventions();

	return ret;
}

bool Dataloader::_load_decisions(GameManager& game_manager) const {
	static constexpr std::string_view decisions_directory = "decisions";
	DecisionManager& decision_manager = game_manager.get_decision_manager();

	bool ret = apply_to_files(
		lookup_files_in_dir(decisions_directory, ".txt"),
		[&decision_manager](fs::path const& file) -> bool {
			return decision_manager.load_decision_file(parse_defines(file).get_file_node());
		}
	);

	decision_manager.lock_decisions();

	return ret;
}

bool Dataloader::_load_history(GameManager& game_manager, bool unused_history_file_warnings) const {

	/* Country History */
	static constexpr std::string_view country_history_directory = "history/countries";
	bool ret = apply_to_files(
		lookup_basic_indentifier_prefixed_files_in_dir(country_history_directory, ".txt"),
		[this, &game_manager, unused_history_file_warnings](fs::path const& file) -> bool {
			const std::string filename = file.stem().string();
			const std::string_view country_id = extract_basic_identifier_prefix(filename);

			Country const* country = game_manager.get_country_manager().get_country_by_identifier(country_id);
			if (country == nullptr) {
				if (unused_history_file_warnings) {
					Logger::warning("Found history file for non-existent country: ", country_id);
				}
				return true;
			}

			return game_manager.get_history_manager().get_country_manager().load_country_history_file(
				game_manager, *this, *country, parse_defines(file).get_file_node()
			);
		}
	);
	game_manager.get_history_manager().get_country_manager().lock_country_histories();

	{
		DeploymentManager& deployment_manager = game_manager.get_military_manager().get_deployment_manager();
		deployment_manager.lock_deployments();
		if (deployment_manager.get_missing_oob_file_count() > 0) {
			Logger::warning(deployment_manager.get_missing_oob_file_count(), " missing OOB files!");
		}
	}

	/* Province History */
	static constexpr std::string_view province_history_directory = "history/provinces";
	ret &= apply_to_files(
		lookup_basic_indentifier_prefixed_files_in_dir_recursive(province_history_directory, ".txt"),
		[this, &game_manager, unused_history_file_warnings](fs::path const& file) -> bool {
			const std::string filename = file.stem().string();
			const std::string_view province_id = extract_basic_identifier_prefix(filename);

			Province const* province = game_manager.get_map().get_province_by_identifier(province_id);
			if (province == nullptr) {
				if (unused_history_file_warnings) {
					Logger::warning("Found history file for non-existent province: ", province_id);
				}
				return true;
			}

			return game_manager.get_history_manager().get_province_manager().load_province_history_file(
				game_manager, *province, parse_defines(file).get_file_node()
			);
		}
	);

	/* Pop History */
	static constexpr std::string_view pop_history_directory = "history/pops";
	for (fs::path root : roots) {
		fs::path concat = root / pop_history_directory;
		for (fs::path dir : fs::directory_iterator(concat)) {
			const Date date = Date::from_string(dir.filename().string());
			ret &= apply_to_files(
				lookup_basic_indentifier_prefixed_files_in_dir_recursive(dir.string(), ".txt"),
				[this, &game_manager, &date](fs::path const& file) -> bool {
					const std::string filename = file.stem().string();
					return game_manager.get_history_manager().get_province_manager().load_pop_history_file(
						game_manager, date, parse_defines(file).get_file_node()
					);
				}
			);
		}
	}

	game_manager.get_history_manager().get_province_manager().lock_province_histories(game_manager.get_map(), false);

	static constexpr std::string_view diplomacy_history_directory = "history/diplomacy";
	ret &= apply_to_files(
		lookup_files_in_dir(diplomacy_history_directory, ".txt"),
		[this, &game_manager](fs::path const& file) -> bool {
			return game_manager.get_history_manager().get_diplomacy_manager().load_diplomacy_history_file(
				game_manager.get_country_manager(), parse_defines(file).get_file_node()
			);
		}
	);
	static constexpr std::string_view war_history_directory = "history/wars";
	ret &= apply_to_files(
		lookup_files_in_dir(war_history_directory, ".txt"),
		[this, &game_manager](fs::path const& file) -> bool {
			return game_manager.get_history_manager().get_diplomacy_manager().load_war_history_file(game_manager, parse_defines(file).get_file_node());
		}
	);
	game_manager.get_history_manager().get_diplomacy_manager().lock_diplomatic_history();

	return ret;
}

bool Dataloader::_load_events(GameManager& game_manager) const {
	static constexpr std::string_view events_directory = "events";
	const bool ret = apply_to_files(
		lookup_files_in_dir(events_directory, ".txt"),
		[&game_manager](fs::path const& file) -> bool {
			return game_manager.get_event_manager().load_event_file(
				game_manager.get_politics_manager().get_issue_manager(), parse_defines(file).get_file_node()
			);
		}
	);
	game_manager.get_event_manager().lock_events();
	return ret;
}

bool Dataloader::_load_map_dir(GameManager& game_manager) const {
	static constexpr std::string_view map_directory = "map/";
	Map& map = game_manager.get_map();

	static constexpr std::string_view defaults_filename = "default.map";
	static constexpr std::string_view default_definitions = "definition.csv";
	static constexpr std::string_view default_provinces = "provinces.bmp";
	static constexpr std::string_view default_positions = "positions.txt";
	static constexpr std::string_view default_terrain = "terrain.bmp";
	static constexpr std::string_view default_rivers = "rivers.bmp";
	static constexpr std::string_view default_terrain_definition = "terrain.txt";
	static constexpr std::string_view default_tree_definition = "trees.txt";
	static constexpr std::string_view default_continent = "continent.txt";
	static constexpr std::string_view default_adjacencies = "adjacencies.csv";
	static constexpr std::string_view default_region = "region.txt";
	static constexpr std::string_view default_region_sea = "region_sea.txt";
	static constexpr std::string_view default_province_flag_sprite = "province_flag_sprites";

	const v2script::Parser parser = parse_defines(lookup_file(append_string_views(map_directory, defaults_filename)));

	std::vector<std::string_view> water_province_identifiers;

#define APPLY_TO_MAP_PATHS(F) \
	F(definitions) \
	F(provinces) \
	F(positions) \
	F(terrain) \
	F(rivers) \
	F(terrain_definition) \
	F(tree_definition) \
	F(continent) \
	F(adjacencies) \
	F(region) \
	F(region_sea) \
	F(province_flag_sprite)

#define MAP_PATH_VAR(X) std::string_view X = default_##X;
	APPLY_TO_MAP_PATHS(MAP_PATH_VAR)
#undef MAP_PATH_VAR

	bool ret = expect_dictionary_keys(
		"max_provinces", ONE_EXACTLY,
			expect_uint<Province::index_t>(std::bind_front(&Map::set_max_provinces, &map)),
		"sea_starts", ONE_EXACTLY,
			expect_list_reserve_length(
				water_province_identifiers, expect_identifier(vector_callback(water_province_identifiers))
			),

#define MAP_PATH_DICT_ENTRY(X) #X, ONE_EXACTLY, expect_string(assign_variable_callback(X)),
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

	if (!map.load_province_definitions(parse_csv(lookup_file(append_string_views(map_directory, definitions))).get_lines())) {
		Logger::error("Failed to load province definitions file!");
		ret = false;
	}

	if (!map.load_province_positions(
		game_manager.get_economy_manager().get_building_type_manager(),
		parse_defines(lookup_file(append_string_views(map_directory, positions))).get_file_node()
	)) {
		Logger::error("Failed to load province positions file!");
		ret = false;
	}

	if (!map.load_region_file(parse_defines(lookup_file(append_string_views(map_directory, region))).get_file_node())) {
		Logger::error("Failed to load region file!");
		ret = false;
	}

	if (!map.set_water_province_list(water_province_identifiers)) {
		Logger::error("Failed to set water provinces!");
		ret = false;
	}

	if (!map.get_terrain_type_manager().load_terrain_types(
		game_manager.get_modifier_manager(),
		parse_defines(lookup_file(append_string_views(map_directory, terrain_definition))).get_file_node()
	)) {
		Logger::error("Failed to load terrain types!");
		ret = false;
	}

	if (!map.load_map_images(
		lookup_file(append_string_views(map_directory, provinces)),
		lookup_file(append_string_views(map_directory, terrain)), false
	)) {
		Logger::error("Failed to load map images!");
		ret = false;
	}

	if (!map.generate_and_load_province_adjacencies(
		parse_csv(lookup_file(append_string_views(map_directory, adjacencies))).get_lines()
	)) {
		Logger::error("Failed to generate and load province adjacencies!");
		ret = false;
	}

	return ret;
}

bool Dataloader::load_defines(GameManager& game_manager) const {
	static constexpr std::string_view defines_file = "common/defines.lua";
	static constexpr std::string_view buildings_file = "common/buildings.txt";
	static constexpr std::string_view bookmark_file = "common/bookmarks.txt";
	static constexpr std::string_view countries_file = "common/countries.txt";
	static constexpr std::string_view culture_file = "common/cultures.txt";
	static constexpr std::string_view governments_file = "common/governments.txt";
	static constexpr std::string_view graphical_culture_type_file = "common/graphicalculturetype.txt";
	static constexpr std::string_view ideology_file = "common/ideologies.txt";
	static constexpr std::string_view issues_file = "common/issues.txt";
	static constexpr std::string_view national_foci_file = "common/national_focus.txt";
	static constexpr std::string_view national_values_file = "common/nationalvalues.txt";
	static constexpr std::string_view production_types_file = "common/production_types.txt";
	static constexpr std::string_view religion_file = "common/religion.txt";
	static constexpr std::string_view leader_traits_file = "common/traits.txt";
	static constexpr std::string_view cb_types_file = "common/cb_types.txt";
	static constexpr std::string_view crime_modifiers_file = "common/crime.txt";
	static constexpr std::string_view event_modifiers_file = "common/event_modifiers.txt";
	static constexpr std::string_view static_modifiers_file = "common/static_modifiers.txt";
	static constexpr std::string_view triggered_modifiers_file = "common/triggered_modifiers.txt";

	bool ret = true;

	if (!_load_interface_files(game_manager.get_ui_manager())) {
		Logger::error("Failed to load interface files!");
		ret = false;
	}
	if (!game_manager.get_modifier_manager().setup_modifier_effects()) {
		Logger::error("Failed to set up modifier effects!");
		ret = false;
	}
	if (!game_manager.get_define_manager().load_defines_file(parse_lua_defines(lookup_file(defines_file)).get_file_node())) {
		Logger::error("Failed to load defines!");
		ret = false;
	}
	if (!_load_goods(game_manager)) {
		Logger::error("Failed to load goods!");
		ret = false;
	}
	if (!_load_units(game_manager)) {
		Logger::error("Failed to load units!");
		ret = false;
	}
	if (!_load_pop_types(
		game_manager.get_pop_manager(), game_manager.get_military_manager().get_unit_manager(),
		game_manager.get_economy_manager().get_good_manager()
	)) {
		Logger::error("Failed to load pop types!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_culture_manager().load_graphical_culture_type_file(
		parse_defines(lookup_file(graphical_culture_type_file)).get_file_node()
	)) {
		Logger::error("Failed to load graphical culture types!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_culture_manager().load_culture_file(
		parse_defines(lookup_file(culture_file)).get_file_node()
	)) {
		Logger::error("Failed to load cultures!");
		ret = false;
	}
	if (!game_manager.get_pop_manager().get_religion_manager().load_religion_file(
		parse_defines(lookup_file(religion_file)).get_file_node()
	)) {
		Logger::error("Failed to load religions!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().get_ideology_manager().load_ideology_file(
		parse_defines(lookup_file(ideology_file)).get_file_node()
	)) {
		Logger::error("Failed to load ideologies!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().load_government_types_file(
		parse_defines(lookup_file(governments_file)).get_file_node()
	)) {
		Logger::error("Failed to load government types!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().get_issue_manager().load_issues_file(
		parse_defines(lookup_file(issues_file)).get_file_node()
	)) {
		Logger::error("Failed to load issues!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().load_national_foci_file(
		game_manager.get_pop_manager(), game_manager.get_economy_manager().get_good_manager(), game_manager.get_modifier_manager(), parse_defines(lookup_file(national_foci_file)).get_file_node()
	)) {
		Logger::error("Failed to load national foci!");
		ret = false;
	}
	if (!game_manager.get_politics_manager().get_national_value_manager().load_national_values_file(
		game_manager.get_modifier_manager(), parse_defines(lookup_file(national_values_file)).get_file_node()
	)) {
		Logger::error("Failed to load national values!");
		ret = false;
	}
	if (!game_manager.get_economy_manager().load_production_types_file(game_manager.get_pop_manager(),
		parse_defines(lookup_file(production_types_file)).get_file_node()
	)) {
		Logger::error("Failed to load production types!");
		ret = false;
	}
	if (!game_manager.get_economy_manager().load_buildings_file(game_manager.get_modifier_manager(),
		parse_defines(lookup_file(buildings_file)).get_file_node()
	)) {
		Logger::error("Failed to load buildings!");
		ret = false;
	}
	if (!_load_rebel_types(game_manager)) {
		Logger::error("Failed to load rebel types!");
		ret = false;
	}
	if (!_load_technologies(game_manager)) {
		ret = false;
	}
	if (!game_manager.get_crime_manager().load_crime_modifiers(
		game_manager.get_modifier_manager(), parse_defines(lookup_file(crime_modifiers_file)).get_file_node()
	)) {
		Logger::error("Failed to load crime modifiers!");
		ret = false;
	}
	if (!game_manager.get_modifier_manager().load_event_modifiers(
		parse_defines(lookup_file(event_modifiers_file)).get_file_node()
	)) {
		Logger::error("Failed to load event modifiers!");
		ret = false;
	}
	if (!game_manager.get_modifier_manager().load_static_modifiers(
		parse_defines(lookup_file(static_modifiers_file)).get_file_node()
	)) {
		Logger::error("Failed to load static modifiers!");
		ret = false;
	}
	if (!game_manager.get_modifier_manager().load_triggered_modifiers(
		parse_defines(lookup_file(triggered_modifiers_file)).get_file_node()
	)) {
		Logger::error("Failed to load triggered modifiers!");
		ret = false;
	}
	if (!_load_inventions(game_manager)) {
		Logger::error("Failed to load inventions!");
		ret = false;
	}
	if (!_load_map_dir(game_manager)) {
		Logger::error("Failed to load map!");
		ret = false;
	}
	if (!game_manager.get_military_manager().get_leader_trait_manager().load_leader_traits_file(
		game_manager.get_modifier_manager(), parse_defines(lookup_file(leader_traits_file)).get_file_node()
	)) {
		Logger::error("Failed to load leader traits!");
		ret = false;
	}
	if (!game_manager.get_military_manager().get_wargoal_manager().load_wargoal_file(
		parse_defines(lookup_file(cb_types_file)).get_file_node()
	)) {
		Logger::error("Failed to load wargoals!");
		ret = false;
	}
	if (!game_manager.get_history_manager().get_bookmark_manager().load_bookmark_file(
		parse_defines(lookup_file(bookmark_file)).get_file_node()
	)) {
		Logger::error("Failed to load bookmarks!");
		ret = false;
	}
	if (!game_manager.get_country_manager().load_countries(
		game_manager, *this, parse_defines(lookup_file(countries_file)).get_file_node()
	)) {
		Logger::error("Failed to load countries!");
		ret = false;
	}
	if (!_load_decisions(game_manager)) {
		Logger::error("Failde to load decisions!");
		ret = false;
	}
	if (!_load_history(game_manager, false)) {
		Logger::error("Failed to load history!");
		ret = false;
	}
	if (!_load_events(game_manager)) {
		Logger::error("Failed to load events!");
		ret = false;
	}

	return ret;
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

bool Dataloader::load_localisation_files(localisation_callback_t callback, std::string_view localisation_dir) const {
	return apply_to_files(
		lookup_files_in_dir(localisation_dir, ".csv"),
		[callback](fs::path path) -> bool {
			return _load_localisation_file(callback, parse_csv(path).get_lines());
		}
	);
}
