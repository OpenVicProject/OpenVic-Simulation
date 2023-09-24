#pragma once
#include <testing/Requirement.hpp>
#include <vector>
#include <GameManager.hpp>

namespace OpenVic {

	class TestScript {

		std::vector<Requirement*> requirements = std::vector<Requirement*>();

	public:

		// expects an overriden method that performs arbitrary code execution
		// so that each script uniquely performs tests
		// for both requirement adding to script and to execute code
		virtual void add_requirements() = 0;
		virtual void execute_script(GameManager& game_manager) = 0;

		// Getters
		std::vector<Requirement*> get_requirements();
		Requirement* get_requirement_at_index(int index);
		Requirement* get_requirement_by_id(std::string id);
		std::vector<Requirement*> get_passed_requirements();
		std::vector<Requirement*> get_failed_requirements();

		// Setters
		void set_requirements(std::vector<Requirement*> in_requirements);
		void add_requirement(Requirement* req);
	};
}
