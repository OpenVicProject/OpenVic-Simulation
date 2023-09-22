<<<<<<< .merge_file_mnTBNX
#ifdef OPENVIC_SIM_HEADLESS

#pragma once
#include <openvic/GameManager.hpp>
#include <openvic/dataloader/Dataloader.hpp>
#include <openvic/utility/Logger.hpp>
#include <openvic/testing/Testing.hpp>
=======
#include <openvic-simulation/GameManager.hpp>
#include <openvic-simulation/dataloader/Dataloader.hpp>
#include <openvic-simulation/utility/Logger.hpp>
>>>>>>> .merge_file_L81t7E

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

<<<<<<< .merge_file_mnTBNX
static return_t headless_load(std::vector<std::filesystem::path> const& roots) {
	return_t ret = SUCCESS;
=======
static bool headless_load(Dataloader::path_vector_t const& roots) {
	bool ret = true;
>>>>>>> .merge_file_L81t7E

	Logger::set_info_func([](std::string&& str) { std::cout << str; });
	Logger::set_warning_func([](std::string&& str) { std::cerr << str; });
	Logger::set_error_func([](std::string&& str) { std::cerr << str; });

	GameManager game_manager { []() {
		Logger::info("State updated");
	} };
	Dataloader dataloader;

<<<<<<< .merge_file_mnTBNX
	if (dataloader.set_roots(roots) != SUCCESS) {
		Logger::error("Failed to set dataloader roots!");
		ret = FAILURE;
	}
	if (dataloader.load_defines(game_manager) != SUCCESS) {
		Logger::error("Failed to load defines!");
		ret = FAILURE;
=======
	if (!dataloader.set_roots(roots)) {
		Logger::error("Failed to set dataloader roots!");
		ret = false;
	}
	if (!dataloader.load_defines(game_manager)) {
		Logger::error("Failed to load defines!");
		ret = false;
>>>>>>> .merge_file_L81t7E
	}
	if (!game_manager.load_hardcoded_defines()) {
		Logger::error("Failed to load hardcoded defines!");
<<<<<<< .merge_file_mnTBNX
		ret = FAILURE;
	}

	Testing testing = Testing(&game_manager);
	std::cout << std::endl << "Testing loaded" << std::endl << std::endl;
=======
		ret = false;
	}
	if (!dataloader.load_localisation_files(
		[](std::string_view key, Dataloader::locale_t locale, std::string_view localisation) -> bool {
			return true;
		}
	)) {
		Logger::error("Failed to load localisation!");
		ret = false;
	}
>>>>>>> .merge_file_L81t7E

	return ret;
}

int main(int argc, char const* argv[]) {
<<<<<<< .merge_file_mnTBNX
	std::vector<std::filesystem::path> roots;
	if (argc < 2) {
		// TODO - non-Windows default paths
		static const std::filesystem::path default_path = "C:/Program Files (x86)/Steam/steamapps/common/Victoria 2";

		std::cout << "Usage: " << get_program_name(argc > 0 ? argv[0] : nullptr) << " <base defines dir> [[mod defines dir]+]\n"
=======
	Dataloader::path_vector_t roots;
	if (argc < 2) {
		// TODO - non-Windows default paths
		static const fs::path default_path = "C:/Program Files (x86)/Steam/steamapps/common/Victoria 2";

		std::cout
			<< "Usage: " << get_program_name(argc > 0 ? argv[0] : nullptr) << " <base defines dir> [[mod defines dir]+]\n"
>>>>>>> .merge_file_L81t7E
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

<<<<<<< .merge_file_mnTBNX
	const return_t ret = headless_load(roots);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret == SUCCESS ? "SUCCESS" : "FAILURE") << std::endl;

	return ret == SUCCESS ? 0 : -1;
=======
	const bool ret = headless_load(roots);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret ? "SUCCESS" : "FAILURE") << std::endl;

	return ret ? 0 : -1;
>>>>>>> .merge_file_L81t7E
}
