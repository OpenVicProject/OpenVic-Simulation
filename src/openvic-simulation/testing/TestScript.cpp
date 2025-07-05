#include <memory>

#include <testing/Requirement.hpp>
#include <testing/TestScript.hpp>

using namespace OpenVic;

TestScript::TestScript(std::string_view new_script_name) : script_name { new_script_name } {}

// Getters
memory::vector<memory::unique_ptr<Requirement>>& TestScript::get_requirements() {
	return requirements;
}
Requirement* TestScript::get_requirement_at_index(int index) {
	return requirements[index].get();
}
Requirement* TestScript::get_requirement_by_id(memory::string id) {
	for (auto& req : requirements) {
		if (req->get_id() == id) {
			return req.get();
		}
	}
	return nullptr; // edge case of failing to find
}
memory::vector<Requirement*> TestScript::get_passed_requirements() {
	memory::vector<Requirement*> passed_requirements = memory::vector<Requirement*>();
	for (auto& req : requirements) {
		if (req->get_pass()) {
			passed_requirements.push_back(req.get());
		}
	}
	return passed_requirements;
}
memory::vector<Requirement*> TestScript::get_failed_requirements() {
	memory::vector<Requirement*> failed_requirements = memory::vector<Requirement*>();
	for (auto& req : requirements) {
		if (!req->get_pass() && req->get_tested()) {
			failed_requirements.push_back(req.get());
		}
	}
	return failed_requirements;
}
memory::vector<Requirement*> TestScript::get_untested_requirements() {
	memory::vector<Requirement*> untested_requirements = memory::vector<Requirement*>();
	for (auto& req : requirements) {
		if (!req->get_tested()) {
			untested_requirements.push_back(req.get());
		}
	}
	return untested_requirements;
}

// Setters
void TestScript::set_requirements(memory::vector<memory::unique_ptr<Requirement>>&& in_requirements) {
	requirements = std::move(in_requirements);
}
void TestScript::add_requirement(memory::unique_ptr<Requirement>&& req) {
	requirements.push_back(std::move(req));
}

// Methods
void TestScript::pass_or_fail_req_with_actual_and_target_values(
	memory::string req_name, memory::string target_value, memory::string actual_value
) {
	Requirement* req = get_requirement_by_id(req_name);
	if (req == nullptr){
		return;
	}
	req->set_target_value(target_value);
	req->set_actual_value(actual_value);
	if (target_value == actual_value) {
		req->set_pass(true);
	} else {
		req->set_pass(false);
	}
}
