#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

	template<typename T>
	using fixed_point_map_t = std::map<T, fixed_point_t>;

	template<typename T>
	using fixed_point_map_value_t = typename fixed_point_map_t<T>::value_type;

	template<typename T>
	using fixed_point_map_const_iterator_t = typename fixed_point_map_t<T>::const_iterator;

	template<typename T>
	constexpr fixed_point_t get_total(fixed_point_map_t<T> const& map) {
		fixed_point_t total = 0;
		for (auto const& [key, value] : map) {
			total += value;
		}
		return total;
	}

	template<typename T>
	constexpr fixed_point_map_const_iterator_t<T> get_largest_item(fixed_point_map_t<T> const& map) {
		constexpr auto pred = [](fixed_point_map_value_t<T> a, fixed_point_map_value_t<T> b) -> bool {
			return a.second < b.second;
		};
		return std::max_element(map.begin(), map.end(), pred);
	}

	template<typename T>
	constexpr std::pair<fixed_point_map_const_iterator_t<T>, fixed_point_map_const_iterator_t<T>> get_largest_two_items(
		fixed_point_map_t<T> const& map
	) {
		fixed_point_map_const_iterator_t<T> largest = map.end(), second_largest = map.end();
		for (fixed_point_map_const_iterator_t<T> it = map.begin(); it != map.end(); ++it) {
			if (largest == map.end() || it->second > largest->second) {
				second_largest = largest;
				largest = it;
			} else if (second_largest == map.end() || it->second > second_largest->second) {
				second_largest = it;
			}
		}
		return std::make_pair(std::move(largest), std::move(second_largest));
	}
}
