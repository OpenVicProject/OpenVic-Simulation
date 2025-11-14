#include <chrono>
#include <filesystem>
#include <memory>
#include <random>

#include <fmt/base.h>
#include <fmt/chrono.h>

#include <range/v3/view/enumerate.hpp>

#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/callback_sink.h>

#include <openvic-simulation/GameManager.hpp>
#include <openvic-simulation/country/CountryInstance.hpp>
#include <openvic-simulation/dataloader/Dataloader.hpp>
#include <openvic-simulation/economy/GoodDefinition.hpp>
#include <openvic-simulation/economy/production/ProductionType.hpp>
#include <openvic-simulation/economy/production/ResourceGatheringOperation.hpp>
#include <openvic-simulation/pathfinding/AStarPathing.hpp>
#include <openvic-simulation/testing/Testing.hpp>
#include <openvic-simulation/utility/Containers.hpp>
#include <openvic-simulation/utility/Logger.hpp>
#include <openvic-simulation/utility/MemoryTracker.hpp>

using namespace OpenVic;

inline static void print_memory_usage( //
	std::string_view prefix, std::source_location const& location = std::source_location::current()
) {
#ifdef DEBUG_ENABLED // memory tracking will return 0 without DEBUG_ENABLED
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
	spdlog::log(
#ifndef SPDLOG_NO_SOURCE_LOC
		spdlog::source_loc { location.file_name(), static_cast<int>(location.line()), location.function_name() },
#else
		spdlog::source_loc {},
#endif
		spdlog::level::info, "{} Memory Usage: {} Bytes", prefix, OpenVic::utility::MemoryTracker::get_memory_usage()
	);
#endif
#endif
}

static void print_help(FILE* file, char const* program_name) {
	fmt::println(file, "Usage: {} [-h] [-t] [-b <path>] [path]+", program_name);
	fmt::println(file, "    -h : Print this help message and exit the program.");
	fmt::println(file, "    -t : Run tests after loading defines.");
	fmt::println(file, "    -b : Use the following path as the base directory (instead of searching for one).");
	fmt::println(file, "    -s : Use the following path as a hint to search for a base directory.");
	fmt::println(
		file,
		"Any following paths are read as mods (/path/to/my/MODNAME.mod), with priority starting at one above the base "
		"directory."
	);
	fmt::println(file, "(Paths with spaces need to be enclosed in \"quotes\").");
}

static void print_rgo(ProvinceInstance const& province) {
	ResourceGatheringOperation const& rgo = province.get_rgo();
	ProductionType const* const production_type_nullable = rgo.get_production_type_nullable();
	if (production_type_nullable == nullptr) {
		spdlog::error_s("\t{} - production_type: nullptr", province);
	}
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
	else {
		ProductionType const& production_type = *production_type_nullable;
		GoodDefinition const& output_good = production_type.output_good;

		SPDLOG_INFO("{}:", province);
		SPDLOG_INFO(
			"\tgood: {}, "
			"production_type: {}, "
			"size_multiplier: {}, "
			"output_quantity_yesterday: {}, "
			"revenue_yesterday: {}, "
			"total owner income: {}, "
			"total employee income: {}",
			production_type.output_good, //
			production_type, //
			rgo.get_size_multiplier().to_string(3), //
			rgo.get_output_quantity_yesterday().to_string(3), //
			rgo.get_revenue_yesterday().to_string(3), //
			rgo.get_total_owner_income_cache().to_string(3), //
			rgo.get_total_employee_income_cache().to_string(3)
		);

		bool logged_employees = false;
		for (auto [pop_type, employees_of_type] : rgo.get_employee_count_per_type_cache()) {
			if (employees_of_type > 0) {
				if (!logged_employees) {
					SPDLOG_INFO("\temployess:");
					logged_employees = true;
				}
				SPDLOG_INFO("\t\t{} {}", employees_of_type, pop_type);
			}
		}
	}
#endif
}

using pathing_rng_t = std::mt19937;
using pathing_rng_seed_t = pathing_rng_t::result_type;

using testing_clock_t = std::chrono::high_resolution_clock;
using test_time_point_t = testing_clock_t::time_point;
using test_duration_t = testing_clock_t::duration;

template<size_t TestCount>
static test_duration_t run_pathing_test(AStarPathing& pathing, std::optional<pathing_rng_seed_t> seed) {
	PointMap const& points = pathing.get_point_map();

	if (!seed.has_value()) {
		thread_local std::random_device rd;
		seed = rd();

		SPDLOG_INFO("Running {} pathing tests with randomly generated seed: {}", TestCount, seed.value());
	} else {
		SPDLOG_INFO("Running {} pathing tests with fixed seed: {}", TestCount, seed.value());
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

	const test_time_point_t start_time = testing_clock_t::now();
	for (auto [index, pair] : from_to_tests | ranges::views::enumerate) {
		test_results[index] = pathing.get_id_path(pair.first.key(), pair.second.key());
	}
	const test_time_point_t end_time = testing_clock_t::now();

	for (auto [index, test] : test_results | ranges::views::enumerate) {
		SPDLOG_INFO("-Start Test {}-", index + 1);
		SPDLOG_INFO("\tFrom: Id {}", from_to_tests[index].first.key());
		for (PointMap::points_key_type const& id : test) {
			if (id == test.back()) {
				break;
			}
			if (id == test.front()) {
				continue;
			}
			SPDLOG_INFO("\t\tId {}", id);
		}
		if (test.empty()) {
			SPDLOG_INFO("\t\tFailed to find path");
		}
		SPDLOG_INFO("\tTo: Id {}", from_to_tests[index].second.key());
		SPDLOG_INFO("-End Test {}-", index + 1);
	}

	return end_time - start_time;
}

struct SinkGuard {
	std::shared_ptr<spdlog::logger> logger;
	spdlog::sink_ptr sink;

	SinkGuard(std::shared_ptr<spdlog::logger> const& l, spdlog::sink_ptr s) : logger(l), sink(s) {
		logger->sinks().push_back(sink);
	}

	~SinkGuard() {
		std::vector<spdlog::sink_ptr>& sinks = logger->sinks();
		sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
	}
};

static size_t info_count = 0, warning_count = 0, error_count = 0, critical_count = 0;
static bool run_headless(fs::path const& root, memory::vector<memory::string>& mods, bool run_tests) {
	bool ret = true;
	Dataloader::path_vector_t roots = { root };
	Dataloader::path_vector_t replace_paths = {};

	spdlog::sink_ptr counter_sink = std::make_shared<spdlog::sinks::callback_sink_st>([](spdlog::details::log_msg const& msg) {
		switch (msg.level) {
			using namespace spdlog::level;
		case info:	   info_count++; break;
		case warn:	   warning_count++; break;
		case err:	   error_count++; break;
		case critical: critical_count++; break;
		default:	   break;
		}
	});

	SinkGuard guard(spdlog::default_logger(), counter_sink);

	GameManager game_manager {
		[] {
			SPDLOG_INFO("State updated");
		},
		nullptr, nullptr //
	};

	SPDLOG_INFO("Commit hash: {}", GameManager::get_commit_hash());

	SPDLOG_INFO("===== Setting base path... =====");
	ret &= game_manager.set_base_path(roots);

	SPDLOG_INFO("===== Loading mod descriptors... =====");
	ret &= game_manager.load_mod_descriptors();

	SPDLOG_INFO("===== Loading mods... =====");
	ret &= game_manager.load_mods(mods);

	SPDLOG_INFO("===== Loading definitions... =====");
	ret &= game_manager.load_definitions(
		[](std::string_view key, Dataloader::locale_t locale, std::string_view localisation) -> bool {
			return true;
		}
	);

	print_memory_usage("Definition Setup");

	if (run_tests) {
		// std::shared_ptr<spdlog::logger> testing_logger = spdlog::basic_logger_st(
		// 	"testing",
		// 	"testing/test_results/results.txt",
		// 	true
		// );
		// testing_logger->set_pattern("%v");
		std::shared_ptr<spdlog::logger> testing_logger = spdlog::default_logger();
		Testing testing { game_manager.get_definition_manager(), testing_logger };
		SPDLOG_INFO("Testing Loaded");
		testing.execute_all_scripts();
		testing.report_results();
		SPDLOG_INFO("Testing Executed");
	}

	SPDLOG_INFO("===== Setting up instance... =====");
	ret &= game_manager.setup_instance(
		game_manager.get_definition_manager().get_history_manager().get_bookmark_manager().get_bookmark_by_index(0)
	);

	print_memory_usage("Instance Setup");

	SPDLOG_INFO("===== Starting game session... =====");
	ret &= game_manager.start_game_session();

	// This triggers a gamestate update
	ret &= game_manager.update_clock();

	print_memory_usage("Game Session Post-Start");

	// TODO - REMOVE TEST CODE
	SPDLOG_INFO("===== Ranking system test... =====");
	if (game_manager.get_instance_manager()) {
		const auto print_ranking_list = [ //
		](std::string_view title, OpenVic::utility::forwardable_span<CountryInstance* const> countries) -> void {
			memory::string countries_str;
			for (CountryInstance* country : countries) {
				countries_str += fmt::format(
					"\n\t{} - Total #{} ({}), Prestige #{} ({}), Industry #{} ({}), Military #{} ({})", //
					*country, //
					country->get_total_rank(), country->total_score.get_untracked().to_string(1), //
					country->get_prestige_rank(), country->get_prestige_untracked().to_string(1),
					country->get_industrial_rank(), country->get_industrial_power_untracked().to_string(1),
					country->get_military_rank(), country->military_power.get_untracked().to_string(1)
				);
			}
			SPDLOG_INFO("{}:{}", title, countries_str);
		};

		CountryInstanceManager const& country_instance_manager =
			game_manager.get_instance_manager()->get_country_instance_manager();

		OpenVic::utility::forwardable_span<CountryInstance* const> great_powers = country_instance_manager.get_great_powers();
		print_ranking_list("Great Powers", great_powers);
		print_ranking_list("Secondary Powers", country_instance_manager.get_secondary_powers());
		print_ranking_list("All countries", country_instance_manager.get_total_ranking());

		SPDLOG_INFO("===== RGO test... =====");
		for (size_t i = 0; i < std::min<size_t>(3, great_powers.size()); ++i) {
			CountryInstance const& great_power = *great_powers[i];
			ProvinceInstance const* const capital_province = great_power.get_capital();
			if (capital_province == nullptr) {
				spdlog::warn_s("{} has no capital ProvinceInstance set.", great_power);
			} else {
				print_rgo(*capital_province);
			}
		}
	} else {
		spdlog::error_s("Instance manager not available!");
		ret = false;
	}

	if (ret) {
		static constexpr size_t TESTS = 10;
		// Set to std::nullopt to use a different random seed on each run
		static constexpr std::optional<pathing_rng_seed_t> LAND_SEED = 1836, SEA_SEED = 1861;

		using test_time_units_t = std::chrono::milliseconds;

		MapInstance& map_instance = game_manager.get_instance_manager()->get_map_instance();

		SPDLOG_INFO("===== Land Pathfinding test... =====");
		test_duration_t duration = std::chrono::duration_cast<test_time_units_t>( //
			run_pathing_test<TESTS>(map_instance.get_land_pathing(), LAND_SEED)
		);
		SPDLOG_INFO("Ran {} land pathing tests in {}", TESTS, duration);

		SPDLOG_INFO("===== Sea Pathfinding test... =====");
		duration = std::chrono::duration_cast<test_time_units_t>( //
			run_pathing_test<TESTS>(map_instance.get_sea_pathing(), SEA_SEED)
		);
		SPDLOG_INFO("Ran {} sea pathing tests in {}", TESTS, duration);
	}

	if (ret) {
		static constexpr size_t TICK_COUNT = 10;

		SPDLOG_INFO("===== Game Tick test... =====");
		size_t ticks_passed = 0;
		test_duration_t min_tick_duration = test_duration_t::max(), //
			max_tick_duration = test_duration_t::min(), //
			total_tick_duration;
		const test_time_point_t start_time = testing_clock_t::now();
		while (++ticks_passed < TICK_COUNT) {
			const test_time_point_t tick_start = testing_clock_t::now();
			game_manager.get_instance_manager()->force_tick_and_update();
			const test_time_point_t tick_end = testing_clock_t::now();

			const test_duration_t tick_duration = tick_end - tick_start;

			min_tick_duration = std::min(min_tick_duration, tick_duration);
			max_tick_duration = std::max(max_tick_duration, tick_duration);

			total_tick_duration += tick_duration;

			print_memory_usage(fmt::format("Tick {} finished", ticks_passed));
		}
		const test_time_point_t end_time = testing_clock_t::now();
		const test_duration_t duration = end_time - start_time;

		if (ticks_passed != 0 && duration != test_duration_t::zero()) {
			const test_duration_t tick_tps = total_tick_duration / ticks_passed;
			const test_duration_t total_tps = duration / ticks_passed;
			SPDLOG_INFO(
				"Ran {} / {} ticks, total time {} at {} per tick, tick time only {} at {} per tick. "
				"Tick lengths ranged from {} to {}.",
				ticks_passed, TICK_COUNT, duration, total_tps, total_tick_duration, tick_tps, //
				min_tick_duration, max_tick_duration
			);
		} else {
			spdlog::error_s(
				"No ticks passed ({}, expected {}) or zero duration measured ({})!", ticks_passed, TICK_COUNT, duration
			);
			ret = false;
		}
	}

	SPDLOG_INFO("===== Ending game session... =====");
	ret &= game_manager.end_game_session();

	SPDLOG_INFO("Max Memory Usage: {} Bytes", OpenVic::utility::MemoryTracker::get_max_memory_usage());

	return ret;
}

/*
	$ program [-h] [-t] [-b] [path]+
*/

int main(int argc, char const* argv[]) {
	char const* program_name = StringUtils::get_filename(argc > 0 ? argv[0] : nullptr, "<program>");
	fs::path root;
	memory::vector<memory::string> mods;
	mods.reserve(argc);
	bool run_tests = false;
	int argn = 0;

	/* Reads the next argument and converts it to a path via path_transform. If reading or converting fails, an error
	 * message and the help text are displayed, along with returning false to signify the program should exit.
	 */
	const auto _read = [&root, &argn, argc, argv,
						program_name //
	](std::string_view command, std::string_view path_use, std::invocable<fs::path> auto path_transform) -> bool {
		if (root.empty()) {
			if (++argn < argc) {
				char const* path = argv[argn];
				root = path_transform(path);
				if (!root.empty()) {
					return true;
				} else {
					fmt::println(
						stderr, "Empty path after giving \"{}\" to {} command line argument \"{}\".", path, path_use, command
					);
				}
			} else {
				fmt::println(stderr, "Missing path after {} command line argument \"{}\".", path_use, command);
			}
		} else {
			fmt::println(stderr, "Duplicate {} command line argument \"-b\".", path_use);
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

	SPDLOG_INFO("!!! HEADLESS SIMULATION START !!!");

	const bool ret = run_headless(root, mods, run_tests);

	SPDLOG_INFO("!!! HEADLESS SIMULATION END !!!");

	spdlog::info("Load returned: {}", ret ? "SUCCESS" : "FAILURE");

	spdlog::info(
		"Logger Summary: "
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
		"Info = {}, "
#endif
		"Warning = {}, "
		"Error = {}, "
		"Critical = {}",
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
		info_count,
#endif
		warning_count, error_count, critical_count
	);

	return ret ? 0 : -1;
}
