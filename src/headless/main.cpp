#include <chrono>
#include <filesystem>
#include <random>
#include <string>

#include <fmt/base.h>
#include <fmt/chrono.h>

#include <range/v3/view/enumerate.hpp>

#include <openvic-simulation/GameManager.hpp>
#include <openvic-simulation/country/CountryInstance.hpp>
#include <openvic-simulation/dataloader/Dataloader.hpp>
#include <openvic-simulation/dataloader/ModManager.hpp>
#include <openvic-simulation/economy/GoodDefinition.hpp>
#include <openvic-simulation/economy/production/ProductionType.hpp>
#include <openvic-simulation/economy/production/ResourceGatheringOperation.hpp>
#include <openvic-simulation/pathfinding/AStarPathing.hpp>
#include <openvic-simulation/pop/Pop.hpp>
#include <openvic-simulation/testing/Testing.hpp>
#include <openvic-simulation/utility/Containers.hpp>
#include <openvic-simulation/utility/Logger.hpp>
#include <openvic-simulation/utility/MemoryTracker.hpp>

using namespace OpenVic;

inline static void print_bytes(std::string_view prefix, const std::source_location& location = std::source_location::current()) {
#ifdef DEBUG_ENABLED // memory tracking will return 0 without DEBUG_ENABLED
	Logger::info<std::string_view&, const char(&)[16], uint64_t&&, const char(&)[7]>(
		prefix, " Memory Usage: ", OpenVic::utility::MemoryTracker::get_memory_usage(), " Bytes", location
	);
#endif
}

static void print_help(FILE* file, char const* program_name) {
	fmt::println(file, "Usage: {} [-h] [-t] [-b <path>] [path]+", program_name);
	fmt::println(file, "    -h : Print this help message and exit the program.");
	fmt::println(file, "    -t : Run tests after loading defines.");
	fmt::println(file, "    -b : Use the following path as the base directory (instead of searching for one).");
	fmt::println(file, "    -s : Use the following path as a hint to search for a base directory.");
	fmt::println(file, "Any following paths are read as mods (/path/to/my/MODNAME.mod), with priority starting at one above the base directory.");
	fmt::println(file, "(Paths with spaces need to be enclosed in \"quotes\").");
}

static void print_rgo(ProvinceInstance const& province) {
	ResourceGatheringOperation const& rgo = province.get_rgo();
	ProductionType const* const production_type_nullable = rgo.get_production_type_nullable();
	if (production_type_nullable == nullptr) {
		Logger::error(
			"\n    ", province.get_identifier(),
			" - production_type: nullptr"
		);
	} else {
		ProductionType const& production_type = *production_type_nullable;
		GoodDefinition const& output_good = production_type.get_output_good();
		memory::string text = StringUtils::append_string_views(
			"\n\t", province.get_identifier(),
			" - good: ", output_good.get_identifier(),
			", production_type: ", production_type.get_identifier(),
			", size_multiplier: ", rgo.get_size_multiplier().to_string(3),
			", output_quantity_yesterday: ", rgo.get_output_quantity_yesterday().to_string(3),
			", revenue_yesterday: ", rgo.get_revenue_yesterday().to_string(3),
			", total owner income: ", rgo.get_total_owner_income_cache().to_string(3),
			", total employee income: ", rgo.get_total_employee_income_cache().to_string(3),
			"\n\temployees:"
		);

		for (auto [pop_type, employees_of_type] : rgo.get_employee_count_per_type_cache()) {
			if (employees_of_type > 0) {
				text += StringUtils::append_string_views(
					"\n\t\t", std::to_string(employees_of_type), " ", pop_type.get_identifier()
				);
			}
		}
		Logger::info("", text);
	}
}

using pathing_rng_t = std::mt19937;
using pathing_rng_seed_t = pathing_rng_t::result_type;

template<size_t TestCount>
static std::chrono::nanoseconds run_pathing_test(AStarPathing& pathing, std::optional<pathing_rng_seed_t> seed) {
	PointMap const& points = pathing.get_point_map();

	if (!seed.has_value()) {
		thread_local std::random_device rd;
		seed = rd();

		Logger::info("Running ", TestCount, " pathing tests with randomly generated seed: ", seed.value());
	} else {
		Logger::info("Running ", TestCount, " pathing tests with fixed seed: ", seed.value());
	}

	thread_local pathing_rng_t gen { seed.value() };

	using it = decltype(points.points_map().end());
	std::array<std::pair<it, it>, TestCount> from_to_tests;
	std::array<memory::vector<PointMap::points_key_type>, TestCount> test_results;

	for (size_t index = 0; index < TestCount; index++) {
		it from = points.points_map().begin();
		it to = points.points_map().begin();
		std::uniform_int_distribution<> dis(0, points.get_point_count() - 1);
		std::advance(from, dis(gen));
		std::advance(to, dis(gen));
		from_to_tests[index] = { from, to };
	}

	auto start_time = std::chrono::high_resolution_clock::now();
	for (auto [index, pair] : from_to_tests | ranges::views::enumerate) {
		test_results[index] = pathing.get_id_path(pair.first.key(), pair.second.key());
	}
	auto end_time = std::chrono::high_resolution_clock::now();

	for (auto [index, test] : test_results | ranges::views::enumerate) {
		Logger::info("-Start Test ", index + 1, "-");
		Logger::info("\tFrom: Id ", from_to_tests[index].first.key());
		for (PointMap::points_key_type const& id : test) {
			if (id == test.back()) {
				break;
			}
			if (id == test.front()) {
				continue;
			}
			Logger::info("\t\tId ", id);
		}
		if (test.empty()) {
			Logger::info("\t\tFailed to find path");
		}
		Logger::info("\tTo: Id ", from_to_tests[index].second.key());
		Logger::info("-End Test ", index + 1, "-");
	}

	return end_time - start_time;
}

static bool run_headless(fs::path const& root, memory::vector<memory::string>& mods, bool run_tests) {
	bool ret = true;
	Dataloader::path_vector_t roots = { root };
	Dataloader::path_vector_t replace_paths = {};

	GameManager game_manager { []() {
		Logger::info("State updated");
	} };

	Logger::info("Commit hash: ", GameManager::get_commit_hash());

	Logger::info("===== Setting base path... =====");
	ret &= game_manager.set_base_path(roots);

	Logger::info("===== Loading mod descriptors... =====");
	ret &= game_manager.load_mod_descriptors();

	Logger::info("===== Loading mods... =====");
	ret &= game_manager.load_mods(roots, replace_paths, mods);

	Logger::info("===== Loading definitions... =====");
	ret &= game_manager.load_definitions(
		[](std::string_view key, Dataloader::locale_t locale, std::string_view localisation) -> bool {
			return true;
		}
	);

	print_bytes("Definition Setup");

	if (run_tests) {
		Testing testing { game_manager.get_definition_manager() };
		std::cout << std::endl << "Testing Loaded" << std::endl << std::endl;
		testing.execute_all_scripts();
		testing.report_results();
		std::cout << "Testing Executed" << std::endl << std::endl;
	}

	Logger::info("===== Setting up instance... =====");
	ret &= game_manager.setup_instance(
		game_manager.get_definition_manager().get_history_manager().get_bookmark_manager().get_bookmark_by_index(0)
	);

	print_bytes("Instance Setup");

	Logger::info("===== Starting game session... =====");
	ret &= game_manager.start_game_session();

	// This triggers a gamestate update
	ret &= game_manager.update_clock();

	print_bytes("Game Session Post-Start");

	// TODO - REMOVE TEST CODE
	Logger::info("===== Ranking system test... =====");
	if (game_manager.get_instance_manager()) {
		const auto print_ranking_list = [](std::string_view title, OpenVic::utility::forwardable_span<CountryInstance* const> countries) -> void {
			memory::string text;
			for (CountryInstance* country : countries) {
				text += StringUtils::append_string_views(
					"\n    ", country->get_identifier(),
					" - Total #", std::to_string(country->get_total_rank()), " (", country->total_score.get_untracked().to_string(1),
					"), Prestige #", std::to_string(country->get_prestige_rank()), " (", country->get_prestige_untracked().to_string(1),
					"), Industry #", std::to_string(country->get_industrial_rank()), " (", country->get_industrial_power_untracked().to_string(1),
					"), Military #", std::to_string(country->get_military_rank()), " (", country->military_power.get_untracked().to_string(1), ")"
				);
			}
			Logger::info(title, ":", text);
		};

		CountryInstanceManager const& country_instance_manager =
			game_manager.get_instance_manager()->get_country_instance_manager();

		OpenVic::utility::forwardable_span<CountryInstance* const> great_powers = country_instance_manager.get_great_powers();
		print_ranking_list("Great Powers", great_powers);
		print_ranking_list("Secondary Powers", country_instance_manager.get_secondary_powers());
		print_ranking_list("All countries", country_instance_manager.get_total_ranking());

		Logger::info("===== RGO test... =====");
		for (size_t i = 0; i < std::min<size_t>(3, great_powers.size()); ++i) {
			CountryInstance const& great_power = *great_powers[i];
			ProvinceInstance const* const capital_province = great_power.get_capital();
			if (capital_province == nullptr) {
				Logger::warning(great_power.get_identifier(), " has no capital ProvinceInstance set.");
			} else {
				print_rgo(*capital_province);
			}
		}
	} else {
		Logger::error("Instance manager not available!");
		ret = false;
	}

	if (ret) {
		static constexpr size_t TESTS = 10;
		// Set to std::nullopt to use a different random seed on each run
		static constexpr std::optional<pathing_rng_seed_t> LAND_SEED = 1836, SEA_SEED = 1861;

		Logger::info("===== Land Pathfinding test... =====");
		std::chrono::nanoseconds ns = run_pathing_test<TESTS>(
			game_manager.get_instance_manager()->get_map_instance().get_land_pathing(), LAND_SEED
		);
		Logger::info("Ran ", TESTS, " land pathing tests in ", ns);

		Logger::info("===== Sea Pathfinding test... =====");
		ns = run_pathing_test<TESTS>(
			game_manager.get_instance_manager()->get_map_instance().get_sea_pathing(), SEA_SEED
		);
		Logger::info("Ran ", TESTS, " sea pathing tests in ", ns);
	}

	if (ret) {
		Logger::info("===== Game Tick test... =====");
		size_t ticks_passed = 0;
		auto start_time = std::chrono::high_resolution_clock::now();
		while (ticks_passed++ < 10) {
			game_manager.get_instance_manager()->force_tick_and_update();
			print_bytes("Tick Finished");
		}
		auto end_time = std::chrono::high_resolution_clock::now();
		Logger::info("Ran ", --ticks_passed, " ticks in ", end_time - start_time);
	}

	Logger::info("===== Ending game session... =====");
	ret &= game_manager.end_game_session();

	Logger::info("Max Memory Usage: ", OpenVic::utility::MemoryTracker::get_max_memory_usage(), " Bytes");

	return ret;
}

/*
	$ program [-h] [-t] [-b] [path]+
*/

int main(int argc, char const* argv[]) {
	Logger::set_logger_funcs();

	char const* program_name = StringUtils::get_filename(argc > 0 ? argv[0] : nullptr, "<program>");
	fs::path root;
	memory::vector<memory::string> mods;
	mods.reserve(argc);
	bool run_tests = false;
	int argn = 0;

	/* Reads the next argument and converts it to a path via path_transform. If reading or converting fails, an error
	 * message and the help text are displayed, along with returning false to signify the program should exit.
	 */
	const auto _read = [&root, &argn, argc, argv, program_name](
		std::string_view command, std::string_view path_use, auto path_transform) -> bool {
		if (root.empty()) {
			if (++argn < argc) {
				char const* path = argv[argn];
				root = path_transform(path);
				if (!root.empty()) {
					return true;
				} else {
					fmt::println(
						stderr,
						"Empty path after giving \"{}\" to {} command line argument \"{}\".",
						path, path_use, command
					);
				}
			} else {
				fmt::println(
					stderr,
					"Missing path after {} command line argument \"{}\".",
					path_use, command
				);
			}
		} else {
			fmt::println(
				stderr,
				"Duplicate {} command line argument \"-b\".",
				path_use
			);
		}
		print_help(stderr, program_name);
		return false;
	};

	while (++argn < argc) {
		char const* arg = argv[argn];
		if (strcmp(arg, "-h") == 0) {
			print_help(stdout, program_name);
			return 0;
		} else if (strcmp(arg, "-t") == 0) {
			run_tests = true;
		} else if (strcmp(arg, "-b") == 0) {
			if (!_read("-b", "base directory", std::identity {})) {
				return -1;
			}
		} else if (strcmp(arg, "-s") == 0) {
			if (!_read("-s", "search hint", Dataloader::search_for_game_path)) {
				return -1;
			}
		} else {
			break;
		}
	}
	if (root.empty()) {
		root = Dataloader::search_for_game_path();
		if (root.empty()) {
			fmt::println(stderr, "Search for base directory path failed!");
			print_help(stderr, program_name);
			return -1;
		}
	}
	while (argn < argc) {
		mods.emplace_back(argv[argn++]);
	}

	std::cout << "!!! HEADLESS SIMULATION START !!!" << std::endl;

	const bool ret = run_headless(root, mods, run_tests);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret ? "SUCCESS" : "FAILURE") << std::endl;

	std::cout << "\nLogger Summary: Info = " << Logger::get_info_count() << ", Warning = " << Logger::get_warning_count()
		<< ", Error = " << Logger::get_error_count() << std::endl;

	return ret ? 0 : -1;
}
