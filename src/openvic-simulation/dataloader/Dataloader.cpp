#include "Dataloader.hpp"

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/utility/Logger.hpp"

#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/detail/CallbackOStream.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

using namespace OpenVic;
using namespace OpenVic::NodeTools;
using namespace ovdl;

bool Dataloader::set_roots(std::vector<std::filesystem::path> new_roots) {
	if (!roots.empty()) {
		Logger::error("Overriding existing dataloader roots!");
		roots.clear();
	}
	bool ret = true;
	for (std::reverse_iterator<std::vector<std::filesystem::path>::const_iterator> it = new_roots.crbegin(); it != new_roots.crend(); ++it) {
		if (std::find(roots.begin(), roots.end(), *it) == roots.end()) {
			if (std::filesystem::is_directory(*it)) {
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

std::filesystem::path Dataloader::lookup_file(std::filesystem::path const& path) const {
	for (std::filesystem::path const& root : roots) {
		const std::filesystem::path composed = root / path;
		if (std::filesystem::is_regular_file(composed)) {
			return composed;
		}
	}
	Logger::error("Lookup for ", path, " failed!");
	return {};
}

const std::filesystem::path Dataloader::TXT = ".txt";

static bool contains_file_with_name(std::vector<std::filesystem::path> const& paths,
	std::filesystem::path const& name) {

	for (std::filesystem::path const& path : paths) {
		if (path.filename() == name) return true;
	}
	return false;
}

std::vector<std::filesystem::path> Dataloader::lookup_files_in_dir(std::filesystem::path const& path,
	std::filesystem::path const* extension) const {

	std::vector<std::filesystem::path> ret;
	for (std::filesystem::path const& root : roots) {
		const std::filesystem::path composed = root / path;
		std::error_code ec;
		for (std::filesystem::directory_entry const& entry : std::filesystem::directory_iterator { composed, ec }) {
			if (entry.is_regular_file()) {
				const std::filesystem::path file = entry;
				if (extension == nullptr || file.extension() == *extension) {
					if (!contains_file_with_name(ret, file.filename())) {
						ret.push_back(file);
					}
				}
			}
		}
	}
	return ret;
}

bool Dataloader::apply_to_files_in_dir(std::filesystem::path const& path,
	std::function<bool(std::filesystem::path const&)> callback,
	std::filesystem::path const* extension) const {

	bool ret = true;
	for (std::filesystem::path const& file : lookup_files_in_dir(path, extension)) {
		ret &= callback(file);
	}
	return ret;
}

template<std::derived_from<detail::BasicParser> Parser, bool(Parser::*parse_func)()>
static Parser _run_ovdl_parser(std::filesystem::path const& path) {
	Parser parser;
	std::string buffer;
	auto error_log_stream = ovdl::detail::CallbackStream {
		[](void const* s, std::streamsize n, void* user_data) -> std::streamsize {
			if (s != nullptr && n > 0 && user_data != nullptr) {
				static_cast<std::string*>(user_data)->append(static_cast<char const*>(s), n);
				return n;
			} else {
				Logger::error("Invalid input to parser error log callback: ", s, " / ", n, " / ", user_data);
				return 0;
			}
		}, &buffer
	};
	parser.set_error_log_to(error_log_stream);
	parser.load_from_file(path);
	if (!buffer.empty()) {
		Logger::error("Parser load errors:\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error()) {
		Logger::error("Parser errors while loading ", path);
		return parser;
	}
	if (!(parser.*parse_func)()) {
		Logger::error("Parse function returned false!");
	}
	if (!buffer.empty()) {
		Logger::error("Parser parse errors:\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error()) {
		Logger::error("Parser errors while parsing ", path);
	}
	return parser;
}

static v2script::Parser _parse_defines(std::filesystem::path const& path) {
	return _run_ovdl_parser<v2script::Parser, &v2script::Parser::simple_parse>(path);
}

static csv::Windows1252Parser _parse_csv(std::filesystem::path const& path) {
	return _run_ovdl_parser<csv::Windows1252Parser, &csv::Windows1252Parser::parse_csv>(path);
}

bool Dataloader::_load_pop_types(PopManager& pop_manager, std::filesystem::path const& pop_type_directory) const {
	const bool ret = apply_to_files_in_dir(pop_type_directory,
		[&pop_manager](std::filesystem::path const& file) -> bool {
			return pop_manager.load_pop_type_file(file, _parse_defines(file).get_file_node());
		}
	);
	if (!ret) {
		Logger::error("Failed to load pop types!");
	}
	pop_manager.lock_pop_types();
	return ret;
}

bool Dataloader::_load_map_dir(Map& map, std::filesystem::path const& map_directory) const {
	static const std::filesystem::path defaults_filename = "default.map";
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

	const v2script::Parser parser = _parse_defines(lookup_file(map_directory / defaults_filename));

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
			expect_uint(
				[&map](uint64_t val) -> bool {
					if (Province::NULL_INDEX < val && val <= Province::MAX_INDEX) {
						return map.set_max_provinces(val);
					}
					Logger::error("Invalid max province count ", val, " (out of valid range ", Province::NULL_INDEX, " < max_provinces <= ", Province::MAX_INDEX, ")");
					return false;
				}
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

	if (!map.load_province_definitions(_parse_csv(lookup_file(map_directory / definitions)).get_lines())) {
		Logger::error("Failed to load province definitions file!");
		ret = false;
	}

	if (!map.set_water_province_list(water_province_identifiers)) {
		Logger::error("Failed to set water provinces!");
		ret = false;
	}
	map.lock_water_provinces();

	return ret;
}

bool Dataloader::load_defines(GameManager& game_manager) const {
	static const std::filesystem::path good_file = "common/goods.txt";
	static const std::filesystem::path pop_type_directory = "poptypes";
	static const std::filesystem::path graphical_culture_type_file = "common/graphicalculturetype.txt";
	static const std::filesystem::path culture_file = "common/cultures.txt";
	static const std::filesystem::path religion_file = "common/religion.txt";
	static const std::filesystem::path map_directory = "map";

	bool ret = true;

	if (!game_manager.good_manager.load_good_file(_parse_defines(lookup_file(good_file)).get_file_node())) {
		Logger::error("Failed to load goods!");
		ret = false;
	}
	if (!_load_pop_types(game_manager.pop_manager, pop_type_directory)) {
		Logger::error("Failed to load pop types!");
		ret = false;
	}
	if (!game_manager.pop_manager.culture_manager.load_graphical_culture_type_file(_parse_defines(lookup_file(graphical_culture_type_file)).get_file_node())) {
		Logger::error("Failed to load graphical culture types!");
		ret = false;
	}
	if (!game_manager.pop_manager.culture_manager.load_culture_file(_parse_defines(lookup_file(culture_file)).get_file_node())) {
		Logger::error("Failed to load cultures!");
		ret = false;
	}
	if (!game_manager.pop_manager.religion_manager.load_religion_file(_parse_defines(lookup_file(religion_file)).get_file_node())) {
		Logger::error("Failed to load religions!");
		ret = false;
	}
	if (!_load_map_dir(game_manager.map, map_directory)) {
		Logger::error("Failed to load map!");
		ret = false;
	}

	return ret;
}

bool Dataloader::load_pop_history(GameManager& game_manager, std::filesystem::path const& path) const {
	return apply_to_files_in_dir(path,
		[&game_manager](std::filesystem::path const& file) -> bool {
			return expect_dictionary(
				[&game_manager](std::string_view province_key, ast::NodeCPtr province_node) -> bool {
					Province* province = game_manager.map.get_province_by_identifier(province_key);
					if (province == nullptr) {
						Logger::error("Invalid province id: ", province_key);
						return false;
					}
					return province->load_pop_list(game_manager.pop_manager, province_node);
				}
			)(_parse_defines(file).get_file_node());
		}
	);
}
