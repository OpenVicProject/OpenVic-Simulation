#pragma once

#include <memory>
#include <vector>

#include <spdlog/spdlog.h>

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/testing/TestScript.hpp"
#include "openvic-simulation/testing/test_scripts/A_001_file_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_002_economy_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_003_military_unit_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_004_networking_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_005_nation_tests.cpp"
#include "openvic-simulation/testing/test_scripts/A_006_politics_tests.cpp"

namespace OpenVic {

	class Testing {
		std::shared_ptr<spdlog::logger> logger;

	public:
		Testing(DefinitionManager const& definition_manager, std::shared_ptr<spdlog::logger>& logger);

		memory::vector<memory::unique_base_ptr<TestScript>> test_scripts;

		void execute_all_scripts();
		void report_results();
		void report_result(memory::string req_title, memory::vector<memory::unique_ptr<Requirement>>& reqs);
		void report_result(memory::string req_title, memory::vector<Requirement*>& reqs);
	};
}
