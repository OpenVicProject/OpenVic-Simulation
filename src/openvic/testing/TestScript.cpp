#include <testing/TestScript.hpp>

using namespace OpenVic;

// Getters
std::vector<Requirement> TestScript::get_requirements() { return requirements; }
Requirement TestScript::get_requirement_at_index(int index) { return requirements[index]; }
Requirement TestScript::get_requirement_by_id(std::string id) {
	for (int i = 0; i < requirements.size(); i++) {
		if (requirements[i].get_id() == id) return requirements[i];
	}
	return Requirement("NULL", "NULL", "NULL");
}

// Setters
void TestScript::set_requirements(std::vector<Requirement> in_requirements) { requirements = in_requirements; }
void TestScript::add_requirement(Requirement req) {	requirements.push_back(req); }
