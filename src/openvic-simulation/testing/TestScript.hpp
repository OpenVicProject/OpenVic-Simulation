#pragma once
#include <memory>
#include <vector>

#include "openvic-simulation/DefinitionManager.hpp"
#include "openvic-simulation/testing/Requirement.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {

	class TestScript {

		memory::vector<memory::unique_ptr<Requirement>> requirements;
		DefinitionManager const* PROPERTY_RW(definition_manager, nullptr);
		memory::string PROPERTY(script_name);

	public:
		TestScript(std::string_view new_script_name);
		virtual ~TestScript() = default;

		// expects an overridden method that performs arbitrary code execution
		// so that each script uniquely performs tests
		// for both requirement adding to script and to execute code
		virtual void add_requirements() = 0;
		virtual void execute_script() = 0;

		// Getters
		memory::vector<memory::unique_ptr<Requirement>>& get_requirements();
		Requirement* get_requirement_at_index(int index);
		Requirement* get_requirement_by_id(memory::string id);
		memory::vector<Requirement*> get_passed_requirements();
		memory::vector<Requirement*> get_failed_requirements();
		memory::vector<Requirement*> get_untested_requirements();

		// Setters
		void set_requirements(memory::vector<memory::unique_ptr<Requirement>>&& in_requirements);
		void add_requirement(memory::unique_ptr<Requirement>&& req);

		// Methods
		void pass_or_fail_req_with_actual_and_target_values(
			memory::string req_name, memory::string target_value, memory::string actual_value
		);
	};
}
