#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <tsl/ordered_map.h>

#include "openvic-simulation/core/template/StringMapCasing.hpp"

namespace OpenVic {
	// Useful for contiguous memory
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using vector_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, std::vector<std::pair<Key, T>, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using deque_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, OpenVic::utility::deque<std::pair<Key, T>, Allocator>, IndexType>;

	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using ordered_map = vector_ordered_map<Key, T, Hash, KeyEqual, Allocator, IndexType>;

	// Useful for contiguous memory
	template<class Key, class T, class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using case_insensitive_vector_ordered_map =
		vector_ordered_map<Key, T, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<class Key, class T, class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using case_insensitive_deque_ordered_map =
		deque_ordered_map<Key, T, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	template<class Key, class T, class Allocator = std::allocator<std::pair<Key, T>>, class IndexType = std::uint_least32_t>
	using case_insensitive_ordered_map = case_insensitive_vector_ordered_map<Key, T, Allocator, IndexType>;

	template<typename KeyType, add_assignable ValueType, class H, class KE, class A, class IT>
	constexpr static ValueType get_total(ordered_map<KeyType, ValueType, H, KE, A, IT> const& map)
	requires std::is_default_constructible_v<ValueType>
	{
		ValueType running_total {};
		for (auto const& [key, value] : map) {
			running_total += value;
		}
		return running_total;
	}

	template<typename KeyType, typename ValueType, class H, class KE, class A, class IT>
	constexpr static typename ordered_map<KeyType, ValueType, H, KE, A, IT>::const_iterator get_largest_item( //
		ordered_map<KeyType, ValueType, H, KE, A, IT> const& map
	)
	requires std::three_way_comparable_with<ValueType, ValueType>
	{
		constexpr auto pred = [](typename ordered_map<KeyType, ValueType, H, KE, A, IT>::value_type const& lhs,
								 typename ordered_map<KeyType, ValueType, H, KE, A, IT>::value_type const& rhs) -> bool {
			return lhs.second < rhs.second;
		};

		return std::max_element(map.begin(), map.end(), pred);
	}

	/* This function includes a key comparator to choose between entries with equal values. */
	template<typename KeyType, typename ValueType, class H, class KE, class A, class IT>
	constexpr static typename ordered_map<KeyType, ValueType, H, KE, A, IT>::const_iterator get_largest_item_tie_break( //
		ordered_map<KeyType, ValueType, H, KE, A, IT> const& map, const auto key_pred
	)
	requires std::three_way_comparable_with<ValueType, ValueType>
	{
		constexpr auto pred = [key_pred](
								  typename ordered_map<KeyType, ValueType, H, KE, A, IT>::value_type const& lhs,
								  typename ordered_map<KeyType, ValueType, H, KE, A, IT>::value_type const& rhs
							  ) -> bool {
			return lhs.second < rhs.second || (lhs.second == rhs.second && key_pred(lhs.first, rhs.first));
		};

		return std::max_element(map.begin(), map.end(), pred);
	}

	template<typename KeyType, typename ValueType, class H, class KE, class A, class IT>
	constexpr static std::pair<
		typename ordered_map<KeyType, ValueType, H, KE, A, IT>::const_iterator,
		typename ordered_map<KeyType, ValueType, H, KE, A, IT>::const_iterator>
	get_largest_two_items( //
		ordered_map<KeyType, ValueType, H, KE, A, IT> const& map
	)
	requires std::three_way_comparable_with<ValueType, ValueType>
	{
		typename ordered_map<KeyType, ValueType, H, KE, A, IT>::const_iterator largest = map.end(), second_largest = map.end();

		for (typename ordered_map<KeyType, ValueType, H, KE, A, IT>::const_iterator it = map.begin(); it != map.end(); ++it) {
			if (largest == map.end() || it->second > largest->second) {
				second_largest = largest;
				largest = it;
			} else if (second_largest == map.end() || it->second > second_largest->second) {
				second_largest = it;
			}
		}

		return std::make_pair(std::move(largest), std::move(second_largest));
	}

	template<typename T>
	concept derived_ordered_map = derived_from_specialization_of<T, tsl::ordered_map>;

	template<typename KeyType, unary_negatable ValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, ValueType, H, KE, A, IT>
	operator-(ordered_map<KeyType, ValueType, H, KE, A, IT> const& rhs) {
		ordered_map<KeyType, ValueType, H, KE, A, IT> result {};
		auto view = //
			rhs | ranges::views::transform([](KeyType const& key, ValueType const& value) {
				return std::make_pair(key, -value);
			});
		result.insert(view.begin(), view.end());
		return result;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator+=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, ordered_map<KeyType, RhsValueType, H, KE, A, IT> const& rhs
	)
	requires add_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] += rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator-=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, ordered_map<KeyType, RhsValueType, H, KE, A, IT> const& rhs
	)
	requires subtract_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] -= rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator*=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, ordered_map<KeyType, RhsValueType, H, KE, A, IT> const& rhs
	)
	requires multiply_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] *= rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator/=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, ordered_map<KeyType, RhsValueType, H, KE, A, IT> const& rhs
	)
	requires divide_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] = rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator+=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, RhsValueType const& rhs
	)
	requires add_assignable<LhsValueType, RhsValueType>
	{
		for (typename decltype(lhs)::iterator it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() += rhs;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator-=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, RhsValueType const& rhs
	)
	requires subtract_assignable<LhsValueType, RhsValueType>
	{
		for (typename decltype(lhs)::iterator it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() -= rhs;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator*=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, RhsValueType const& rhs
	)
	requires multiply_assignable<LhsValueType, RhsValueType>
	{
		for (typename decltype(lhs)::iterator it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() *= rhs;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType, class H, class KE, class A, class IT>
	constexpr ordered_map<KeyType, LhsValueType, H, KE, A, IT>& operator/=( //
		ordered_map<KeyType, LhsValueType, H, KE, A, IT>& lhs, RhsValueType const& rhs
	)
	requires divide_assignable<LhsValueType, RhsValueType>
	{
		for (auto it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() /= rhs;
		}
		return lhs;
	}
}
