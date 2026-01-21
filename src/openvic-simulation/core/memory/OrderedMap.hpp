#pragma once

#include "openvic-simulation/core/container/OrderedMap.hpp"
#include "openvic-simulation/utility/MemoryTracker.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	// Useful for contiguous memory
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, tracker<RawAllocator>>>
	using vector_ordered_map = vector_ordered_map<Key, T, Hash, KeyEqual, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, tracker<RawAllocator>>>
	using deque_ordered_map = deque_ordered_map<Key, T, Hash, KeyEqual, Allocator, IndexType>;

	template<
		class Key, class T, class Hash = container_hash<Key>, class KeyEqual = std::equal_to<>,
		class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, tracker<RawAllocator>>>
	using ordered_map = ordered_map<Key, T, Hash, KeyEqual, Allocator, IndexType>;

	// Useful for contiguous memory
	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_vector_ordered_map = case_insensitive_vector_ordered_map<Key, T, Allocator, IndexType>;

	// Useful for stable memory addresses (so long as you don't remove or insert values)
	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_deque_ordered_map = case_insensitive_deque_ordered_map<Key, T, Allocator, IndexType>;

	template<
		class Key, class T, class RawAllocator = foonathan::memory::default_allocator, class IndexType = std::uint_least32_t,
		class Allocator = foonathan::memory::std_allocator<std::pair<Key, T>, memory::tracker<RawAllocator>>>
	using case_insensitive_ordered_map = case_insensitive_ordered_map<Key, T, Allocator, IndexType>;
}
