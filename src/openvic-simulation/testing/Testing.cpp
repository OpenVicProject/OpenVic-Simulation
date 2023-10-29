#include "Testing.hpp"

#include <fstream>

#include "openvic-simulation/testing/TestScript.hpp"

using namespace OpenVic;

Testing::Testing(GameManager* game_manager) {

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

	for (auto test_script : test_scripts) {
		test_script->set_game_manager(game_manager);
	}
}

Testing::~Testing() {
	for (TestScript* test_script : test_scripts) {
		delete test_script;
	}
}

void Testing::execute_all_scripts() {
	for (auto test_script : test_scripts) {
		test_script->execute_script();
	}
}

void Testing::report_results() {
	std::ofstream test_results;
	// _mkdir("../src/openvic - simulation/testing/test_results"); - replace with compatible version (boost?)
	test_results.open("../src/openvic-simulation/testing/test_results/results.txt");
	for (auto test_script : test_scripts) {
		std::vector<Requirement*> reqs = test_script->get_requirements();
		std::vector<Requirement*> passed_reqs = test_script->get_passed_requirements();
		std::vector<Requirement*> failed_reqs = test_script->get_failed_requirements();
		std::vector<Requirement*> untested_reqs = test_script->get_untested_requirements();

		test_results << test_script->get_script_name() << ":" << std::endl;
		report_result("Requirements for Test", test_results, reqs);
		report_result("Passed Requirements", test_results, passed_reqs);
		report_result("Failed Requirements", test_results, failed_reqs);
		report_result("Untested Requirements", test_results, untested_reqs);

		test_results << std::endl << std::endl;
	}
	test_results.close();
}

void Testing::report_result(std::string req_title, std::ofstream& outfile, std::vector<Requirement*> reqs) {
	outfile << "\t" << req_title << std::endl;
	outfile << "\t";
	for (auto req : reqs) {
		outfile << req->get_id() << " ";
	}
	if (reqs.size() < 1) {
		outfile << "None";
	}
	outfile << std::endl << std::endl;
}
