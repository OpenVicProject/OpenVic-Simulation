#pragma once

#include "openvic-simulation/core/container/OrderedSet.hpp"
#include "openvic-simulation/utility/MemoryTracker.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	// Useful for contiguous memory
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using vector_ordered_set = vector_ordered_set<Key, Hash, KeyEqual, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using deque_ordered_set = deque_ordered_set<Key, Hash, KeyEqual, Allocator, IndexType>;

	template<
		class Key, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using ordered_set = ordered_set<Key, Hash, KeyEqual, Allocator, IndexType>;

	// Useful for contiguous memory
	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_vector_ordered_set = case_insensitive_vector_ordered_set<Key, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_deque_ordered_set = case_insensitive_deque_ordered_set<Key, Allocator, IndexType>;

	template<
		class Key, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<Key, memory::tracker<RawAllocator>>>
	using case_insensitive_ordered_set = case_insensitive_ordered_set<Key, Allocator, IndexType>;
}
