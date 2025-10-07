#include "Testing.hpp"

#include <memory>

#include "openvic-simulation/testing/TestScript.hpp"
#include "openvic-simulation/utility/Containers.hpp"

using namespace OpenVic;

Testing::Testing(DefinitionManager const& definition_manager, std::shared_ptr<spdlog::logger>& logger) : logger { logger } {

	// Constructor for the tests will add requirements
	// Then execute the script
	test_scripts.push_back(memory::make_unique<A_001_file_tests>());
	test_scripts.push_back(memory::make_unique<A_002_economy_tests>());
	test_scripts.push_back(memory::make_unique<A_003_military_unit_tests>());
	test_scripts.push_back(memory::make_unique<A_004_networking_tests>());
	test_scripts.push_back(memory::make_unique<A_005_nation_tests>());
	test_scripts.push_back(memory::make_unique<A_006_politics_tests>());

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
	for (auto& test_script : test_scripts) {
		memory::vector<memory::unique_ptr<Requirement>>& reqs = test_script->get_requirements();
		memory::vector<Requirement*> passed_reqs = test_script->get_passed_requirements();
		memory::vector<Requirement*> failed_reqs = test_script->get_failed_requirements();
		memory::vector<Requirement*> untested_reqs = test_script->get_untested_requirements();

		logger->info("{}:", test_script->get_script_name());
		report_result("Requirements for Test", reqs);
		report_result("Passed Requirements", passed_reqs);
		report_result("Failed Requirements", failed_reqs);
		report_result("Untested Requirements", untested_reqs);
	}
}

void Testing::report_result(memory::string req_title, memory::vector<memory::unique_ptr<Requirement>>& reqs) {
	logger->info("\t{}", req_title);
	for (auto& req : reqs) {
		logger->info("\t\t{} ", req->get_id());
	}
	if (reqs.size() < 1) {
		logger->info("\t\tNone");
	}
	logger->info("");
}

void Testing::report_result(memory::string req_title, memory::vector<Requirement*>& reqs) {
	logger->info("\t{}", req_title);
	for (auto req : reqs) {
		logger->info("\t\t{} ", req->get_id());
	}
	if (reqs.size() < 1) {
		logger->info("\t\tNone");
	}
	logger->info("");
}
