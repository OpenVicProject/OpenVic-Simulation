#include "openvic-simulation/core/reactive/DerivedState.hpp"

#include "openvic-simulation/core/Signal.hpp"
#include "openvic-simulation/core/reactive/DependencyTracker.hpp"
#include "openvic-simulation/core/reactive/MutableState.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("DerivedState untracked", "[DerivedState-untracked]") {
	MutableState<int> mutable_state_a(0);
	MutableState<int> mutable_state_b(0);
	DerivedState<int> sum(
		[&a = static_cast<ReadOnlyMutableState<int>&>(mutable_state_a),
		 &b = static_cast<ReadOnlyMutableState<int>&>(mutable_state_b)](DependencyTracker& tracker) -> int {
			return a.get(tracker) + b.get(tracker);
		}
	);

	CHECK(sum.get_untracked() == 0);
	mutable_state_a.set(1);
	CHECK(sum.get_untracked() == 1);
	mutable_state_b.set(2);
	CHECK(sum.get_untracked() == 3);
}

TEST_CASE("DerivedState reactive", "[DerivedState-reactive]") {
	MutableState<int> mutable_state_a(0);
	MutableState<int> mutable_state_b(0);
	DerivedState<int> sum(
		[&a = static_cast<ReadOnlyMutableState<int>&>(mutable_state_a),
		 &b = static_cast<ReadOnlyMutableState<int>&>(mutable_state_b)](DependencyTracker& tracker) -> int {
			return a.get(tracker) + b.get(tracker);
		}
	);

	int times_marked_dirty = 0;
	connection conn = sum.connect([&times_marked_dirty]() -> void {
		++times_marked_dirty;
	});

	mutable_state_a.set(1);
	CHECK(times_marked_dirty == 0); // starts dirty
	CHECK(sum.get_untracked() == 1); // force update

	mutable_state_b.set(2);
	CHECK(times_marked_dirty == 1);

	mutable_state_a.set(2);
	CHECK(times_marked_dirty == 1);

	CHECK(sum.get_untracked() == 4);
	mutable_state_a.set(3);
	CHECK(times_marked_dirty == 2);
}

TEST_CASE("DerivedState untracked in callback", "[DerivedState-untracked-in-callback]") {
	MutableState<int> mutable_state_a(0);
	MutableState<int> mutable_state_b(0);
	DerivedState<int> sum(
		[&a = static_cast<ReadOnlyMutableState<int>&>(mutable_state_a),
		 &b = static_cast<ReadOnlyMutableState<int>&>(mutable_state_b)](DependencyTracker& tracker) -> int {
			return a.get(tracker) + b.get(tracker);
		}
	);

	connection conn;
	int sum_value = sum.get([&sum, &conn, &sum_value](OpenVic::signal<>& sum_changed) mutable -> void {
		conn = sum_changed.connect([&sum, &sum_value]() mutable -> void {
			sum_value = sum.get_untracked();
		});
	});
	CHECK(sum_value == 0);
	mutable_state_a.set(1);
	CHECK(sum_value == 1);
	mutable_state_a.set(2);
	CHECK(sum_value == 2);
}
