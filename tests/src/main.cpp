#include <array>
#include <optional>

#include "GameManager.hpp"
#include <snitch/snitch_cli.hpp>
#include <snitch/snitch_registry.hpp>

namespace snitch {
	OpenVic::GameManager* game_manager;

	extern constexpr size_t MP_GAME_MANAGERS_SIZE = 3;
	static OpenVic::GameManager* mp_game_managers;
	extern OpenVic::GameManager* get_mp_game_managers(size_t index) {
		if (index >= MP_GAME_MANAGERS_SIZE) {
			return nullptr;
		}
		return &mp_game_managers[index];
	}
}

int main(int argc, char* argv[]) {
	if constexpr (snitch::is_enabled) {
		// Parse the command-line arguments.
		std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
		if (!args) {
			// Parsing failed, an error has been reported, just return.
			return 1;
		}

		// Configure snitch using command-line options.
		// You can then override the configuration below, or just remove this call to disable
		// command-line options entirely.
		snitch::tests.configure(*args);

		// Your own initialization code goes here.
		// ...

		using namespace OpenVic;
		using namespace snitch;

		GameManager _manager { [] {}, nullptr, nullptr };
		game_manager = &_manager;

		alignas(GameManager) std::array<std::byte, sizeof(GameManager) * MP_GAME_MANAGERS_SIZE> _mp_managers_bytes; // NOLINT
		for (size_t i = 0; i < MP_GAME_MANAGERS_SIZE; i++) {
			new (&_mp_managers_bytes[i * sizeof(GameManager)]) GameManager { [] {}, nullptr, nullptr };
		}
		mp_game_managers = reinterpret_cast<GameManager*>(_mp_managers_bytes.data());

		// Actually run the tests.
		// This will apply any filtering specified on the command-line.
		int result = snitch::tests.run_tests(*args) ? 0 : 1;

		game_manager = nullptr;
		for (size_t i = 0; i < MP_GAME_MANAGERS_SIZE; i++) {
			mp_game_managers[i].~GameManager();
		}
		mp_game_managers = nullptr;

		return result;
	} else {
		return 0;
	}
}
