#ifdef OPENVIC_SIM_HEADLESS

#pragma once
#include <openvic/GameManager.hpp>
#include <openvic/dataloader/Dataloader.hpp>
#include <openvic/utility/Logger.hpp>
#include <openvic/testing/Testing.hpp>

using namespace OpenVic;

static char const* get_program_name(char const* name) {
	static constexpr char const* missing_name = "<program>";
	if (name == nullptr) return missing_name;
	char const* last_separator = name;
	while (*name != '\0') {
		if (*name == '/' || *name == '\\') {
			last_separator = name + 1;
		}
		++name;
	}
	if (*last_separator == '\0') return missing_name;
	return last_separator;
}

static return_t headless_load(std::vector<std::filesystem::path> const& roots) {
	return_t ret = SUCCESS;

	Logger::set_info_func([](std::string&& str) { std::cout << str; });
	Logger::set_error_func([](std::string&& str) { std::cerr << str; });

	GameManager game_manager { []() {
		Logger::info("State updated");
	} };
	Dataloader dataloader;

	if (dataloader.set_roots(roots) != SUCCESS) {
		Logger::error("Failed to set dataloader roots!");
		ret = FAILURE;
	}
	if (dataloader.load_defines(game_manager) != SUCCESS) {
		Logger::error("Failed to load defines!");
		ret = FAILURE;
	}
	if (game_manager.load_hardcoded_defines() != SUCCESS) {
		Logger::error("Failed to load hardcoded defines!");
		ret = FAILURE;
	}

	Testing testing = Testing(&game_manager);
	std::cout << std::endl << "Testing loaded" << std::endl << std::endl;

	return ret;
}

int main(int argc, char const* argv[]) {
	std::vector<std::filesystem::path> roots;
	if (argc < 2) {
		// TODO - non-Windows default paths
		static const std::filesystem::path default_path = "C:/Program Files (x86)/Steam/steamapps/common/Victoria 2";

		std::cout << "Usage: " << get_program_name(argc > 0 ? argv[0] : nullptr) << " <base defines dir> [[mod defines dir]+]\n"
			<< "Requires defines path(s) as arguments, starting with the base defines and continuing with mods.\n"
			<< "(Paths with spaces need to be enclosed in \"quotes\").\n"
			<< "Defaulting to " << default_path << std::endl;
		roots.push_back(default_path);
	} else {
		for (int i = 1; i < argc; ++i) {
			roots.push_back(argv[i]);
		}
	}

	std::cout << "!!! HEADLESS SIMULATION START !!!" << std::endl;

	const return_t ret = headless_load(roots);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret == SUCCESS ? "SUCCESS" : "FAILURE") << std::endl;

	return ret == SUCCESS ? 0 : -1;
}
#endif
