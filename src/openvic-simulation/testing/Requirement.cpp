#include <testing/Requirement.hpp>

using namespace OpenVic;

// Getters
std::string Requirement::get_id() { return id; }
std::string Requirement::get_text() { return text; }
std::string Requirement::get_acceptance_criteria() { return acceptance_criteria; }
bool Requirement::get_pass() { return pass; }
bool Requirement::get_tested() { return tested; }
std::string Requirement::get_target_value() { return target_value; }
std::string Requirement::get_actual_value() { return actual_value; }

// Setters
void Requirement::set_id(std::string in_id) { id = in_id; }
void Requirement::set_text(std::string in_text) { text = in_text; }
void Requirement::set_acceptance_criteria(std::string in_acceptance_criteria) { acceptance_criteria = in_acceptance_criteria; }
void Requirement::set_pass(bool in_pass) {
	pass = in_pass;
	set_tested(true);	// Ever setting a pass condition implies it has been tested
}
void Requirement::set_tested(bool in_tested) { tested = in_tested; }
void Requirement::set_target_value(std::string in_target_value) { target_value = in_target_value; }
void Requirement::set_actual_value(std::string in_actual_value) { actual_value = in_actual_value; }
