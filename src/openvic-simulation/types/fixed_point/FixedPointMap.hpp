#pragma once

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/OrderedContainersMath.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

	template<typename T>
	using fixed_point_map_t = ordered_map<T, fixed_point_t>;

	template<typename T>
	using fixed_point_map_value_t = typename fixed_point_map_t<T>::value_type;

	template<typename T>
	using fixed_point_map_const_iterator_t = typename fixed_point_map_t<T>::const_iterator;

	template<typename T, std::derived_from<T> U>
	constexpr fixed_point_map_t<T const*>& cast_map(fixed_point_map_t<U const*>& map) {
		return *reinterpret_cast<fixed_point_map_t<T const*>*>(&map);
	}

	template<typename T, std::derived_from<T> U>
	constexpr fixed_point_map_t<T const*> const& cast_map(fixed_point_map_t<U const*> const& map) {
		return *reinterpret_cast<fixed_point_map_t<T const*> const*>(&map);
	}

	template<typename T>
	constexpr fixed_point_map_t<T>& fixed_point_map_mul_add(
		fixed_point_map_t<T>& lhs, fixed_point_map_t<T> const& rhs, fixed_point_t factor
	) {
		if (factor != 0) {
			for (auto const& [key, value] : rhs) {
				lhs[key] += value * factor;
			}
		}

		return lhs;
	}

	template<typename T>
	constexpr fixed_point_map_t<T> operator+(fixed_point_map_t<T> const& lhs, fixed_point_map_t<T> const& rhs) {
		fixed_point_map_t<T> result = lhs;

		result += rhs;

		return result;
	}

	template<typename T>
	constexpr fixed_point_map_t<T> operator*(fixed_point_map_t<T> const& lhs, fixed_point_t rhs) {
		fixed_point_map_t<T> result = lhs;

		result *= rhs;

		return result;
	}

	template<typename T>
	constexpr fixed_point_map_t<T> operator*(fixed_point_t lhs, fixed_point_map_t<T> const& rhs) {
		return rhs * lhs;
	}

	template<typename T>
	constexpr fixed_point_map_t<T> operator/(fixed_point_map_t<T> const& lhs, fixed_point_t rhs) {
		fixed_point_map_t<T> result = lhs;

		result /= rhs;

		return result;
	}

	/* Assumes arguments are sorted with keys ascending. Result is determined by the
	 * first non-equal keys or values found starting from the back of the maps. */
	template<typename T>
	constexpr bool sorted_fixed_map_less_than(fixed_point_map_t<T> const& lhs, fixed_point_map_t<T> const& rhs) {

		typename fixed_point_map_t<T>::const_reverse_iterator lit, rit;

		for (lit = lhs.rcbegin(), rit = rhs.rcbegin(); lit != lhs.rcend() && rit != rhs.rcend(); lit++, rit++) {

			const std::strong_ordering key_cmp = lit->first <=> rit->first;

			if (key_cmp != std::strong_ordering::equal) {
				return key_cmp == std::strong_ordering::less;
			}

			const std::strong_ordering value_cmp = lit->second <=> rit->second;

			if (value_cmp != std::strong_ordering::equal) {
				return value_cmp == std::strong_ordering::less;
			}

		}

		return rit != rhs.rcend();
	}

	template<typename T>
	constexpr void normalise_fixed_point_map(fixed_point_map_t<T>& map) {
		const fixed_point_t total = get_total(map);

		if (total > 0) {
			map /= total;
		}
	}

	template<typename T>
	constexpr void rescale_fixed_point_map(fixed_point_map_t<T>& map, fixed_point_t new_total) {
		const fixed_point_t total = get_total(map);

		if (total > 0) {
			for (auto [key, value] : mutable_iterator(map)) {
				value *= new_total;
				value /= total;
			}
		}
	}
}
