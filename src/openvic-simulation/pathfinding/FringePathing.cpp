#include "openvic-simulation/pathfinding/FringePathing.hpp"

#include <vector>

#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/heap_algorithm.hpp>

#include "openvic-simulation/pathfinding/PointMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

using namespace OpenVic;

bool FringePathing::_solve(search_iterator begin_point, search_iterator end_point, uint64_t pass, bool allow_partial_path) {
	last_closest_point = search.end();

	if (!_is_point_enabled(end_point) && !allow_partial_path) {
		return false;
	}

	bool found_route = false;

	std::vector<search_iterator> fringe;
	fringe.reserve(20);

	begin_point.value().g_score = 0;
	begin_point.value().f_score = _estimate_cost(begin_point, end_point);
	begin_point.value().open_pass = pass;
	fringe.push_back(begin_point);

	fixed_point_t f_limit = begin_point.value().f_score;
	fixed_point_t next_f_limit = fixed_point_t::max();

	while (!fringe.empty() && !found_route) {
		ranges::pop_heap(fringe, FringeSort {});
		search_iterator p = fringe.back();
		fringe.pop_back();

		if (p.value().open_pass != pass) {
			continue;
		}

		if (p.value().f_score > f_limit) {
			f_limit = next_f_limit;
			next_f_limit = fixed_point_t::max();
			fringe.push_back(p);
			ranges::push_heap(fringe, FringeSort {});
			continue;
		}

		if (p.value().f_score > f_limit && p.value().f_score < next_f_limit) {
			next_f_limit = p.value().f_score;
		}

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (last_closest_point == search.end() || last_closest_point.value().abs_f_score > p.value().abs_f_score ||
			(last_closest_point.value().abs_f_score >= p.value().abs_f_score && //
			 last_closest_point.value().g_score > p.value().g_score)) {
			last_closest_point = p;
		}

		if (p == end_point) {
			found_route = true;
			break;
		}

		p.value().closed_pass = pass;

		for (PointMap::points_key_type const& neighbor_id : p.value().point->neighbors) {
			search_iterator e = search.find(neighbor_id); // The neighbor point.
			if (e == search.end()) {
				PointMap::points_value_type const* neighbor_point = point_map->get_point(neighbor_id);
				if (neighbor_point == nullptr) {
					continue;
				}
				e = search.insert({ neighbor_id, { neighbor_point } }).first;
			}

			if (!_is_point_enabled(e) || e.value().closed_pass == pass) {
				continue;
			}

			fixed_point_t tentative_g_score = p.value().g_score + _compute_cost(p, e) * e.value().point->weight_scale;
			bool new_point = e.value().open_pass != pass;

			if (new_point) { // The point wasn't inside the open list.
				e.value().open_pass = pass;
				e.value().abs_f_score = _estimate_cost(p, e);
				e.value().f_score = tentative_g_score + e.value().abs_f_score;
			} else if (tentative_g_score >= e.value().g_score) { // The new path is worse than the previous.
				continue;
			} else {
				e.value().abs_f_score = e.value().f_score - e.value().g_score;
				e.value().f_score = tentative_g_score + e.value().abs_f_score;
			}

			e.value().g_score = tentative_g_score;
			e.value().prev_point = p;

			if (new_point) {
				fringe.push_back(e);
				ranges::push_heap(fringe, FringeSort {});
			} else {
				decltype(fringe)::iterator temp_it = ranges::find(fringe, e);
				ranges::push_heap(fringe.begin(), temp_it + 1, FringeSort {});
			}
		}
	}

	return found_route;
}
