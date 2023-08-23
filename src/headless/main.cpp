#ifdef OPENVIC_SIM_HEADLESS

#include <openvic/utility/Logger.hpp>
#include <openvic/GameManager.hpp>
#include <openvic/dataloader/Dataloader.hpp>

using namespace OpenVic;

static char const* get_program_name(char const* name) {
	static char const* const missing_name = "<program>";
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

int main(int argc, char const* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << get_program_name(argc > 0 ? argv[0] : nullptr) << " <base defines dir> [[mod defines dir]+]" << std::endl;
		std::cout << "Requires defines path(s) as arguments, starting with the base defines and continuing with mods (paths with spaces need to be enclosed in \"quotes\")." << std::endl;
		return -1;
	}

	std::cout << "!!! HEADLESS SIMULATION START !!!" << std::endl;

	Logger::set_info_func([](std::string&& str) { std::cout << str; });
	Logger::set_error_func([](std::string&& str) { std::cerr << str; });

	GameManager game_manager { []() {
		Logger::info("State updated");
	} };
	Dataloader dataloader;

	std::vector<std::filesystem::path> roots;
	for (int i = 1; i < argc; ++i) {
		roots.push_back(argv[i]);
	}
	dataloader.set_roots(roots);

	if (dataloader.load_defines(game_manager) != SUCCESS) {
		Logger::error("Failed to load defines!");
	}
	if (game_manager.load_hardcoded_defines() != SUCCESS) {
		Logger::error("Failed to load hardcoded defines!");
	}

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;
	return 0;
}
#endif
