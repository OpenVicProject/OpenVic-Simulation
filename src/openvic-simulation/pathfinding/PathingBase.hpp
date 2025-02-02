#pragma once

#include <fmt/core.h>

#include "openvic-simulation/pathfinding/PointMap.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"

namespace OpenVic {
	template<typename ValueT, typename KeyT = PointMap::points_key_type>
	struct PathingNodeBase {
		using search_key_type = KeyT;
		using search_value_type = ValueT;
		using search_pair_type = std::pair<search_key_type, search_value_type>;
		using search_allocator_type = std::allocator<search_pair_type>;
		using search_container_type = std::vector<search_pair_type, search_allocator_type>;
		using search_map_type = tsl::ordered_map<
			search_key_type, search_value_type, //
			std::hash<search_key_type>, std::equal_to<search_key_type>, search_allocator_type, search_container_type>;
		using search_iterator = search_map_type::iterator;
		using search_const_iterator = search_map_type::const_iterator;

		PathingNodeBase() = default;
		PathingNodeBase(PointMap::points_value_type const* p) {
			point = p;
		}

		PointMap::points_value_type const* point = nullptr;

		// Used for pathfinding.
		search_iterator prev_point;
	};

	template<typename ValueT, typename KeyT = PointMap::points_key_type>
	struct PathingBase {
		using search_node_type = PathingNodeBase<ValueT, KeyT>;
		using search_key_type = search_node_type::search_key_type;
		using search_value_type = search_node_type::search_value_type;
		using search_pair_type = search_node_type::search_pair_type;
		using search_allocator_type = search_node_type::search_allocator_type;
		using search_container_type = search_node_type::search_container_type;
		using search_map_type = search_node_type::search_map_type;
		using search_iterator = search_node_type::search_iterator;
		using search_const_iterator = search_node_type::search_const_iterator;

	protected:
		search_map_type search;

		PointMap const* point_map;

		uint64_t current_pass = 1;
		search_iterator last_closest_point;

		virtual bool _is_point_enabled(search_const_iterator it) const {
			return it.value().point->enabled;
		}

		virtual fixed_point_t _estimate_cost(search_const_iterator from_it, search_const_iterator end_it) {
			return fixed_point_t { from_it.value().point->position.distance_squared(end_it.value().point->position) }.sqrt();
		}

		virtual fixed_point_t _compute_cost(search_const_iterator from_it, search_const_iterator end_it) {
			return fixed_point_t { from_it.value().point->position.distance_squared(end_it.value().point->position) }.sqrt();
		}

		virtual bool _solve( //
			search_iterator begin_point, search_iterator end_point, uint64_t pass, bool allow_partial_path
		) = 0;

	public:
		PathingBase(PointMap const* map) : point_map(map) {
			reserve_space(point_map->get_point_count());
		}

		PathingBase(PointMap const& map) : PathingBase(&map) {}

		PointMap const& get_point_map() const {
			return *point_map;
		}

		size_t get_search_count() const {
			return search.size();
		}

		size_t get_search_capacity() const {
			return search.size();
		}

		void reset_search() {
			clear();
			reserve_space(point_map->get_point_count());
		}

		void reserve_space(size_t num_nodes) {
			search.reserve(num_nodes);
		}

		void clear() {
			search.clear();
			last_closest_point = search.end();
		}

		std::vector<ivec2_t> get_point_path( //
			PointMap::points_key_type from_id, PointMap::points_key_type to_id, bool allow_partial_path = false
		) {
			search_iterator from_it = search.find(from_id);
			if (from_it == search.end()) {
				PointMap::points_value_type const* from_ptr = point_map->try_get_point(from_id);
				if (from_ptr != nullptr) {
					from_it = search.insert({ from_id, { .point = from_ptr } }).first;
				}
			}
			OV_ERR_FAIL_COND_V_MSG(
				from_it == search.end(), std::vector<ivec2_t>(),
				fmt::format("Can't get point path. Point with id: {} doesn't exist.", from_id)
			);

			search_iterator to_it = search.find(to_id);
			if (to_it == search.end()) {
				PointMap::points_value_type const* to_ptr = point_map->try_get_point(to_id);
				if (to_ptr != nullptr) {
					to_it = search.insert({ to_id, { .point = to_ptr } }).first;
				}
			}
			OV_ERR_FAIL_COND_V_MSG(
				to_it == search.end(), std::vector<ivec2_t>(),
				fmt::format("Can't get point path. Point with id: {} doesn't exist.", to_id)
			);

			if (from_it == to_it) {
				return std::vector<ivec2_t> { 1, from_it.value().point->position };
			}

			bool found_route = _solve(from_it, to_it, current_pass++, allow_partial_path);
			if (!found_route) {
				if (!allow_partial_path || last_closest_point == search.end()) {
					return std::vector<ivec2_t>();
				}

				// Use closest point instead.
				to_it = last_closest_point;
			}

			search_iterator p = to_it;
			int64_t pc = 1; // Begin point
			while (p != from_it) {
				pc++;
				p = p.value().prev_point;
			}

			std::vector<ivec2_t> path;
			path.resize(pc);

			{
				ivec2_t* w = path.data();

				search_iterator p2 = to_it;
				int64_t idx = pc - 1;
				while (p2 != from_it) {
					w[idx--] = p2.value().point->position;
					p2 = p2.value().prev_point;
				}

				w[0] = p2.value().point->position; // Assign first
			}

			return path;
		}

		std::vector<PointMap::points_key_type> get_id_path( //
			PointMap::points_key_type from_id, PointMap::points_key_type to_id, bool allow_partial_path = false
		) {
			search_iterator from_it = search.find(from_id);
			if (from_it == search.end()) {
				PointMap::points_value_type const* from_ptr = point_map->try_get_point(from_id);
				if (from_ptr != nullptr) {
					from_it = search.insert({ from_id, { from_ptr } }).first;
				}
			}
			OV_ERR_FAIL_COND_V_MSG(
				from_it == search.end(), std::vector<PointMap::points_key_type>(),
				fmt::format("Can't get id path. Point with id: {} doesn't exist.", from_id)
			);

			search_iterator to_it = search.find(to_id);
			if (to_it == search.end()) {
				PointMap::points_value_type const* to_ptr = point_map->try_get_point(to_id);
				if (to_ptr != nullptr) {
					to_it = search.insert({ to_id, { to_ptr } }).first;
				}
			}
			OV_ERR_FAIL_COND_V_MSG(
				to_it == search.end(), std::vector<PointMap::points_key_type>(),
				fmt::format("Can't get id path. Point with id: {} doesn't exist.", to_id)
			);

			if (from_it == to_it) {
				return std::vector<PointMap::points_key_type> { 1, from_id };
			}

			bool found_route = _solve(from_it, to_it, current_pass++, allow_partial_path);
			if (!found_route) {
				if (!allow_partial_path || last_closest_point == search.end()) {
					return std::vector<PointMap::points_key_type>();
				}

				// Use closest point instead.
				to_it = last_closest_point;
			}

			search_iterator p = to_it;
			size_t pc = 1; // Begin point
			while (p != from_it) {
				pc++;
				p = p.value().prev_point;
			}

			std::vector<PointMap::points_key_type> path;
			path.resize(pc);

			{
				PointMap::points_key_type* w = path.data();

				p = to_it;
				size_t idx = pc - 1;
				while (p != from_it) {
					w[idx--] = p.key();
					p = p.value().prev_point;
				}

				w[0] = p.key(); // Assign first
			}

			return path;
		}
	};
}
