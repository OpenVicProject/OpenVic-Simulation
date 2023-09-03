#pragma once
#include <testing/TestScript.hpp>
#include <GameManager.hpp>
#include <iostream>
#include <vector>

#include <testing/test_scripts/A_001_file_tests.cpp>
#include <testing/test_scripts/A_002_economy_tests.cpp>
#include <testing/test_scripts/A_003_military_unit_tests.cpp>
#include <testing/test_scripts/A_004_networking_tests.cpp>
#include <testing/test_scripts/A_005_nation_tests.cpp>
#include <testing/test_scripts/A_006_politics_tests.cpp>

namespace OpenVic {

	class Testing {

	public:
		GameManager* game_manager;
		Map* map;
		BuildingManager* building_manager;
		GoodManager* good_manager;
		PopManager* pop_manager;
		GameAdvancementHook* clock;

		std::vector<TestScript*> test_scripts = std::vector<TestScript*>();

		Testing(GameManager* g_manager) {

			game_manager = g_manager;
			map = game_manager->get_map();
			building_manager = game_manager->get_building_manager();
			good_manager = game_manager->get_good_manager();
			clock = game_manager->get_game_advancement_hook();

			// Prototype test script
			const BuildingType* building_type = building_manager->get_building_type_by_identifier("building_fort");
			std::cout << "building_fort"
					  << " build time is " << building_type->get_build_time() << std::endl;
			std::cout << "building_fort"
					  << " identifier is " << building_type->get_identifier() << std::endl;
			std::cout << "building_fort"
					  << " max level is " << int(building_type->get_max_level()) << std::endl;
			for (const auto& good : good_manager->get_goods())
				std::cout << good.get_identifier() << " price = " << good.get_base_price() << std::endl;

			// Constructor for the tests will add requirements
			// Then execute the script
			A_001_file_tests* a_001_file_tests = new A_001_file_tests();
			test_scripts.push_back(a_001_file_tests);
			A_002_economy_tests* a_002_economy_tests = new A_002_economy_tests();
			test_scripts.push_back(a_002_economy_tests);
			A_003_military_unit_tests* a_003_military_unit_tests = new A_003_military_unit_tests();
			test_scripts.push_back(a_003_military_unit_tests);
			A_004_networking_tests* a_004_networking_tests = new A_004_networking_tests();
			test_scripts.push_back(a_004_networking_tests);
			A_005_nation_tests* a_005_nation_tests = new A_005_nation_tests();
			test_scripts.push_back(a_005_nation_tests);
			A_006_politics_tests* a_006_politics_tests = new A_006_politics_tests();
			test_scripts.push_back(a_006_politics_tests);
		}
	};
}
