#include <openvic-simulation/country/CountryInstance.hpp>
#include <openvic-simulation/dataloader/Dataloader.hpp>
#include <openvic-simulation/economy/GoodDefinition.hpp>
#include <openvic-simulation/economy/production/ProductionType.hpp>
#include <openvic-simulation/economy/production/ResourceGatheringOperation.hpp>
#include <openvic-simulation/GameManager.hpp>
#include <openvic-simulation/pop/Pop.hpp>
#include <openvic-simulation/testing/Testing.hpp>
#include <openvic-simulation/utility/Logger.hpp>

using namespace OpenVic;

static void print_help(std::ostream& stream, char const* program_name) {
	stream
		<< "Usage: " << program_name << " [-h] [-t] [-b <path>] [path]+\n"
		<< "    -h : Print this help message and exit the program.\n"
		<< "    -t : Run tests after loading defines.\n"
		<< "    -b : Use the following path as the base directory (instead of searching for one).\n"
		<< "    -s : Use the following path as a hint to search for a base directory.\n"
		<< "Any following paths are read as mod directories, with priority starting at one above the base directory.\n"
		<< "(Paths with spaces need to be enclosed in \"quotes\").\n";
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
		std::string text = StringUtils::append_string_views(
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

		IndexedMap<PopType, pop_size_t> const& employee_count_per_type_cache = rgo.get_employee_count_per_type_cache();
		for (PopType const& pop_type : *employee_count_per_type_cache.get_keys()) {
			const pop_size_t employees_of_type = employee_count_per_type_cache[pop_type];
			if (employees_of_type > 0) {
				text += StringUtils::append_string_views(
					"\n\t\t", std::to_string(employees_of_type), " ", pop_type.get_identifier()
				);
			}
		}
		Logger::info("", text);
	}
}

static void test_rgo(InstanceManager const& instance_manager) {
	std::vector<CountryInstance*> const& great_powers = instance_manager.get_country_instance_manager().get_great_powers();

	for (size_t i = 0; i < std::min<size_t>(3, great_powers.size()); ++i) {
		CountryInstance const* great_power = great_powers[i];
		if (great_power == nullptr) {
			Logger::warning("Great power index ", i, " is null.");
			continue;
		}

		ProvinceInstance const* const capital_province = great_power->get_capital();
		if (capital_province == nullptr) {
			Logger::warning(great_power->get_identifier(), " has no capital ProvinceInstance set.");
		} else {
			print_rgo(*capital_province);
		}
	}
}

template<UnitType::branch_t Branch>
static bool test_pathing(
	InstanceManager& instance_manager, std::string_view country_identifier, size_t unit_group_index,
	size_t target_province_index, bool continue_movement, bool expect_success = true
) {
	using enum UnitType::branch_t;

	CountryInstance* country =
		instance_manager.get_country_instance_manager().get_country_instance_by_identifier(country_identifier);
	if (country == nullptr) {
		Logger::error("Country ", country_identifier, " not found!");
		return false;
	}

	ordered_set<UnitInstanceGroupBranched<Branch>*> const& unit_groups = country->get_unit_instance_groups<Branch>();
	if (unit_groups.size() <= unit_group_index) {
		Logger::error(
			Branch == LAND ? "Land" : "Naval", " unit group index ", unit_group_index, " out of bounds for country ",
			country_identifier
		);
		return false;
	}
	UnitInstanceGroupBranched<Branch>* unit_group = unit_groups.data()[unit_group_index];

	MapInstance const& map_instance = instance_manager.get_map_instance();

	ProvinceInstance const* target_province = map_instance.get_province_instance_by_index(target_province_index);
	if (target_province == nullptr) {
		Logger::error("Province index ", target_province_index, " out of bounds!");
		return false;
	}

	const bool path_found = unit_group->path_to(*target_province, continue_movement, map_instance);

	if (expect_success) {
		if (path_found) {
			Logger::info(
				"Path found for ", Branch == LAND ? "land" : "naval", " unit group \"", unit_group->get_name(), "\" to ",
				target_province->get_identifier(), ". New full path:"
			);

			for (ProvinceInstance const* province : unit_group->get_path()) {
				Logger::info("    ", province->get_identifier());
			}

			return true;
		} else {
			Logger::error(
				"Failed to find path for ", Branch == LAND ? "land" : "naval", " unit group \"", unit_group->get_name(), "\" to ",
				target_province->get_identifier()
			);
			return false;
		}
	} else {
		if (path_found) {
			Logger::error(
				"Found path for ", Branch == LAND ? "land" : "naval", " unit group \"", unit_group->get_name(), "\" to ",
				target_province->get_identifier(), ", despite expecting to fail. New (invalid) path:"
			);

			for (ProvinceInstance const* province : unit_group->get_path()) {
				Logger::info("    ", province->get_identifier());
			}

			return false;
		} else {
			Logger::info(
				"Did not find path for ", Branch == LAND ? "land" : "naval", " unit group \"", unit_group->get_name(), "\" to ",
				target_province->get_identifier(), ", as expected"
			);
			return true;
		}
	}
}

static bool run_headless(Dataloader::path_vector_t const& roots, bool run_tests) {
	bool ret = true;

	GameManager game_manager { []() {
		Logger::info("State updated");
	}, nullptr };

	Logger::info("===== Loading definitions... =====");
	ret &= game_manager.set_roots(roots);
	ret &= game_manager.load_definitions(
		[](std::string_view key, Dataloader::locale_t locale, std::string_view localisation) -> bool {
			return true;
		}
	);

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

	Logger::info("===== Starting game session... =====");
	ret &= game_manager.start_game_session();

	// This triggers a gamestate update
	ret &= game_manager.update_clock();

	// TODO - REMOVE TEST CODE
	if (game_manager.get_instance_manager()) {
		InstanceManager& instance_manager = *game_manager.get_instance_manager();

		Logger::info("===== RGO test... =====");
		test_rgo(instance_manager);

		Logger::info("===== Pathing test... =====");
		using enum UnitType::branch_t;
		// Move "Garde Royale" from Paris to Brest
		ret &= test_pathing<LAND>(instance_manager, "FRA", 0, 420, false);
		// Move "Garde Royale" from Brest to Toulon
		ret &= test_pathing<LAND>(instance_manager, "FRA", 0, 470, true);
		// Move "Garde Royale" from Paris to Strasbourg
		ret &= test_pathing<LAND>(instance_manager, "FRA", 0, 409, false);

		// Move "The South Africa Garrison" from Cape Town to Djibouti
		ret &= test_pathing<LAND>(instance_manager, "ENG", 6, 1875, false);
		// Move "The South Africa Garrison" from Djibouti to Socotra (expected to fail)
		ret &= test_pathing<LAND>(instance_manager, "ENG", 6, 1177, true, false);
		// Move "The South Africa Garrison" from Cape Town to Tananarive (expected to fail)
		ret &= test_pathing<LAND>(instance_manager, "ENG", 6, 2115, false, false);

		// Move "Imperial Fleet" from Kyoto to Taiwan Strait
		ret &= test_pathing<NAVAL>(instance_manager, "JAP", 0, 2825, false);
		// Move "Imperial Fleet" from Kyoto to Falkland Plateau (should go East)
		ret &= test_pathing<NAVAL>(instance_manager, "JAP", 0, 3047, false);
		// Move "Imperial Fleet" from Kyoto to Coast of South Georgia (should go West)
		ret &= test_pathing<NAVAL>(instance_manager, "JAP", 0, 3200, false);
	} else {
		Logger::error("Instance manager not available!");
		ret = false;
	}

	return ret;
}

/*
	$ program [-h] [-t] [-b] [path]+
*/

int main(int argc, char const* argv[]) {
	Logger::set_logger_funcs();

	char const* program_name = StringUtils::get_filename(argc > 0 ? argv[0] : nullptr, "<program>");
	fs::path root;
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
					std::cerr << "Empty path after giving \"" << path << "\" to " << path_use
						<< " command line argument \"" << command << "\"." << std::endl;
				}
			} else {
				std::cerr << "Missing path after " << path_use << " command line argument \"" << command << "\"." << std::endl;
			}
		} else {
			std::cerr << "Duplicate " << path_use << " command line argument \"-b\"." << std::endl;
		}
		print_help(std::cerr, program_name);
		return false;
	};

	while (++argn < argc) {
		char const* arg = argv[argn];
		if (strcmp(arg, "-h") == 0) {
			print_help(std::cout, program_name);
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
			std::cerr << "Search for base directory path failed!" << std::endl;
			print_help(std::cerr, program_name);
			return -1;
		}
	}
	Dataloader::path_vector_t roots { root };
	while (argn < argc) {
		static const fs::path mod_directory = "mod";
		roots.emplace_back(root / mod_directory / argv[argn++]);
	}

	std::cout << "!!! HEADLESS SIMULATION START !!!" << std::endl;

	const bool ret = run_headless(roots, run_tests);

	std::cout << "!!! HEADLESS SIMULATION END !!!" << std::endl;

	std::cout << "\nLoad returned: " << (ret ? "SUCCESS" : "FAILURE") << std::endl;

	std::cout << "\nLogger Summary: Info = " << Logger::get_info_count() << ", Warning = " << Logger::get_warning_count()
		<< ", Error = " << Logger::get_error_count() << std::endl;

	return ret ? 0 : -1;
}
