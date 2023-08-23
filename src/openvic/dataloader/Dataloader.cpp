#include "Dataloader.hpp"

#include "openvic/utility/Logger.hpp"
#include "openvic//GameManager.hpp"
#include "openvic/dataloader/NodeTools.hpp"

#include <openvic-dataloader/detail/CallbackOStream.hpp>

using namespace OpenVic;
using namespace ovdl::v2script;

return_t Dataloader::set_roots(std::vector<std::filesystem::path> new_roots) {
	if (!roots.empty()) {
		Logger::error("Overriding existing dataloader roots!");
		roots.clear();
	}
	for (std::reverse_iterator<std::vector<std::filesystem::path>::const_iterator> it = new_roots.crbegin(); it != new_roots.crend(); ++it) {
		if (std::filesystem::is_directory(*it)) {
			Logger::info("Adding dataloader root: ", *it);
			roots.push_back(*it);
		} else {
			Logger::error("Invalid dataloader root (must be an existing directory): ", *it);
		}
	}
	if (roots.empty()) {
		Logger::error("Dataloader has no roots after attempting to add ", new_roots.size());
		return FAILURE;
	}
	return SUCCESS;
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

static bool contains_file_with_name(std::vector<std::filesystem::path> const& paths, std::filesystem::path const& name) {
	for (std::filesystem::path const& path : paths) {
		if (path.filename() == name) return true;
	}
	return false;
}

std::vector<std::filesystem::path> Dataloader::lookup_files_in_dir(std::filesystem::path const& path) const {
	std::vector<std::filesystem::path> ret;
	for (std::filesystem::path const& root : roots) {
		const std::filesystem::path composed = root / path;
		std::error_code ec;
		for (std::filesystem::directory_entry const& entry : std::filesystem::directory_iterator { composed, ec }) {
			if (entry.is_regular_file() && !contains_file_with_name(ret, entry.path().filename())) {
				ret.push_back(entry);
			}
		}
	}
	return ret;
}

static Parser parse_defines(std::filesystem::path const& path) {
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
	if (parser.has_fatal_error() || parser.has_error() || parser.has_warning()) {
		Logger::error("Parser warnings/errors while loading ", path);
		return parser;
	}
	parser.simple_parse();
	if (!buffer.empty()) {
		Logger::error("Parser parse errors:\n\n", buffer, "\n");
		buffer.clear();
	}
	if (parser.has_fatal_error() || parser.has_error() || parser.has_warning()) {
		Logger::error("Parser warnings/errors while parsing ", path);
	}
	return parser;
}

return_t Dataloader::load_defines(GameManager& game_manager) const {
	static const std::filesystem::path graphical_culture_type_file = "common/graphicalculturetype.txt";
	static const std::filesystem::path culture_file = "common/cultures.txt";

	return_t ret = SUCCESS;

	if (game_manager.pop_manager.culture_manager.load_graphical_culture_type_file(parse_defines(
		lookup_file(graphical_culture_type_file)).get_file_node()) != SUCCESS) {
		Logger::error("Failed to load graphical culture types!");
		ret = FAILURE;
	}
	if (game_manager.pop_manager.culture_manager.load_culture_file(parse_defines(
		lookup_file(culture_file)).get_file_node()) != SUCCESS) {
		Logger::error("Failed to load cultures!");
		ret = FAILURE;
	}

	return ret;
}

static return_t load_pop_history_file(GameManager& game_manager, std::filesystem::path const& path) {
	return NodeTools::expect_dictionary(parse_defines(path).get_file_node(), [&game_manager](std::string_view province_key, ast::NodeCPtr province_node) -> return_t {
		Province* province = game_manager.map.get_province_by_identifier(province_key);
		if (province == nullptr) {
			Logger::error("Invalid province id: ", province_key);
			return FAILURE;
		}
		return NodeTools::expect_list(province_node, [&game_manager, &province](ast::NodeCPtr pop_node) -> return_t {
			return game_manager.pop_manager.load_pop_into_province(*province, pop_node);
		});
	}, true);
}

return_t Dataloader::load_pop_history(GameManager& game_manager, std::filesystem::path const& path) const {
	return_t ret = SUCCESS;
	for (std::filesystem::path const& file : lookup_files_in_dir(path)) {
		if (load_pop_history_file(game_manager, file) != SUCCESS) {
			ret = FAILURE;
		}
	}
	return ret;
}
