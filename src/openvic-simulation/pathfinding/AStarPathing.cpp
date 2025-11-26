#include "openvic-simulation/pathfinding/AStarPathing.hpp"

#include <tsl/ordered_set.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/heap_algorithm.hpp>

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/military/UnitInstanceGroup.hpp"
#include "openvic-simulation/pathfinding/PointMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Containers.hpp"
#include "openvic-simulation/core/Typedefs.hpp"

using namespace OpenVic;

bool AStarPathing::_solve(search_iterator begin_point, search_iterator end_point, uint64_t pass, bool allow_partial_path) {
	last_closest_point = search.end();

	if (!_is_point_enabled(end_point) && !allow_partial_path) {
		return false;
	}

	bool found_route = false;

	memory::vector<search_iterator> open_list;
	open_list.reserve(20);

	begin_point.value().g_score = 0;
	begin_point.value().f_score = _estimate_cost(begin_point, end_point);
	begin_point.value().abs_f_score = begin_point.value().f_score;
	open_list.push_back(begin_point);

	while (!open_list.empty()) {
		search_iterator p = open_list.front(); // The currently processed point.

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

		ranges::pop_heap(open_list, PointSort {}); // Remove the current point from the open list.
		open_list.pop_back();
		p.value().closed_pass = pass; // Mark the point as closed.

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

			bool new_point = false;

			if (e.value().open_pass != pass) { // The point wasn't inside the open list.
				e.value().open_pass = pass;
				new_point = true;
			} else if (tentative_g_score >= e.value().g_score) { // The new path is worse than the previous.
				continue;
			}

			e.value().prev_point = p;
			e.value().g_score = tentative_g_score;
			e.value().abs_f_score = _estimate_cost(e, end_point);
			e.value().f_score = e.value().g_score + e.value().abs_f_score;

			if (new_point) { // The position of the new points is already known.
				open_list.push_back(e);
				ranges::push_heap(open_list, PointSort {});
			} else if (open_list.size() > 1) {
				decltype(open_list)::iterator temp_it = ranges::find(open_list, e);
				ranges::push_heap(open_list.begin(), temp_it + 1, PointSort {});
			}
		}
	}

	return found_route;
}

ArmyAStarPathing::ArmyAStarPathing(
	MapDefinition const& new_map_definition,
	MapInstance const& new_map_instance
) : map_instance { new_map_instance },
	AStarPathing(&new_map_definition.get_path_map_land())
	{}

bool ArmyAStarPathing::_is_point_enabled(search_const_iterator it) const {
	if (!AStarPathing::_is_point_enabled(it)) {
		return false;
	}

	if (OV_unlikely(army_instance == nullptr)) {
		return true;
	}

	if (army_instance->is_exiled()) {
		return true;
	}

	ProvinceInstance const* province = map_instance.get_province_instance_from_number(it->first);

	if (OV_unlikely(province == nullptr)) {
		return true;
	}

	CountryInstance const* province_owner = province->get_owner();

	if (OV_unlikely(province_owner == nullptr)) {
		return true;
	}

	return army_instance->get_country()->can_army_units_enter(*province_owner);
}

NavyAStarPathing::NavyAStarPathing(
	MapDefinition const& new_map_definition,
	MapInstance const& new_map_instance
) : map_instance {new_map_instance},
	AStarPathing(&new_map_definition.get_path_map_land())
	{}

bool NavyAStarPathing::_is_point_enabled(search_const_iterator it) const {
	if (!AStarPathing::_is_point_enabled(it)) {
		return false;
	}

	if (OV_unlikely(navy_instance == nullptr)) {
		return true;
	}

	ProvinceInstance const* province = map_instance.get_province_instance_from_number(it->first);

	if (OV_unlikely(province == nullptr)) {
		return true;
	}

	if (OV_likely(province->province_definition.is_water())) {
		return true;
	}

	CountryInstance const* province_controller = province->get_controller();

	if (OV_unlikely(province_controller == nullptr)) {
		return true;
	}

	return navy_instance->get_country()->can_navy_units_enter(*province_controller);
}
