#include "Dataloader.hpp"

#include "openvic/GameManager.hpp"
#include "openvic/utility/Logger.hpp"

#include <openvic-dataloader/detail/CallbackOStream.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

using namespace OpenVic;
using namespace ovdl::v2script;

return_t Dataloader::set_roots(std::vector<std::filesystem::path> new_roots) {
	if (!roots.empty()) {
		Logger::error("Overriding existing dataloader roots!");
		roots.clear();
	}
	return_t ret = SUCCESS;
	for (std::reverse_iterator<std::vector<std::filesystem::path>::const_iterator> it = new_roots.crbegin(); it != new_roots.crend(); ++it) {
		if (std::find(roots.begin(), roots.end(), *it) == roots.end()) {
			if (std::filesystem::is_directory(*it)) {
				Logger::info("Adding dataloader root: ", *it);
				roots.push_back(*it);
			} else {
				Logger::error("Invalid dataloader root (must be an existing directory): ", *it);
				ret = FAILURE;
			}
		} else {
			Logger::error("Duplicate dataloader root: ", *it);
			ret = FAILURE;
		}
	}
	if (roots.empty()) {
		Logger::error("Dataloader has no roots after attempting to add ", new_roots.size());
		ret = FAILURE;
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

return_t Dataloader::apply_to_files_in_dir(std::filesystem::path const& path,
	std::function<return_t(std::filesystem::path const&)> callback,
	std::filesystem::path const* extension) const {

	return_t ret = SUCCESS;
	for (std::filesystem::path const& file : lookup_files_in_dir(path, extension)) {
		if (callback(file) != SUCCESS) {
			ret = FAILURE;
		}
	}
	return ret;
}

Parser Dataloader::parse_defines(std::filesystem::path const& path) {
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
	parser.simple_parse();
	if (!buffer.empty()) {
		Logger::error("Parser parse errors:\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error()) {
		Logger::error("Parser errors while parsing ", path);
	}
	return parser;
}

Parser Dataloader::parse_defines_lookup(std::filesystem::path const& path) const {
	return parse_defines(lookup_file(path));
}

return_t Dataloader::_load_pop_types(PopManager& pop_manager, std::filesystem::path const& pop_type_directory) const {
	return_t ret = SUCCESS;
	if (apply_to_files_in_dir(pop_type_directory, [&pop_manager](std::filesystem::path const& file) -> return_t {
		return pop_manager.load_pop_type_file(file, parse_defines(file).get_file_node());
	}) != SUCCESS) {
		Logger::error("Failed to load pop types!");
		ret = FAILURE;
	}
	pop_manager.lock_pop_types();
	return ret;
}

return_t Dataloader::load_defines(GameManager& game_manager) const {
	static const std::filesystem::path good_file = "common/goods.txt";
	static const std::filesystem::path pop_type_directory = "poptypes";
	static const std::filesystem::path graphical_culture_type_file = "common/graphicalculturetype.txt";
	static const std::filesystem::path culture_file = "common/cultures.txt";
	static const std::filesystem::path religion_file = "common/religion.txt";

	return_t ret = SUCCESS;

	if (game_manager.good_manager.load_good_file(parse_defines_lookup(good_file).get_file_node()) != SUCCESS) {
		Logger::error("Failed to load goods!");
		ret = FAILURE;
	}
	if (_load_pop_types(game_manager.pop_manager, pop_type_directory) != SUCCESS) {
		Logger::error("Failed to load pop types!");
		ret = FAILURE;
	}
	if (game_manager.pop_manager.culture_manager.load_graphical_culture_type_file(parse_defines_lookup(graphical_culture_type_file).get_file_node()) != SUCCESS) {
		Logger::error("Failed to load graphical culture types!");
		ret = FAILURE;
	}
	if (game_manager.pop_manager.culture_manager.load_culture_file(parse_defines_lookup(culture_file).get_file_node()) != SUCCESS) {
		Logger::error("Failed to load cultures!");
		ret = FAILURE;
	}
	if (game_manager.pop_manager.religion_manager.load_religion_file(parse_defines_lookup(religion_file).get_file_node()) != SUCCESS) {
		Logger::error("Failed to load religions!");
		ret = FAILURE;
	}

	return ret;
}

return_t Dataloader::load_pop_history(GameManager& game_manager, std::filesystem::path const& path) const {
	return apply_to_files_in_dir(path, [&game_manager](std::filesystem::path const& file) -> return_t {
		return NodeTools::expect_dictionary(parse_defines(file).get_file_node(),
			[&game_manager](std::string_view province_key, ast::NodeCPtr province_node) -> return_t {
				Province* province = game_manager.map.get_province_by_identifier(province_key);
				if (province == nullptr) {
					Logger::error("Invalid province id: ", province_key);
					return FAILURE;
				}
				return NodeTools::expect_list(province_node, [&game_manager, &province](ast::NodeCPtr pop_node) -> return_t {
					return game_manager.pop_manager.load_pop_into_province(*province, pop_node);
				}
			);
		}, true);
	});
}
