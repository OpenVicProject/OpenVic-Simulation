#pragma once

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/deleter.hpp>

#include "openvic-simulation/core/memory/allocators/AllocatorReference.hpp"
#include "openvic-simulation/core/memory/make_tracked_allocator.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator>
	using allocator_deleter = foonathan::memory::allocator_deleter<T, RawAllocator>;
	
	template<typename T, class RawAllocator>
	requires(!std::is_array_v<T>)
	static inline void dealloc_delete(RawAllocator&& alloc, T* ptr) {
		ptr->~T();
		allocator_reference<RawAllocator> alloc_ref = foonathan::memory::make_allocator_reference(alloc);
		alloc_ref.deallocate(ptr, sizeof(T), alignof(T));
	}

	template<typename T, class RawAllocator>
	requires(std::is_unbounded_array_v<T>)
	static inline void dealloc_delete(RawAllocator&& alloc, T* ptr, size_t n) {
		std::destroy_n(ptr, n);
		allocator_reference<RawAllocator> alloc_ref = foonathan::memory::make_allocator_reference(alloc);
		alloc_ref.deallocate(ptr, sizeof(T) * n, alignof(T));
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	requires(!std::is_array_v<T>)
	static inline void delete_at(T* ptr) {
		dealloc_delete(OpenVic::memory::make_tracked_allocator(RawAllocator {}), ptr);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	requires(std::is_unbounded_array_v<T>)
	static inline void delete_at(T* ptr, size_t n) {
		dealloc_delete(OpenVic::memory::make_tracked_allocator(RawAllocator {}), ptr, n);
	}
}