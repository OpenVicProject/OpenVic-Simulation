#pragma once

#include <type_traits>

#include <foonathan/memory/default_allocator.hpp>

#include "openvic-simulation/core/memory/allocators/AllocatorReference.hpp"
#include "openvic-simulation/core/memory/make_tracked_allocator.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline T* allocate_new(RawAllocator&& alloc, Args&&... args) {
		allocator_reference<RawAllocator> alloc_ref = foonathan::memory::make_allocator_reference(alloc);
		T* ptr = static_cast<T*>(alloc_ref.allocate_node(sizeof(T), alignof(T)));
		::new (ptr) T(std::forward<Args>(args)...);
		return ptr;
	}

	template<typename T, class RawAllocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline T* allocate_new(RawAllocator&& alloc, size_t n) {
		allocator_reference<RawAllocator> alloc_ref = foonathan::memory::make_allocator_reference(alloc);
		return static_cast<T*>(alloc_ref.allocate_node(sizeof(T) * n, alignof(T)));
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline T* make_new(Args&&... args) {
		return allocate_new<T>(OpenVic::memory::make_tracked_allocator(RawAllocator {}), std::forward<Args>(args)...);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline T* make_new(size_t n) {
		return allocate_new<T>(OpenVic::memory::make_tracked_allocator(RawAllocator {}), n);
	}
}