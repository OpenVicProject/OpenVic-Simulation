#include "Testing.hpp"

#include <fstream>
#include <memory>

#include "openvic-simulation/testing/TestScript.hpp"

using namespace OpenVic;

Testing::Testing(DefinitionManager const& definition_manager) {

	// Constructor for the tests will add requirements
	// Then execute the script
	test_scripts.push_back(std::make_unique<A_001_file_tests>());
	test_scripts.push_back(std::make_unique<A_002_economy_tests>());
	test_scripts.push_back(std::make_unique<A_003_military_unit_tests>());
	test_scripts.push_back(std::make_unique<A_004_networking_tests>());
	test_scripts.push_back(std::make_unique<A_005_nation_tests>());
	test_scripts.push_back(std::make_unique<A_006_politics_tests>());

	for (auto& test_script : test_scripts) {
		test_script->set_definition_manager(&definition_manager);
	}
}

void Testing::execute_all_scripts() {
	for (auto& test_script : test_scripts) {
		test_script->execute_script();
	}
}

void Testing::report_results() {
	std::ofstream test_results;
	// _mkdir("../src/openvic - simulation/testing/test_results"); - replace with compatible version (boost?)
	test_results.open("../src/openvic-simulation/testing/test_results/results.txt");
	for (auto& test_script : test_scripts) {
		std::vector<std::unique_ptr<Requirement>>& reqs = test_script->get_requirements();
		std::vector<Requirement*> passed_reqs = test_script->get_passed_requirements();
		std::vector<Requirement*> failed_reqs = test_script->get_failed_requirements();
		std::vector<Requirement*> untested_reqs = test_script->get_untested_requirements();

		test_results << test_script->get_script_name() << ":" << '\n';
		report_result("Requirements for Test", test_results, reqs);
		report_result("Passed Requirements", test_results, passed_reqs);
		report_result("Failed Requirements", test_results, failed_reqs);
		report_result("Untested Requirements", test_results, untested_reqs);

		test_results << "\n\n";
	}
	test_results.close();
}

void Testing::report_result(std::string req_title, std::ofstream& outfile, std::vector<std::unique_ptr<Requirement>>& reqs) {
	outfile << "\t" << req_title << '\n';
	outfile << "\t";
	for (auto& req : reqs) {
		outfile << req->get_id() << " ";
	}
	if (reqs.size() < 1) {
		outfile << "None";
	}
	outfile << "\n\n";
}

void Testing::report_result(std::string req_title, std::ofstream& outfile, std::vector<Requirement*>& reqs) {
	outfile << "\t" << req_title << '\n';
	outfile << "\t";
	for (auto req : reqs) {
		outfile << req->get_id() << " ";
	}
	if (reqs.size() < 1) {
		outfile << "None";
	}
	outfile << "\n\n";
}
