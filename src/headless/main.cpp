#include <cstring>

#include <openvic-simulation/GameManager.hpp>
#include <openvic-simulation/dataloader/Dataloader.hpp>
#include <openvic-simulation/utility/Logger.hpp>
#include <openvic-simulation/testing/Testing.hpp>

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

static void print_help(char const* program_name) {
	std::cout
		<< "Usage: " << program_name << " [-h] [-t] [-b <path>] [path]+\n"
		<< "    -h : Print this help message and exit the program.\n"
		<< "    -t : Run tests after loading defines.\n"
		<< "    -b : Use the following path as the base directory (instead of searching for one).\n"
		<< "Any following paths are read as mod directories, with priority starting at one above the base directory.\n"
		<< "(Paths with spaces need to be enclosed in \"quotes\").\n";
}

static void setup_logger_funcs() {
	Logger::set_info_func([](std::string&& str) { std::cout << str; });
	Logger::set_warning_func([](std::string&& str) { std::cerr << str; });
	Logger::set_error_func([](std::string&& str) { std::cerr << str; });
}

static bool headless_load(GameManager& game_manager, Dataloader const& dataloader) {
	bool ret = true;

	if (!dataloader.load_defines(game_manager)) {
		Logger::error("Failed to load defines!");
		ret = false;
	}
	if (!game_manager.load_hardcoded_defines()) {
		Logger::error("Failed to load hardcoded defines!");
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

	return ret;
}

static bool run_headless(Dataloader::path_vector_t const& roots, bool run_tests) {
	bool ret = true;

	setup_logger_funcs();

	Dataloader dataloader;
	if (!dataloader.set_roots(roots)) {
		Logger::error("Failed to set dataloader roots!");
		ret = false;
	}

	GameManager game_manager { []() {
		Logger::info("State updated");
	} };

	ret &= headless_load(game_manager, dataloader);

	if (run_tests) {
		Testing testing = Testing(&game_manager);
		std::cout << std::endl << "Testing Loaded" << std::endl << std::endl;
		testing.execute_all_scripts();
		testing.report_results();
		std::cout << "Testing Executed" << std::endl << std::endl;
	}

	return ret;
}

/*
	$ program [-h] [-t] [-b] [path]+
*/

int main(int argc, char const* argv[]) {
	char const* program_name = get_program_name(argc > 0 ? argv[0] : nullptr);

	fs::path root;
	bool run_tests = false;

	int argn = 0;
	while (++argn < argc) {
		char const* arg = argv[argn];
		if (strcmp(arg, "-h") == 0) {
			print_help(program_name);
			return 0;
		} else if (strcmp(arg, "-t") == 0) {
			run_tests = true;
		} else if (strcmp(arg, "-b") == 0) {
			if (root.empty())  {
				if (++argn < argc) {
					root = argv[argn];
					if (!root.empty()) {
						continue;
					} else {
						std::cerr << "Empty path after base directory command line argument \"-b\"." << std::endl;
					}
				} else {
					std::cerr << "Missing path after base directory command line argument \"-b\"." << std::endl;
				}
			} else {
				std::cerr << "Duplicate base directory command line argument \"-b\"." << std::endl;
			}
			print_help(program_name);
			return -1;
		} else {
			break;
		}
	}
	if (root.empty()) {
		root = Dataloader::search_for_game_path();
		if (root.empty()) {
			std::cerr << "Search for base directory path failed!" << std::endl;
			print_help(program_name);
			return -1;
		}
	}
	Dataloader::path_vector_t roots = { root };
	while (argn < argc) {
		roots.emplace_back(root / argv[argn++]);
	}

	std::cout << "!!! HEADLESS SIMULATION START !!!" << std::endl;

	const bool ret = run_headless(roots, run_tests);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret ? "SUCCESS" : "FAILURE") << std::endl;

	return ret ? 0 : -1;
}
