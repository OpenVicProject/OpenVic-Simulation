#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>

#include "openvic-simulation/core/template/EnumBitfield.hpp"
#include "openvic-simulation/types/Signal.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/core/Hash.hpp"

namespace OpenVic {
	struct PointMap {
		struct Point;

		using points_key_type = uint64_t;
		using points_value_type = Point;
		using points_pair_type = std::pair<points_key_type, points_value_type>;
		using points_allocator_type = foonathan::memory::std_allocator<
			points_pair_type,
			memory::tracker<foonathan::memory::default_allocator>
		>;
		using points_container_type = std::vector<points_pair_type, points_allocator_type>;
		using points_map_type = tsl::ordered_map<
			points_key_type, points_value_type, //
			std::hash<points_key_type>, std::equal_to<points_key_type>, points_allocator_type, points_container_type
		>;
		using points_iterator = points_map_type::iterator;
		using points_const_iterator = points_map_type::const_iterator;

		struct Segment {
			std::pair<points_key_type, points_key_type> key;

			enum class Direction : uint8_t {
				NONE = 0, //
				FORWARD = 1,
				BACKWARD = 2,
				BIDIRECTIONAL = FORWARD | BACKWARD
			};

			Direction direction = Direction::NONE;

			inline constexpr bool operator==(Segment const& s) const {
				return key == s.key;
			}

			inline constexpr Segment() {};
			inline constexpr Segment(points_key_type from, points_key_type to) {
				if (from < to) {
					key.first = from;
					key.second = to;
					direction = Direction::FORWARD;
				} else {
					key.first = to;
					key.second = from;
					direction = Direction::BACKWARD;
				}
			};
		};

		struct SegmentHash {
			inline constexpr std::size_t operator()(Segment const& segment) const {
				return hash_murmur3(hash_murmur3(segment.key.first) << 32)
					| hash_murmur3(segment.key.second);
			}
		};

		using segments_type = Segment;
		using segments_hash_type = SegmentHash;
		using segments_allocator_type = foonathan::memory::std_allocator<
			segments_type,
			memory::tracker<foonathan::memory::default_allocator>
		>;
		using segments_container_type = std::vector<segments_type, segments_allocator_type>;
		using segments_set_type = tsl::ordered_set<
			segments_type, segments_hash_type, std::equal_to<segments_type>, segments_allocator_type, segments_container_type>;
		using segments_iterator = segments_set_type::iterator;
		using segments_const_iterator = segments_set_type::const_iterator;

		struct Point {
			using neighbor_point_id_type = points_key_type;
			using neighbors_allocator_type = foonathan::memory::std_allocator<
				neighbor_point_id_type,
				memory::tracker<foonathan::memory::default_allocator>
			>;
			using neighbors_container_type = std::vector<neighbor_point_id_type, neighbors_allocator_type>;
			using neighbors_type = tsl::ordered_set<
				neighbor_point_id_type, //
				std::hash<neighbor_point_id_type>, std::equal_to<neighbor_point_id_type>, neighbors_allocator_type,
				neighbors_container_type
			>;
			using neighbors_iterator = neighbors_type::iterator;
			using neighbors_const_iterator = neighbors_type::const_iterator;

			neighbors_type neighbors;
			neighbors_type unlinked_neighbors;

			ivec2_t position;
			fixed_point_t weight_scale = 0;

			bool enabled = false;
		};

		struct ProxyPoints {
			using const_iterator = points_const_iterator;

			inline points_value_type const& operator[](points_key_type key) {
				return ((PointMap*)(this))->points.find(key).value();
			}

			inline const_iterator begin() const {
				return ((PointMap*)(this))->points.cbegin();
			}

			inline const_iterator end() const {
				return ((PointMap*)(this))->points.cend();
			}
		};

		struct ProxySegments {
			using const_iterator = segments_const_iterator;

			inline const_iterator begin() const {
				return ((PointMap*)(this))->segments.cbegin();
			}

			inline const_iterator end() const {
				return ((PointMap*)(this))->segments.cend();
			}
		};

	private:
		ProxyPoints points_proxy;
		ProxySegments segments_proxy;

		points_map_type points;
		segments_set_type segments;

		mutable points_key_type last_free_id = 0;

	public:
		mutable signal_property<PointMap, points_key_type> point_invalidated;
		mutable signal_property<PointMap> points_pointers_invalidated;
		mutable signal_property<PointMap> destroyed;

		~PointMap();

		int64_t get_available_points() const;

		bool try_add_point(
			points_key_type id, ivec2_t const& position, fixed_point_t weight_scale = 1,
			std::span<points_key_type> adjacent_points = {}
		);

		void add_point(
			points_key_type id, ivec2_t const& position, fixed_point_t weight_scale = 1,
			std::span<points_key_type> adjacent_points = {}
		);

		Point const* get_point(points_key_type id) const;
		Point const* try_get_point(points_key_type id) const;

		ivec2_t get_point_position(points_key_type id) const;
		void set_point_position(points_key_type id, ivec2_t const& position);
		fixed_point_t get_point_weight_scale(points_key_type id) const;
		void set_point_weight_scale(points_key_type id, fixed_point_t weight_scale);

		void remove_point(points_key_type id);
		bool has_point(points_key_type id) const;

		std::span<const points_key_type> get_point_connections(points_key_type id) const;
		memory::vector<points_key_type> get_point_ids();

		void set_point_disabled(points_key_type id, bool disabled = true);
		bool is_point_disabled(points_key_type id) const;

		void connect_points(points_key_type id, points_key_type with_id, bool bidirectional = true);
		void disconnect_points(points_key_type id, points_key_type with_id, bool bidirectional = true);
		bool are_points_connected(points_key_type id, points_key_type with_id, bool bidirectional = true) const;

		size_t get_point_count() const;
		size_t get_point_capacity() const;
		void reserve_space(size_t num_nodes);
		void shrink_to_fit();
		void clear();

		points_key_type get_closest_point(ivec2_t const& point, bool include_disabled = false) const;
		fvec2_t get_closest_position_in_segment(fvec2_t const& point) const;

		inline constexpr ProxyPoints const& points_map() const {
			return points_proxy;
		}

		inline constexpr ProxySegments const& segments_set() const {
			return segments_proxy;
		}
	};

	template<>
	struct enable_bitfield<PointMap::Segment::Direction> : std::true_type {};
}
