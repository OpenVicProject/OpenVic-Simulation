#include <testing/Requirement.hpp>

using namespace OpenVic;

void Requirement::set_pass(bool in_pass) {
	pass = in_pass;
	set_tested(true); // Ever setting a pass condition implies it has been tested
}
