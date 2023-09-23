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
		GameManager& game_manager;
		Map& map;
		BuildingManager& building_manager;
		GoodManager& good_manager;
		PopManager& pop_manager;
		GameAdvancementHook& clock;

		std::vector<TestScript*> test_scripts = std::vector<TestScript*>();

		//// Prototype test script
		//const BuildingType* building_type = building_manager->get_building_type_by_identifier("building_fort");
		//std::cout << "building_fort"
		//		  << " build time is " << building_type->get_build_time() << std::endl;
		//std::cout << "building_fort"
		//		  << " identifier is " << building_type->get_identifier() << std::endl;
		//std::cout << "building_fort"
		//		  << " max level is " << int(building_type->get_max_level()) << std::endl;
		//for (const auto& good : good_manager->get_goods())
		//	std::cout << good.get_identifier() << " price = " << good.get_base_price() << std::endl;

		Testing(GameManager& g_manager);
		~Testing();

		void report_results();
	};
}
