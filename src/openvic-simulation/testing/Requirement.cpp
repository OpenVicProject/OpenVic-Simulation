#include <testing/Requirement.hpp>

using namespace OpenVic;

void Requirement::set_pass(bool in_pass) {
	pass = in_pass;
	set_tested(true); // Ever setting a pass condition implies it has been tested
}

void Requirement::set_target_value(std::string_view new_target_value) {
	target_value = new_target_value;
}

void Requirement::set_actual_value(std::string_view new_actual_value) {
	actual_value = new_actual_value;
}
