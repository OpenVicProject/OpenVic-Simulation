#include <nanobench.h>

#include "openvic-simulation/GameManager.hpp"

#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("Dataloading benchmark", "[benchmarks][benchmark-dataloading]") {
	std::filesystem::path root = Dataloader::search_for_game_path();
	Dataloader::path_vector_t roots { root };

	ankerl::nanobench::Bench().epochs(10).run("Dataloading", [&] {
		OpenVic::GameManager game_manager { []() {} };

		game_manager.set_roots(roots, {});
		game_manager.load_definitions(
			[](std::string_view key, Dataloader::locale_t locale, std::string_view localisation) -> bool {
				return true;
			}
		);
	});
}
