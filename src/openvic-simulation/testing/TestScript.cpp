#include <memory>
#include <string_view>
#include <utility>

#include <testing/Requirement.hpp>
#include <testing/TestScript.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;

TestScript::TestScript(std::string_view new_script_name) : script_name { new_script_name } {}

// Getters
std::vector<std::unique_ptr<Requirement>>& TestScript::get_requirements() {
	return requirements;
}
Requirement* TestScript::get_requirement_at_index(int index) {
	return requirements[index].get();
}
Requirement* TestScript::get_requirement_by_id(std::string_view id) {
	for (std::unique_ptr<Requirement> const& req : requirements) {
		if (req->get_id() == id) {
			return req.get();
		}
	}
	return new Requirement("NULL"sv, "NULL"sv, "NULL"sv); // edge case of failing to find
}
std::vector<Requirement*> TestScript::get_passed_requirements() {
	std::vector<Requirement*> passed_requirements = std::vector<Requirement*>();
	for (auto& req : requirements) {
		if (req->get_pass()) {
			passed_requirements.push_back(req.get());
		}
	}
	return passed_requirements;
}
std::vector<Requirement*> TestScript::get_failed_requirements() {
	std::vector<Requirement*> failed_requirements = std::vector<Requirement*>();
	for (auto& req : requirements) {
		if (!req->get_pass() && req->get_tested()) {
			failed_requirements.push_back(req.get());
		}
	}
	return failed_requirements;
}
std::vector<Requirement*> TestScript::get_untested_requirements() {
	std::vector<Requirement*> untested_requirements = std::vector<Requirement*>();
	for (auto& req : requirements) {
		if (!req->get_tested()) {
			untested_requirements.push_back(req.get());
		}
	}
	return untested_requirements;
}

// Setters
void TestScript::set_requirements(std::vector<std::unique_ptr<Requirement>>&& in_requirements) {
	requirements = std::move(in_requirements);
}
void TestScript::add_requirement(Requirement* req) {
	requirements.push_back(std::unique_ptr<Requirement>{ req });
}

// Methods
void TestScript::pass_or_fail_req_with_actual_and_target_values(
	std::string_view req_name, std::string_view target_value, std::string_view actual_value
) {
	Requirement* req = get_requirement_by_id(req_name);
	req->set_target_value(target_value);
	req->set_actual_value(actual_value);
	if (target_value == actual_value) {
		req->set_pass(true);
	} else {
		req->set_pass(false);
	}
}
