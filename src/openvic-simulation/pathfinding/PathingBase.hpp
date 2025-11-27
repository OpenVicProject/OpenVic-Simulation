#pragma once

#include <functional>

#include <fmt/core.h>

#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/pathfinding/PointMap.hpp"
#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Containers.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic {
	template<typename ValueT, typename KeyT = PointMap::points_key_type>
	struct PathingNodeBase {
		using search_key_type = KeyT;
		using search_value_type = ValueT;
		using search_pair_type = std::pair<search_key_type, search_value_type>;
		using search_allocator_type = foonathan::memory::std_allocator<
			search_pair_type,
			memory::tracker<foonathan::memory::default_allocator>
		>;
		using search_container_type = std::vector<search_pair_type, search_allocator_type>;
		using search_map_type = tsl::ordered_map<
			search_key_type, search_value_type, //
			std::hash<search_key_type>, std::equal_to<search_key_type>, search_allocator_type, search_container_type
		>;
		using search_iterator = search_map_type::iterator;
		using search_const_iterator = search_map_type::const_iterator;

		constexpr PathingNodeBase() {};
		PathingNodeBase(PointMap::points_value_type const* p) {
			point = p;
		}

		PointMap::points_value_type const* point = nullptr;

		// Used for pathfinding.
		search_iterator prev_point;
	};

	template<typename ValueT, typename KeyT = PointMap::points_key_type>
	struct PathingBase : observer {
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

		void _on_point_invalidated(search_key_type id) {
			search_const_iterator it = search.find(id);
			if (it != search.end()) {
				search.unordered_erase(it);
			}
		}

		void _on_point_map_destroyed() {
			point_map = nullptr;
		}

	public:
		PathingBase(PointMap const* map) : point_map(map) {
			reserve_space(point_map->get_point_count());
			point_map->point_invalidated.connect(&PathingBase::_on_point_invalidated, this);
			point_map->points_pointers_invalidated.connect(&PathingBase::reset_search, this);
			point_map->destroyed.connect(&PathingBase::_on_point_map_destroyed, this);
		}

		PathingBase(PointMap const& map) : PathingBase(&map) {}

		virtual ~PathingBase() {
			this->disconnect_all();
		}

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

		inline search_iterator get_iterator_by_id(PointMap::points_key_type id) {
			search_iterator it = search.find(id);
			if (it == search.end()) {
				PointMap::points_value_type const* ptr = point_map->try_get_point(id);
				if (ptr != nullptr) {
					it = search.insert({ id, { ptr } }).first;
				}
			}
			OV_ERR_FAIL_COND_V_MSG(it == search.end(), it, memory::fmt::format("Can't get point by id: {} doesn't exist.", id));
			return it;
		}

		template<typename T, typename Projection = std::identity>
		inline memory::vector<T> generate_path_result(search_iterator from_it, search_iterator to_it, Projection projection = {}) {
			search_iterator p = to_it;
			int64_t pc = 1; // Begin point
			while (p != from_it) {
				pc++;
				p = p.value().prev_point;
			}

			memory::vector<T> path;
			path.resize(pc);

			{
				T* w = path.data();

				search_iterator p2 = to_it;
				int64_t idx = pc - 1;
				while (p2 != from_it) {
					w[idx--] = std::invoke(projection, p2);
					p2 = p2.value().prev_point;
				}

				w[0] = std::invoke(projection, p2); // Assign first
			}

			return path;
		}

		memory::vector<ivec2_t> get_point_path( //
			PointMap::points_key_type from_id, PointMap::points_key_type to_id, bool allow_partial_path = false
		) {
			OV_ERR_FAIL_COND_V(point_map == nullptr, memory::vector<ivec2_t>());

			search_iterator from_it = get_iterator_by_id(from_id);
			OV_ERR_FAIL_COND_V(from_it == search.end(), memory::vector<ivec2_t>());

			if (!_is_point_enabled(from_it)) {
				return memory::vector<ivec2_t>();
			}

			search_iterator to_it = get_iterator_by_id(to_id);
			OV_ERR_FAIL_COND_V(to_it == search.end(), memory::vector<ivec2_t>());

			if (from_it == to_it) {
				return memory::vector<ivec2_t> { 1, from_it.value().point->position };
			}

			bool found_route = _solve(from_it, to_it, current_pass++, allow_partial_path);
			if (!found_route) {
				if (!allow_partial_path || last_closest_point == search.end()) {
					return memory::vector<ivec2_t>();
				}

				// Use closest point instead.
				to_it = last_closest_point;
			}

			return generate_path_result<ivec2_t>(from_it, to_it, [](search_iterator it) -> ivec2_t {
				return it.value().point->position;
			});
		}

		memory::vector<PointMap::points_key_type> get_id_path( //
			PointMap::points_key_type from_id, PointMap::points_key_type to_id, bool allow_partial_path = false
		) {
			OV_ERR_FAIL_COND_V(point_map == nullptr, memory::vector<PointMap::points_key_type>());

			search_iterator from_it = get_iterator_by_id(from_id);
			OV_ERR_FAIL_COND_V(from_it == search.end(), memory::vector<PointMap::points_key_type>());

			if (!_is_point_enabled(from_it)) {
				return memory::vector<PointMap::points_key_type>();
			}

			search_iterator to_it = get_iterator_by_id(to_id);
			OV_ERR_FAIL_COND_V(to_it == search.end(), memory::vector<PointMap::points_key_type>());

			if (from_it == to_it) {
				return memory::vector<PointMap::points_key_type> { 1, from_id };
			}

			bool found_route = _solve(from_it, to_it, current_pass++, allow_partial_path);
			if (!found_route) {
				if (!allow_partial_path || last_closest_point == search.end()) {
					return memory::vector<PointMap::points_key_type>();
				}

				// Use closest point instead.
				to_it = last_closest_point;
			}

			return generate_path_result<PointMap::points_key_type>(from_it, to_it, &search_iterator::key);
		}
	};
}
