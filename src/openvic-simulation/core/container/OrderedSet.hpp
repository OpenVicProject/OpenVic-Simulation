#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <tsl/ordered_set.h>

#include "openvic-simulation/core/template/StringMapCasing.hpp"

namespace OpenVic {
	// Useful for contiguous memory
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<Key>,
		class IndexType = std::uint_least32_t>
	using vector_ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, std::vector<Key, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<Key>,
		class IndexType = std::uint_least32_t>
	using deque_ordered_set =
		tsl::ordered_set<Key, Hash, KeyEqual, Allocator, OpenVic::utility::deque<Key, Allocator>, IndexType>;

	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>, class Allocator = std::allocator<Key>,
		class IndexType = std::uint_least32_t>
	using ordered_set = vector_ordered_set<Key, Hash, KeyEqual, Allocator, IndexType>;

	// Useful for contiguous memory
	template<class Key, class Allocator = std::allocator<Key>, class IndexType = std::uint_least32_t>
	using case_insensitive_vector_ordered_set =
		vector_ordered_set<Key, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<class Key, class Allocator = std::allocator<Key>, class IndexType = std::uint_least32_t>
	using case_insensitive_deque_ordered_set =
		deque_ordered_set<Key, case_insensitive_string_hash, case_insensitive_string_equal, Allocator, IndexType>;

	template<class Key, class Allocator = std::allocator<Key>, class IndexType = std::uint_least32_t>
	using case_insensitive_ordered_set = case_insensitive_vector_ordered_set<Key, Allocator, IndexType>;
}
