#pragma once
#include <iostream>
#include <vector>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/testing/TestScript.hpp"
#include "openvic-simulation/testing/test_scripts/A_001_file_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_002_economy_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_003_military_unit_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_004_networking_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_005_nation_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_006_politics_tests.cpp"

namespace OpenVic {

	class Testing {

	public:
		Testing(GameManager* game_manager);
		~Testing();

		std::vector<TestScript*> test_scripts = std::vector<TestScript*>();

		void execute_all_scripts();
		void report_results();
		void report_result(std::string req_title, std::ofstream& outfile, std::vector<Requirement*> reqs);
	};
}
