#include "openvic-simulation/pathfinding/PointMap.hpp"

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch.hpp>
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>

using namespace OpenVic;

TEST_CASE("PointMap Add/Remove", "[point-map][point-map-add-remove]") {
	PointMap a;

	// Manual tests.
	a.add_point(1, ivec2_t(0, 0));
	a.add_point(2, ivec2_t(0, 1));
	a.add_point(3, ivec2_t(1, 1));
	a.add_point(4, ivec2_t(2, 0));
	a.connect_points(1, 2, true);
	a.connect_points(1, 3, true);
	a.connect_points(1, 4, false);

	CHECK(a.are_points_connected(2, 1));
	CHECK(a.are_points_connected(4, 1));
	CHECK(a.are_points_connected(2, 1, false));
	CHECK_FALSE(a.are_points_connected(4, 1, false));

	a.disconnect_points(1, 2, true);
	CHECK(a.get_point_connections(1).size() == 2); // 3, 4
	CHECK(a.get_point_connections(2).size() == 0);

	a.disconnect_points(4, 1, false);
	CHECK(a.get_point_connections(1).size() == 2); // 3, 4
	CHECK(a.get_point_connections(4).size() == 0);

	a.disconnect_points(4, 1, true);
	CHECK(a.get_point_connections(1).size() == 1); // 3
	CHECK(a.get_point_connections(4).size() == 0);

	a.connect_points(2, 3, false);
	CHECK(a.get_point_connections(2).size() == 1); // 3
	CHECK(a.get_point_connections(3).size() == 1); // 1

	a.connect_points(2, 3, true);
	CHECK(a.get_point_connections(2).size() == 1); // 3
	CHECK(a.get_point_connections(3).size() == 2); // 1, 2

	a.disconnect_points(2, 3, false);
	CHECK(a.get_point_connections(2).size() == 0);
	CHECK(a.get_point_connections(3).size() == 2); // 1, 2

	a.connect_points(4, 3, true);
	CHECK(a.get_point_connections(3).size() == 3); // 1, 2, 4
	CHECK(a.get_point_connections(4).size() == 1); // 3

	a.disconnect_points(3, 4, false);
	CHECK(a.get_point_connections(3).size() == 2); // 1, 2
	CHECK(a.get_point_connections(4).size() == 1); // 3

	a.remove_point(3);
	CHECK(a.get_point_connections(1).size() == 0);
	CHECK(a.get_point_connections(2).size() == 0);
	CHECK(a.get_point_connections(4).size() == 0);

	a.add_point(0, ivec2_t(0, -1));
	a.add_point(3, ivec2_t(2, 1));
	// 0: (0, -1)
	// 1: (0, 0)
	// 2: (0, 1)
	// 3: (2, 1)
	// 4: (2, 0)

	// Tests for get_closest_position_in_segment.
	a.connect_points(2, 3);
	CHECK(
		a.get_closest_position_in_segment(
			fvec2_t { fixed_point_t::_0_50, fixed_point_t::_0_50 }
		) == fvec2_t { fixed_point_t::_0_50, 1 }
	);

	a.connect_points(3, 4);
	a.connect_points(0, 3);
	a.connect_points(1, 4);
	a.disconnect_points(1, 4, false);
	a.disconnect_points(4, 3, false);
	a.disconnect_points(3, 4, false);

	static constexpr fixed_point_t _1_75 = fixed_point_t::_1 / 4 * 7;
	static constexpr fixed_point_t _0_75 = fixed_point_t::_1 / 4 * 3;
	// Remaining edges: <2, 3>, <0, 3>, <1, 4> (directed).
	CHECK(a.get_closest_position_in_segment(fvec2_t(2, fixed_point_t::_0_50)) == fvec2_t(_1_75, _0_75));
	CHECK(a.get_closest_position_in_segment(fvec2_t(-1, fixed_point_t::_0_20)) == fvec2_t(0));
	CHECK(a.get_closest_position_in_segment(fvec2_t(3, 2)) == fvec2_t(2, 1));

	std::srand(0);

	// Random tests for connectivity checks
	for (int i = 0; i < 20000; i++) {
		int u = std::rand() % 5;
		int v = std::rand() % 4;
		if (u == v) {
			v = 4;
		}
		if (std::rand() % 2 == 1) {
			// Add a (possibly existing) directed edge and confirm connectivity.
			a.connect_points(u, v, false);
			CHECK(a.are_points_connected(u, v, false));
		} else {
			// Remove a (possibly nonexistent) directed edge and confirm disconnectivity.
			a.disconnect_points(u, v, false);
			CHECK_FALSE(a.are_points_connected(u, v, false));
		}
	}

	// Random tests for point removal.
	for (int i = 0; i < 20000; i++) {
		a.clear();
		for (int j = 0; j < 5; j++) {
			a.add_point(j, ivec2_t(0, 0));
		}

		// Add or remove random edges.
		for (int j = 0; j < 10; j++) {
			int u = std::rand() % 5;
			int v = std::rand() % 4;
			if (u == v) {
				v = 4;
			}
			if (std::rand() % 2 == 1) {
				a.connect_points(u, v, false);
			} else {
				a.disconnect_points(u, v, false);
			}
		}

		// Remove point 0.
		a.remove_point(0);
		// White box: this will check all edges remaining in the segments set.
		for (int j = 1; j < 5; j++) {
			CHECK_FALSE(a.are_points_connected(0, j, true));
		}
	}
	// It's been great work, cheers. \(^ ^)/
}
