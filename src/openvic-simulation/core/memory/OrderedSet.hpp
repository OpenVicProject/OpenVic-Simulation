#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <tsl/ordered_set.h>

#include "openvic-simulation/core/Hash.hpp"
#include "openvic-simulation/core/memory/Tracker.hpp"
#include "openvic-simulation/core/portable/Deque.hpp"
#include "openvic-simulation/core/template/ContainerHash.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	// Useful for contiguous memory
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using vector_ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, std::vector<Key, Allocator>, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using deque_ordered_set =
		tsl::ordered_set<Key, Hash, KeyEqual, Allocator, OpenVic::utility::deque<Key, Allocator>, IndexType>;

	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using ordered_set = vector_ordered_set<Key, Hash, KeyEqual, RawAllocator, IndexType, Allocator>;

	// Useful for contiguous memory
	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_vector_ordered_set = vector_ordered_set<
		Key, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_deque_ordered_set =
		deque_ordered_set<Key, case_insensitive_string_hash, case_insensitive_string_equal, RawAllocator, IndexType, Allocator>;

	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_ordered_set = case_insensitive_vector_ordered_set<Key, RawAllocator, IndexType, Allocator>;
}
