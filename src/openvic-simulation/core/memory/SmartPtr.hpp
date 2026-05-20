#pragma once

#include <memory>
#include <type_traits>
#include <utility>

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/smart_ptr.hpp>

#include "openvic-simulation/core/memory/make_tracked_allocator.hpp"
#include "openvic-simulation/core/memory/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<class RawAllocator>
	struct will_optimize_deleter_for : std::false_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::heap_allocator> : std::true_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::malloc_allocator> : std::true_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::new_allocator> : std::true_type {};


#ifdef DEBUG_ENABLED
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_ptr = foonathan::memory::unique_ptr<T, tracker<RawAllocator>>;

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using unique_base_ptr = foonathan::memory::unique_base_ptr<T, tracker<RawAllocator>>;

	template<typename T, class RawAllocator, typename... Args>
	requires(!std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, Args&&... args) {
		return foonathan::memory::allocate_unique<T>(
			OpenVic::memory::make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
		);
	}

	template<typename T, class RawAllocator>
	requires(std::is_array_v<T>)
	unique_ptr<T, std::decay_t<RawAllocator>> allocate_unique(RawAllocator&& alloc, std::size_t size) {
		return foonathan::memory::allocate_unique<T, tracker<RawAllocator>>(
			OpenVic::memory::make_tracked_allocator(std::forward<RawAllocator>(alloc)), size
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
				OpenVic::memory::make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
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
				OpenVic::memory::make_tracked_allocator(std::forward<RawAllocator>(alloc)), size
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
		return allocate_unique<T>(
			OpenVic::memory::make_tracked_allocator(RawAllocator {}),
			n
		);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	requires(!std::is_array_v<T>)
	static inline unique_ptr<T, RawAllocator> make_unique_for_overwrite() {
		using raw_ptr = std::unique_ptr<T, foonathan::memory::allocator_deallocator<T, tracker<RawAllocator>>>;

		tracker<RawAllocator> alloc = OpenVic::memory::make_tracked_allocator(RawAllocator {});
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

		tracker<RawAllocator> alloc = OpenVic::memory::make_tracked_allocator(RawAllocator {});
		auto memory = alloc.allocate_array(size, sizeof(T), alignof(T));
		// raw_ptr deallocates memory in case of constructor exception
		raw_ptr result(static_cast<T*>(memory), { alloc, size });
		// pass ownership to return value using a deleter that calls destructor
		return { result.release(), { alloc, size } };
	}

	template<typename T, class RawAllocator, typename... Args>
	std::shared_ptr<T> allocate_shared(RawAllocator&& alloc, Args&&... args) {
		return foonathan::memory::allocate_shared<T>(
			OpenVic::memory::make_tracked_allocator(
				std::forward<RawAllocator>(alloc)
			),
			std::forward<Args>(args)...
		);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	static inline std::shared_ptr<T> make_shared(Args&&... args) {
		return allocate_shared<T>(RawAllocator {}, std::forward<Args>(args)...);
	}
}
