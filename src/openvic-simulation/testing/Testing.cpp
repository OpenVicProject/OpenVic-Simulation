#include <testing/Testing.hpp>
#include <testing/TestScript.hpp>
#include <fstream>
#include <direct.h>

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

	for (int i = 0; i < test_scripts.size(); i++) {
		test_scripts[i]->set_game_manager(game_manager);
	}
}

Testing::~Testing() {
	for (TestScript* test_script : test_scripts) {
		delete test_script;
	}
}

void Testing::execute_all_scripts() {
	for (int i = 0; i < test_scripts.size(); i++) {
		test_scripts[i]->execute_script();
	}
}

void Testing::report_results() {
	std::ofstream test_results;
	// _mkdir("..\\src\\openvic - simulation\\testing\\test_results"); - replace with compatible version (boost?)
	test_results.open("..\\src\\openvic-simulation\\testing\\test_results\\results.txt");
	for (auto test_script : test_scripts) {
		std::vector<OpenVic::Requirement*> reqs = test_script->get_requirements();
		std::vector<OpenVic::Requirement*> passed_reqs = test_script->get_passed_requirements();
		std::vector<OpenVic::Requirement*> failed_reqs = test_script->get_failed_requirements();
		std::vector<OpenVic::Requirement*> untested_reqs = test_script->get_untested_requirements();

		test_results << test_script->get_script_name() << ":" << std::endl;
		test_results << "\t" << "Requirements for Test" << std::endl;
		test_results << "\t";
		for (auto req : reqs) {
			test_results << req->get_id() << " ";
		}
		if (reqs.size() < 1) test_results << "None";
		test_results << std::endl << std::endl;

		test_results << "\t"<< "Passed Requirements" << std::endl;
		test_results << "\t";
		for (auto req : passed_reqs) {
			test_results << req->get_id() << " ";
		}
		if (passed_reqs.size() < 1) test_results << "None";
		test_results << std::endl << std::endl;

		test_results << "\t" << "Failed Requirements" << std::endl;
		test_results << "\t";
		for (auto req : failed_reqs) {
			test_results << req->get_id() << " ";
		}
		if (failed_reqs.size() < 1) test_results << "None";
		test_results << std::endl << std::endl;

		test_results << "\t" << "Untested Requirements" << std::endl;
		test_results << "\t";
		for (auto req : untested_reqs) {
			test_results << req->get_id() << " ";
		}
		if (untested_reqs.size() < 1) test_results << "None";
		test_results << std::endl << std::endl;

		test_results << std::endl<< std::endl;
	}
	test_results.close();

	// Create Summary File

}
