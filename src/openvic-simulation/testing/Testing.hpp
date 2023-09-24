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

		Testing(GameManager* game_manager);
		~Testing();

		std::vector<TestScript*> test_scripts = std::vector<TestScript*>();

		void execute_all_scripts();
		void report_results();
	};
}
