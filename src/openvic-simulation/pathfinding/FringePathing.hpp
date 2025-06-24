#pragma once

#include "openvic-simulation/pathfinding/PathingBase.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	struct FringePathingNode final : public PathingNodeBase<FringePathingNode> {
		using PathingNodeBase::PathingNodeBase;

		// Used for pathfinding.
		uint64_t open_pass = 0;
		uint64_t closed_pass = 0;

		// Used for pathfinding.
		fixed_point_t g_score = 0;
		fixed_point_t f_score = -1;

		// Used for getting closest_point_of_last_pathing_call.
		fixed_point_t abs_f_score = 0;
	};

	/** Fringe Pathfinding implementation
	 */
	struct FringePathing : public PathingBase<FringePathingNode> {
		using PathingBase::PathingBase;

		struct FringeSort {
			// Returns true when the Point A is worse than Point B.
			bool operator()(search_const_iterator A, search_const_iterator B) const {
				if (A.value().f_score > B.value().f_score) {
					return true;
				} else if (A.value().f_score < B.value().f_score) {
					return false;
				} else {
					// If the f_costs are the same then prioritize the points that
					// are further away from the start.
					return A.value().g_score < B.value().g_score;
				}
			}
		};

	protected:
		virtual bool _solve( //
			search_iterator begin_point, search_iterator end_point, uint64_t pass, bool allow_partial_path
		) override;
	};
}
