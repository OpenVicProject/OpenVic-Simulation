#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include <tsl/ordered_map.h>

#include "openvic-simulation/core/memory/Tracker.hpp"
#include "openvic-simulation/core/portable/Deque.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/template/ContainerHash.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	// Useful for contiguous memory
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, tracker<RawAllocator>>>
	using vector_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, std::vector<std::pair<Key, T>, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, tracker<RawAllocator>>>
	using deque_ordered_map =
		tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, OpenVic::utility::deque<std::pair<Key, T>, Allocator>, IndexType>;

	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, tracker<RawAllocator>>>
	using ordered_map = vector_ordered_map<Key, T, Hash, KeyEqual, RawAllocator, IndexType, Allocator>;

	// Useful for contiguous memory
	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_vector_ordered_map = vector_ordered_map<
		Key, T, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_deque_ordered_map = deque_ordered_map<
		Key, T, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_ordered_map = case_insensitive_vector_ordered_map<Key, T, RawAllocator, IndexType, Allocator>;


	template<typename KeyType, add_assignable ValueType>
	constexpr static ValueType get_total(memory::ordered_map<KeyType, ValueType> const& map)
	requires std::is_default_constructible_v<ValueType>
	{
		ValueType running_total {};
		for (auto const& [key, value] : map) {
			running_total += value;
		}
		return running_total;
	}

	template<typename KeyType, typename ValueType>
	constexpr static typename memory::ordered_map<KeyType, ValueType>::const_iterator get_largest_item( //
		memory::ordered_map<KeyType, ValueType> const& map
	)
	requires std::three_way_comparable_with<ValueType, ValueType>
	{
		constexpr auto pred = [](typename memory::ordered_map<KeyType, ValueType>::value_type const& lhs,
								 typename memory::ordered_map<KeyType, ValueType>::value_type const& rhs) -> bool {
			return lhs.second < rhs.second;
		};

		return std::max_element(map.begin(), map.end(), pred);
	}

	/* This function includes a key comparator to choose between entries with equal values. */
	template<typename KeyType, typename ValueType>
	constexpr static typename memory::ordered_map<KeyType, ValueType>::const_iterator get_largest_item_tie_break( //
		memory::ordered_map<KeyType, ValueType> const& map, const auto key_pred
	)
	requires std::three_way_comparable_with<ValueType, ValueType>
	{
		constexpr auto pred = [key_pred](
								  typename memory::ordered_map<KeyType, ValueType>::value_type const& lhs,
								  typename memory::ordered_map<KeyType, ValueType>::value_type const& rhs
							  ) -> bool {
			return lhs.second < rhs.second || (lhs.second == rhs.second && key_pred(lhs.first, rhs.first));
		};

		return std::max_element(map.begin(), map.end(), pred);
	}

	template<typename KeyType, typename ValueType>
	constexpr static std::pair<
		typename memory::ordered_map<KeyType, ValueType>::const_iterator,
		typename memory::ordered_map<KeyType, ValueType>::const_iterator>
	get_largest_two_items( //
		memory::ordered_map<KeyType, ValueType> const& map
	)
	requires std::three_way_comparable_with<ValueType, ValueType>
	{
		typename memory::ordered_map<KeyType, ValueType>::const_iterator largest = map.end(), second_largest = map.end();

		for (typename memory::ordered_map<KeyType, ValueType>::const_iterator it = map.begin(); it != map.end(); ++it) {
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

namespace OpenVic {
	template<typename T>
	concept derived_ordered_map = derived_from_specialization_of<T, tsl::ordered_map>;

	template<typename KeyType, unary_negatable ValueType>
	constexpr memory::ordered_map<KeyType, ValueType> operator-(memory::ordered_map<KeyType, ValueType> const& rhs) {
		memory::ordered_map<KeyType, ValueType> result {};
		auto view = //
			rhs | ranges::views::transform([](KeyType const& key, ValueType const& value) {
				return std::make_pair(key, -value);
			});
		result.insert(view.begin(), view.end());
		return result;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator+=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, memory::ordered_map<KeyType, RhsValueType> const& rhs
	)
	requires add_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] += rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator-=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, memory::ordered_map<KeyType, RhsValueType> const& rhs
	)
	requires subtract_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] -= rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator*=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, memory::ordered_map<KeyType, RhsValueType> const& rhs
	)
	requires multiply_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] *= rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator/=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, memory::ordered_map<KeyType, RhsValueType> const& rhs
	)
	requires divide_assignable<LhsValueType, RhsValueType>
	{
		for (auto const& [key, rhs_value] : rhs) {
			lhs[key] = rhs_value;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator+=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, RhsValueType const& rhs
	)
	requires add_assignable<LhsValueType, RhsValueType>
	{
		for (typename decltype(lhs)::iterator it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() += rhs;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator-=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, RhsValueType const& rhs
	)
	requires subtract_assignable<LhsValueType, RhsValueType>
	{
		for (typename decltype(lhs)::iterator it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() -= rhs;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator*=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, RhsValueType const& rhs
	)
	requires multiply_assignable<LhsValueType, RhsValueType>
	{
		for (typename decltype(lhs)::iterator it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() *= rhs;
		}
		return lhs;
	}

	template<typename KeyType, typename LhsValueType, typename RhsValueType>
	constexpr memory::ordered_map<KeyType, LhsValueType>& operator/=( //
		memory::ordered_map<KeyType, LhsValueType>& lhs, RhsValueType const& rhs
	)
	requires divide_assignable<LhsValueType, RhsValueType>
	{
		for (auto it = lhs.begin(); it != lhs.end(); ++it) {
			it.value() /= rhs;
		}
		return lhs;
	}
}
