
#include "openvic-simulation/pathfinding/PointMap.hpp"

#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include <fmt/core.h>

#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Utility.hpp"

using namespace OpenVic;

PointMap::~PointMap() {
	destroyed();
}

int64_t PointMap::get_available_points() const {
	if (points.contains(last_free_id)) {
		int64_t cur_new_id = last_free_id + 1;
		while (points.contains(cur_new_id)) {
			cur_new_id++;
		}
		last_free_id = cur_new_id;
	}

	return last_free_id;
}

bool PointMap::try_add_point(
	points_key_type id, ivec2_t const& position, fixed_point_t weight_scale, std::span<points_key_type> adjacent_points
) {
	if constexpr (!std::is_unsigned_v<points_key_type>) {
		OV_ERR_FAIL_COND_V_MSG(id < 0, false, fmt::format("Can't add a point with negative id: {}.", id));
	}
	OV_ERR_FAIL_COND_V_MSG(
		weight_scale < 0, false, fmt::format("Can't add a point with weight scale less than 0.0: {}.", weight_scale.to_string())
	);

	if (OV_unlikely(points.contains(id))) {
		return false;
	}

	size_t prev_capacity = get_point_capacity();
	points[id] = Point { .position = position, .weight_scale = weight_scale, .enabled = true };
	if (OV_unlikely(prev_capacity != get_point_capacity())) {
		points_pointers_invalidated();
	}

	for (points_key_type const& adj_id : adjacent_points) {
		connect_points(id, adj_id);
	}

	return true;
}

void PointMap::add_point(
	points_key_type id, ivec2_t const& position, fixed_point_t weight_scale, std::span<points_key_type> adjacent_points
) {
	if constexpr (!std::is_unsigned_v<points_key_type>) {
		OV_ERR_FAIL_COND_MSG(id < 0, fmt::format("Can't add a point with negative id: {}.", id));
	}
	OV_ERR_FAIL_COND_MSG(
		weight_scale < 0, fmt::format("Can't add a point with weight scale less than 0.0: {}.", weight_scale.to_string())
	);

	points_iterator found_pt = points.find(id);

	if (found_pt == points.end()) {
		size_t prev_capacity = get_point_capacity();
		points[id] = Point { .position = position, .weight_scale = weight_scale, .enabled = true };
		if (OV_unlikely(get_point_capacity() != prev_capacity)) {
			points_pointers_invalidated();
		}
	} else {
		found_pt.value().position = position;
		found_pt.value().weight_scale = weight_scale;
	}

	for (points_key_type const& adj_id : adjacent_points) {
		connect_points(id, adj_id);
	}
}

PointMap::Point const* PointMap::get_point(points_key_type id) const {
	points_const_iterator it = points.find(id);
	OV_ERR_FAIL_COND_V_MSG(it == points.end(), nullptr, fmt::format("Can't get a point. Point with id: {} doesn't exist.", id));

	return &it.value();
}

PointMap::Point const* PointMap::try_get_point(points_key_type id) const {
	points_const_iterator it = points.find(id);
	if (it == points.end()) {
		return nullptr;
	}

	return &it.value();
}

ivec2_t PointMap::get_point_position(points_key_type id) const {
	points_const_iterator it = points.find(id);
	OV_ERR_FAIL_COND_V_MSG(
		it == points.end(), ivec2_t(), fmt::format("Can't get point's position. Point with id: {} doesn't exist.", id)
	);

	return it.value().position;
}

void PointMap::set_point_position(points_key_type id, ivec2_t const& position) {
	points_iterator it = points.find(id);
	OV_ERR_FAIL_COND_MSG(it == points.end(), fmt::format("Can't set point's position. Point with id: {} doesn't exist.", id));

	it.value().position = position;
}

fixed_point_t PointMap::get_point_weight_scale(points_key_type id) const {
	points_const_iterator it = points.find(id);
	OV_ERR_FAIL_COND_V_MSG(
		it == points.end(), 0, fmt::format("Can't get point's weight scale. Point with id: {} doesn't exist.", id)
	);

	return it.value().weight_scale;
}

void PointMap::set_point_weight_scale(points_key_type id, fixed_point_t weight_scale) {
	points_iterator it = points.find(id);
	OV_ERR_FAIL_COND_MSG(
		it == points.end(), fmt::format("Can't set point's weight scale. Point with id: {} doesn't exist.", id)
	);
	OV_ERR_FAIL_COND_MSG(
		weight_scale < 0, fmt::format("Can't set point's weight scale less than 0.0: {}.", weight_scale.to_double())
	);

	it.value().weight_scale = weight_scale;
}

void PointMap::remove_point(points_key_type id) {
	points_iterator it = points.find(id);
	OV_ERR_FAIL_COND_MSG(it == points.end(), fmt::format("Can't remove point. Point with id: {} doesn't exist.", id));

	for (points_key_type const& neighbor_id : it.value().neighbors) {
		Point& neighbor = points.find(neighbor_id).value();

		Segment s(id, neighbor_id);
		segments.unordered_erase(s);

		neighbor.neighbors.unordered_erase(it.key());
		neighbor.unlinked_neighbors.unordered_erase(it.key());
	}

	for (points_key_type const& neighbor_id : it.value().unlinked_neighbors) {
		Point& neighbor = points.find(neighbor_id).value();

		Segment s(id, neighbor_id);
		segments.unordered_erase(s);

		neighbor.neighbors.unordered_erase(it.key());
		neighbor.unlinked_neighbors.unordered_erase(it.key());
	}

	points.unordered_erase(it);
	last_free_id = id;
	point_invalidated(id);
}

bool PointMap::has_point(points_key_type id) const {
	return points.contains(id);
}

std::span<const PointMap::points_key_type> PointMap::get_point_connections(points_key_type id) const {
	points_const_iterator it = points.find(id);
	OV_ERR_FAIL_COND_V_MSG(
		it == points.end(), std::span<const points_key_type>(),
		fmt::format("Can't get point's connections. Point with id: {} doesn't exist.", id)
	);

	return std::span<const points_key_type> { it.value().neighbors.values_container() };
}

std::vector<PointMap::points_key_type> PointMap::get_point_ids() {
	std::vector<points_key_type> point_list;
	point_list.reserve(points.size());

	for (points_const_iterator it = points.begin(); it != points.end(); it++) {
		point_list.push_back(it.key());
	}

	return point_list;
}

void PointMap::set_point_disabled(points_key_type id, bool disabled) {
	points_iterator it = points.find(id);
	OV_ERR_FAIL_COND_MSG(
		it == points.end(), fmt::format("Can't set if point is disabled. Point with id: {} doesn't exist.", id)
	);

	it.value().enabled = !disabled;
}

bool PointMap::is_point_disabled(points_key_type id) const {
	points_const_iterator it = points.find(id);
	OV_ERR_FAIL_COND_V_MSG(
		it == points.end(), false, fmt::format("Can't get if point is disabled. Point with id: {} doesn't exist.", id)
	);

	return !it.value().enabled;
}

void PointMap::connect_points(points_key_type id, points_key_type with_id, bool bidirectional) {
	OV_ERR_FAIL_COND_MSG(id == with_id, fmt::format("Can't connect point with id: {} to itself.", id));

	points_iterator from_it = points.find(id);
	OV_ERR_FAIL_COND_MSG(from_it == points.end(), fmt::format("Can't connect points. Point with id: {} doesn't exist.", id));

	points_iterator to_it = points.find(with_id);
	OV_ERR_FAIL_COND_MSG(to_it == points.end(), fmt::format("Can't connect points. Point with id: {} doesn't exist.", with_id));

	from_it.value().neighbors.insert(to_it.key());

	if (bidirectional) {
		to_it.value().neighbors.insert(from_it.key());
	} else {
		to_it.value().unlinked_neighbors.insert(from_it.key());
	}

	Segment s { id, with_id };
	if (bidirectional) {
		s.direction = Segment::Direction::BIDIRECTIONAL;
	}

	segments_iterator element_it = segments.find(s);
	if (element_it != segments.end()) {
		s.direction |= element_it->direction;
		if (s.direction == Segment::Direction::BIDIRECTIONAL) {
			// Both are neighbors of each other now
			from_it.value().unlinked_neighbors.unordered_erase(to_it.key());
			to_it.value().unlinked_neighbors.unordered_erase(from_it.key());
		}
		segments.unordered_erase(element_it);
	}

	segments.insert(s);
}

void PointMap::disconnect_points(points_key_type id, points_key_type with_id, bool bidirectional) {
	points_iterator from_it = points.find(id);
	OV_ERR_FAIL_COND_MSG(from_it == points.end(), fmt::format("Can't disconnect points. Point with id: {} doesn't exist.", id));

	points_iterator to_it = points.find(with_id);
	OV_ERR_FAIL_COND_MSG(
		to_it == points.end(), fmt::format("Can't disconnect points. Point with id: {} doesn't exist.", with_id)
	);

	Segment s { id, with_id };
	Segment::Direction remove_direction = bidirectional ? Segment::Direction::BIDIRECTIONAL : s.direction;

	segments_iterator element_it = segments.find(s);
	if (element_it != segments.end()) {
		// s is the new segment
		// Erase the directions to be removed
		s.direction = (element_it->direction & ~remove_direction);

		from_it.value().neighbors.unordered_erase(to_it.key());
		if (bidirectional) {
			to_it.value().neighbors.unordered_erase(from_it.key());
			if (element_it->direction != Segment::Direction::BIDIRECTIONAL) {
				from_it.value().unlinked_neighbors.unordered_erase(to_it.key());
				to_it.value().unlinked_neighbors.unordered_erase(from_it.key());
			}
		} else {
			if (s.direction == Segment::Direction::NONE) {
				to_it.value().unlinked_neighbors.unordered_erase(from_it.key());
			} else {
				from_it.value().unlinked_neighbors.insert(to_it.key());
			}
		}

		segments.unordered_erase(element_it);
		if (s.direction != Segment::Direction::NONE) {
			segments.insert(s);
		}
	}
}

bool PointMap::are_points_connected(points_key_type id, points_key_type with_id, bool bidirectional) const {
	Segment s(id, with_id);
	segments_const_iterator element_it = segments.find(s);

	return element_it != segments.end() && (bidirectional || (element_it->direction & s.direction) == s.direction);
}

size_t PointMap::get_point_count() const {
	return points.size();
}

size_t PointMap::get_point_capacity() const {
	return points.capacity();
}

void PointMap::reserve_space(size_t num_nodes) {
	OV_ERR_FAIL_COND_MSG(num_nodes <= 0, fmt::format("New capacity must be greater than 0, new was: {}.", num_nodes));
	OV_ERR_FAIL_COND_MSG(
		num_nodes <= points.capacity(),
		fmt::format("New capacity must be greater than current capacity: {}, new was: {}.", points.capacity(), num_nodes)
	);
	points.reserve(num_nodes);
	points_pointers_invalidated();
}

void PointMap::shrink_to_fit() {
	points.shrink_to_fit();
	points_pointers_invalidated();
}

void PointMap::clear() {
	last_free_id = 0;
	segments.clear();
	points.clear();
	points_pointers_invalidated();
}

PointMap::points_key_type PointMap::get_closest_point(ivec2_t const& point, bool include_disabled) const {
	points_key_type closest_id = -1;
	int32_t closest_dist = std::numeric_limits<int32_t>::max();

	for (points_const_iterator it = points.begin(); it != points.end(); it++) {
		if (!include_disabled && !it.value().enabled) {
			continue; // Disabled points should not be considered.
		}

		// Keep the closest point's ID, and in case of multiple closest IDs,
		// the smallest one (makes it deterministic).
		int32_t d = point.distance_squared(it.value().position);
		points_key_type id = it.key();
		if (d <= closest_dist) {
			if (d == closest_dist && id > closest_id) { // Keep lowest ID.
				continue;
			}
			closest_dist = d;
			closest_id = id;
		}
	}

	return closest_id;
}

static fvec2_t get_closest_point_to_segment(fvec2_t const& p_point, std::span<ivec2_t, 2> p_segment) {
	fvec2_t p = p_point - fvec2_t { p_segment[0] };
	fvec2_t n = fvec2_t { p_segment[1] } - fvec2_t { p_segment[0] };
	int32_t l2 = n.length_squared();
	if (l2 == 0) {
		return fvec2_t { p_segment[0] }; // Both points are the same, just give any.
	}

	fixed_point_t d = n.dot(p) / l2;

	if (d <= 0) {
		return fvec2_t { p_segment[0] }; // Before first point.
	} else if (d >= 1) {
		return fvec2_t { p_segment[1] }; // After second point.
	} else {
		return fvec2_t { p_segment[0] } + n * d; // Inside.
	}
}

fvec2_t PointMap::get_closest_position_in_segment(fvec2_t const& point) const {
	fixed_point_t closest_dist = fixed_point_t::max();
	fvec2_t closest_point;

	for (Segment const& E : segments) {
		points_const_iterator from_it = points.find(E.key.first);
		points_const_iterator to_it = points.find(E.key.second);

		if (!(from_it.value().enabled && to_it.value().enabled)) {
			continue;
		}

		ivec2_t segment[2] = {
			from_it.value().position,
			to_it.value().position,
		};

		fvec2_t p = get_closest_point_to_segment(point, segment);
		fixed_point_t d = point.distance_squared(p);
		if (d < closest_dist) {
			closest_point = p;
			closest_dist = d;
		}
	}

	return closest_point;
}
