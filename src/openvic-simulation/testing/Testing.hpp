#pragma once
#include <testing/TestScript.hpp>
#include <GameManager.hpp>
#include <iostream>
#include <vector>

namespace OpenVic {

	class Testing {

	public:
		GameManager* game_manager;
		Map* map;
		BuildingManager* building_manager;
		GoodManager* good_manager;
		PopManager* pop_manager;
		GameAdvancementHook* clock;

		Testing(GameManager* g_manager) {

			game_manager = g_manager;
			map = game_manager->get_map();
			building_manager = game_manager->get_building_manager();
			good_manager = game_manager->get_good_manager();
			clock = game_manager->get_game_advancement_hook();

			const BuildingType* building_type = building_manager->get_building_type_by_identifier("building_fort");
			std::cout << "building_fort"
					  << " build time is " << building_type->get_build_time() << std::endl;
			std::cout << "building_fort"
					  << " identifier is " << building_type->get_identifier() << std::endl;
			std::cout << "building_fort"
					  << " max level is " << int(building_type->get_max_level()) << std::endl;
			for (const auto& good : good_manager->get_goods())
				std::cout << good.get_identifier() << " price = " << good.get_base_price() << std::endl;

			// Default test script for now
			TestScript testScript1 = TestScript();
			execute_script(testScript1);
		}

		void execute_script(TestScript test_script);
	};
}
