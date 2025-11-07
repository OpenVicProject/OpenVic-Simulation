#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include "openvic-simulation/core/memory/Allocator.hpp"

#include <foonathan/memory/default_allocator.hpp>

namespace OpenVic::memory {
	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline T* allocate_new(RawAllocator&& alloc, Args&&... args) {
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		T* ptr = static_cast<T*>(alloc_ref.allocate_node(sizeof(T), alignof(T)));
		::new (ptr) T(std::forward<Args>(args)...);
		return ptr;
	}

	template<typename T, class RawAllocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline T* allocate_new(RawAllocator&& alloc, std::size_t n) {
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		return static_cast<T*>(alloc_ref.allocate_node(sizeof(T) * n, alignof(T)));
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline T* make_new(Args&&... args) {
		return allocate_new<T>(make_tracked_allocator(RawAllocator {}), std::forward<Args>(args)...);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline T* make_new(std::size_t n) {
		return allocate_new<T>(make_tracked_allocator(RawAllocator {}), n);
	}

	template<typename T, class RawAllocator>
	requires(!std::is_array_v<T>)
	static inline void dealloc_delete(RawAllocator&& alloc, T* ptr) {
		ptr->~T();
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		alloc_ref.deallocate(ptr, sizeof(T), alignof(T));
	}

	template<typename T, class RawAllocator>
	requires(std::is_unbounded_array_v<T>)
	static inline void dealloc_delete(RawAllocator&& alloc, T* ptr, std::size_t n) {
		std::destroy_n(ptr, n);
		allocator_reference<RawAllocator> alloc_ref = ::OpenVic::memory::make_allocator_reference(alloc);
		alloc_ref.deallocate(ptr, sizeof(T) * n, alignof(T));
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	requires(!std::is_array_v<T>)
	static inline void delete_at(T* ptr) {
		dealloc_delete(make_tracked_allocator(RawAllocator {}), ptr);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	requires(std::is_unbounded_array_v<T>)
	static inline void delete_at(T* ptr, std::size_t n) {
		dealloc_delete(make_tracked_allocator(RawAllocator {}), ptr, n);
	}
}
