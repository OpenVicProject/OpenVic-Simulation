#pragma once

#include <ranges>
#include <type_traits>

#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/utility/MathConcepts.hpp"

namespace OpenVic {
	template <typename KeyType, typename ValueType>
	constexpr ordered_map<KeyType, ValueType> operator-(ordered_map<KeyType, ValueType> const& rhs)
	requires UnaryNegatable<ValueType> {
		ordered_map<KeyType, ValueType> result {};
		auto view = rhs | std::views::transform(
			[](KeyType const& key, ValueType const& value) {
				return std::make_pair(key, -value);
			}
		);
		result.insert(view.begin(), view.end());
		return result;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator+=(
		ordered_map<KeyType, LhsValueType>& lhs,
		ordered_map<KeyType, RhsValueType> const& rhs
	) requires AddAssignable<LhsValueType,RhsValueType> {
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] += rhs_value;
		}
		return lhs;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator-=(
		ordered_map<KeyType, LhsValueType>& lhs,
		ordered_map<KeyType, RhsValueType> const& rhs
	) requires SubtractAssignable<LhsValueType,RhsValueType> {
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] -= rhs_value;
		}
		return lhs;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator*=(
		ordered_map<KeyType, LhsValueType>& lhs,
		ordered_map<KeyType, RhsValueType> const& rhs
	) requires MultiplyAssignable<LhsValueType,RhsValueType> {
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] *= rhs_value;
		}
		return lhs;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator/=(
		ordered_map<KeyType, LhsValueType>& lhs,
		ordered_map<KeyType, RhsValueType> const& rhs
	) requires DivideAssignable<LhsValueType,RhsValueType> {
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] = rhs_value;
		}
		return lhs;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator+=(
		ordered_map<KeyType, LhsValueType>& lhs,
		RhsValueType const& rhs
	) requires AddAssignable<LhsValueType,RhsValueType> {
		for (auto& [key, lhs_value] : lhs) {
			lhs_value += rhs;
		}
		return lhs;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator-=(
		ordered_map<KeyType, LhsValueType>& lhs,
		RhsValueType const& rhs
	) requires SubtractAssignable<LhsValueType,RhsValueType> {
		for (auto& [key, lhs_value] : lhs) {
			lhs_value -= rhs;
		}
		return lhs;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator*=(
		ordered_map<KeyType, LhsValueType>& lhs,
		RhsValueType const& rhs
	) requires MultiplyAssignable<LhsValueType,RhsValueType> {
		for (auto& [key, lhs_value] : lhs) {
			lhs_value *= rhs;
		}
		return lhs;
	}

	template <typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr ordered_map<KeyType, LhsValueType>& operator/=(
		ordered_map<KeyType, LhsValueType>& lhs,
		RhsValueType const& rhs
	) requires DivideAssignable<LhsValueType,RhsValueType> {
		for (auto& [key, lhs_value] : lhs) {
			lhs_value /= rhs;
		}
		return lhs;
	}

	template <typename KeyType, typename ValueType>
	constexpr static ValueType get_total(
		ordered_map<KeyType, ValueType> const& map
	) requires AddAssignable<ValueType> && std::is_default_constructible_v<ValueType> {
		ValueType running_total {};
		for (auto& [key, value] : map) {
			running_total += value;
		}
		return running_total;
	}

	template <typename KeyType, typename ValueType>
	constexpr static typename ordered_map<KeyType, ValueType>::const_iterator get_largest_item(
		ordered_map<KeyType, ValueType> const& map
	) requires std::three_way_comparable_with<ValueType,ValueType> {
		constexpr auto pred = [](
			typename ordered_map<KeyType, ValueType>::value_type const& lhs,
			typename ordered_map<KeyType, ValueType>::value_type const& rhs
		) -> bool {
			return lhs.second < rhs.second;
		};

		return std::max_element(map.begin(), map.end(), pred);
	}

	/* This function includes a key comparator to choose between entries with equal values. */
	template <typename KeyType, typename ValueType>
	constexpr static typename ordered_map<KeyType, ValueType>::const_iterator get_largest_item_tie_break(
		ordered_map<KeyType, ValueType> const& map, const auto key_pred
	) requires std::three_way_comparable_with<ValueType,ValueType> {
		constexpr auto pred = [key_pred](
			typename ordered_map<KeyType, ValueType>::value_type const& lhs,
			typename ordered_map<KeyType, ValueType>::value_type const& rhs
		) -> bool {
			return lhs.second < rhs.second || (lhs.second == rhs.second && key_pred(lhs.first, rhs.first));
		};

		return std::max_element(map.begin(), map.end(), pred);
	}

	template <typename KeyType, typename ValueType>
	constexpr static std::pair<
		typename ordered_map<KeyType, ValueType>::const_iterator,
		typename ordered_map<KeyType, ValueType>::const_iterator
	> get_largest_two_items(
		ordered_map<KeyType, ValueType> const& map
	) requires std::three_way_comparable_with<ValueType,ValueType> {
		typename ordered_map<KeyType, ValueType>::const_iterator largest = map.end(), second_largest = map.end();

		for (typename ordered_map<KeyType, ValueType>::const_iterator it = map.begin(); it != map.end(); ++it) {
			if (largest == map.end() || it->second > largest->second) {
				second_largest = largest;
				largest = it;
			} else if (second_largest == map.end() || it->second > second_largest->second) {
				second_largest = it;
			}
		}

		return std::make_pair(
			std::move(largest),
			std::move(second_largest)
		);
	}
}