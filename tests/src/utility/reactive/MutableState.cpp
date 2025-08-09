#include "openvic-simulation/utility/reactive//MutableState.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("MutableState", "[MutableState]") {
	int sum = 0;
	MutableState<int> mutable_state(0);
	connection conn = mutable_state.connect([&sum](const int new_value) { sum += new_value; });
	CHECK(sum == 0);
	mutable_state.set(1);
	CHECK(sum == 1);
	mutable_state.set(2);
	CHECK(sum == 3);
	mutable_state.set(2);
	CHECK(sum == 5);
	conn.disconnect();
	mutable_state.set(3);
	CHECK(sum == 5);
}