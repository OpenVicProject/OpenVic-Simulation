#pragma once
#include <memory>
#include <vector>

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/testing/Requirement.hpp"

namespace OpenVic {

	class TestScript {

		std::vector<std::unique_ptr<Requirement>> requirements;
		DefinitionManager const* PROPERTY_RW(definition_manager, nullptr);
		std::string PROPERTY(script_name);

	public:
		TestScript(std::string_view new_script_name);
		virtual ~TestScript() = default;

		// expects an overriden method that performs arbitrary code execution
		// so that each script uniquely performs tests
		// for both requirement adding to script and to execute code
		virtual void add_requirements() = 0;
		virtual void execute_script() = 0;

		// Getters
		std::vector<std::unique_ptr<Requirement>>& get_requirements();
		Requirement* get_requirement_at_index(int index);
		Requirement* get_requirement_by_id(std::string id);
		std::vector<Requirement*> get_passed_requirements();
		std::vector<Requirement*> get_failed_requirements();
		std::vector<Requirement*> get_untested_requirements();

		// Setters
		void set_requirements(std::vector<std::unique_ptr<Requirement>>&& in_requirements);
		void add_requirement(Requirement* req);

		// Methods
		void pass_or_fail_req_with_actual_and_target_values(
			std::string req_name, std::string target_value, std::string actual_value
		);
	};
}
