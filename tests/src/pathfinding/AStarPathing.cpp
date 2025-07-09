#include "openvic-simulation/pathfinding/AStarPathing.hpp"

#include "openvic-simulation/pathfinding/PointMap.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Containers.hpp"

#include "Helper.hpp"
#include "pathfinding/Pathing.hpp"
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	struct ABC final : public PointMap, public AStarPathing {
		enum { A, B, C, D };

		ABC() : PointMap(), AStarPathing(this) {
			add_point(A, ivec2_t(0, 0));
			add_point(B, ivec2_t(1, 0));
			add_point(C, ivec2_t(0, 1));
			add_point(D, ivec2_t(1, 1));
			connect_points(A, B);
			connect_points(A, C);
			connect_points(B, C);
			connect_points(D, A);
			reset_search();
		}

		// Disable heuristic completely.
		fixed_point_t _compute_cost(search_const_iterator p_from, search_const_iterator p_to) override {
			if (p_from.key() == A && p_to.key() == C) {
				return 1000;
			}
			return 100;
		}
	};
}

TEST_CASE("AStarPathing ABC path", "[astar-pathing][astar-pathing-abc-path]") {
	ABC abc;
	memory::vector<PointMap::points_key_type> path = abc.get_id_path(ABC::A, ABC::C);
	CHECK_IF(path.size() == 3) {
		CHECK(path[0] == ABC::A);
		CHECK(path[1] == ABC::B);
		CHECK(path[2] == ABC::C);
	}
}

TEST_CASE("AStarPathing Stress Find paths", "[astar-pathing][astar-pathing-stress-find-paths]") {
	OpenVic::testing::_stress_test<OpenVic::AStarPathing>();
}
