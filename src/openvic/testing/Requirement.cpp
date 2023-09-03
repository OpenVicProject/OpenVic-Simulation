#include <testing/Requirement.hpp>

using namespace OpenVic;

// Getters
std::string Requirement::get_id() { return id; }
std::string Requirement::get_text() { return text; }
std::string Requirement::get_acceptance_criteria() { return acceptance_criteria; }
bool Requirement::get_pass() { return pass; }

// Setters
void Requirement::set_id(std::string in_id) { id = in_id; }
void Requirement::set_text(std::string in_text) { text = in_text; }
void Requirement::set_acceptance_criteria(std::string in_acceptance_criteria) { acceptance_criteria = in_acceptance_criteria; }
void Requirement::set_pass(bool in_pass) { pass = in_pass; }
