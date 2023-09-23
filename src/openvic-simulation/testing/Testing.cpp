#include <testing/Testing.hpp>
#include <testing/TestScript.hpp>

using namespace OpenVic;

Testing::Testing(GameManager& g_manager)
	: game_manager { g_manager },
	  map { g_manager.get_map() },
	  building_manager { g_manager.get_building_manager() },
	  good_manager { g_manager.get_good_manager() },
	  pop_manager { g_manager.get_pop_manager() },
	  clock { g_manager.get_clock() } {

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

Testing::~Testing() {
	for (TestScript* test_script : test_scripts) {
		delete test_script;
	}
}

void Testing::report_results() {
	for (int i = 0; i < test_scripts.size(); i++) {

	}

	// Create Summary File

}
