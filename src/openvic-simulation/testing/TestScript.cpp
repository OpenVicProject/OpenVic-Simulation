#include <testing/TestScript.hpp>

using namespace OpenVic;

// Getters
std::vector<Requirement*> TestScript::get_requirements() { return requirements; }
Requirement* TestScript::get_requirement_at_index(int index) { return requirements[index]; }
Requirement* TestScript::get_requirement_by_id(std::string id) {
	for (int i = 0; i < requirements.size(); i++) {
		if (requirements[i]->get_id() == id) return requirements[i];
	}
	return new Requirement("NULL", "NULL", "NULL");	// edge case of failing to find
}
std::vector<Requirement*> TestScript::get_passed_requirements() {
	std::vector<Requirement*> passed_requirements = std::vector<Requirement*>();
	for (int i = 0; i < requirements.size(); i++) {
		if (requirements[i]->get_pass()) passed_requirements.push_back(requirements[i]);
	}
	return passed_requirements;
}
std::vector<Requirement*> TestScript::get_failed_requirements() {
	std::vector<Requirement*> failed_requirements = std::vector<Requirement*>();
	for (int i = 0; i < requirements.size(); i++) {
		if (!requirements[i]->get_pass()) failed_requirements.push_back(requirements[i]);
	}
	return failed_requirements;
}

// Setters
void TestScript::set_requirements(std::vector<Requirement*> in_requirements) { requirements = in_requirements; }
void TestScript::add_requirement(Requirement* req) {	requirements.push_back(req); }
