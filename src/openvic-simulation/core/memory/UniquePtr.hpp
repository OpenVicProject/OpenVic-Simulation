#pragma once

#include <type_traits>
#include <utility>

#include "openvic-simulation/core/memory/Allocator.hpp"
#include "openvic-simulation/core/memory/Tracker.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/smart_ptr.hpp>

namespace OpenVic::memory {
#ifdef DEBUG_ENABLED
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_ptr = foonathan::memory::unique_ptr<T, tracker<RawAllocator>>;

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_base_ptr = foonathan::memory::unique_base_ptr<T, tracker<RawAllocator>>;

	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, Args&&... args) {
		return foonathan::memory::allocate_unique<T>(
			make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
		);
	}

	template<typename T, class RawAllocator>
	requires(std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, std::size_t size) {
		return foonathan::memory::allocate_unique<T, tracker<RawAllocator>>(
			make_tracked_allocator(std::forward<RawAllocator>(alloc)), size
		);
	}
#else
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_ptr = std::conditional_t<
		will_optimize_deleter_for<RawAllocator>::value, std::unique_ptr<T>,
		foonathan::memory::unique_ptr<T, tracker<RawAllocator>>>;

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_base_ptr = std::conditional_t<
		will_optimize_deleter_for<RawAllocator>::value, std::unique_ptr<T>,
		foonathan::memory::unique_base_ptr<T, tracker<RawAllocator>>>;

	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, Args&&... args) {
		if constexpr (will_optimize_deleter_for<RawAllocator>::value) {
			return std::make_unique<T>(std::forward<Args>(args)...);
		} else {
			return foonathan::memory::allocate_unique<T>(
				make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
			);
		}
	}

	template<typename T, class RawAllocator>
	requires(std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, std::size_t size) {
		if constexpr (will_optimize_deleter_for<RawAllocator>::value) {
			return std::make_unique<T>(size);
		} else {
			return foonathan::memory::allocate_unique<T, tracker<RawAllocator>>(
				make_tracked_allocator(std::forward<RawAllocator>(alloc)), size
			);
		}
	}
#endif

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique(Args&&... args) {
		return allocate_unique<T>(RawAllocator {}, std::forward<Args>(args)...);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique(size_t n) {
		return allocate_unique<T>(make_tracked_allocator(RawAllocator {}), n);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique_for_overwrite() {
		using raw_ptr = std::unique_ptr<T, foonathan::memory::allocator_deallocator<T, tracker<RawAllocator>>>;

		tracker<RawAllocator> alloc = make_tracked_allocator(RawAllocator {});
		auto memory = alloc.allocate_node(sizeof(T), alignof(T));
		// raw_ptr deallocates memory in case of constructor exception
		raw_ptr result(static_cast<T*>(memory), { alloc });
		// pass ownership to return value using a deleter that calls destructor
		return { result.release(), { alloc } };
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(std::is_unbounded_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique_for_overwrite(size_t size) {
		using raw_ptr = std::unique_ptr<T, foonathan::memory::allocator_deallocator<T, tracker<RawAllocator>>>;

		tracker<RawAllocator> alloc = make_tracked_allocator(RawAllocator {});
		auto memory = alloc.allocate_array(size, sizeof(T), alignof(T));
		// raw_ptr deallocates memory in case of constructor exception
		raw_ptr result(static_cast<T*>(memory), { alloc, size });
		// pass ownership to return value using a deleter that calls destructor
		return { result.release(), { alloc, size } };
	}
}
