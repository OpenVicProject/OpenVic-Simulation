#pragma once

#include <type_traits>
#include <utility>

#include "openvic-simulation/core/memory/Tracker.hpp"

#include <foonathan/memory/allocator_storage.hpp>
#include <foonathan/memory/deleter.hpp>
#include <foonathan/memory/heap_allocator.hpp>
#include <foonathan/memory/new_allocator.hpp>

namespace OpenVic::memory {
	template<class RawAllocator>
	tracker<RawAllocator> make_tracked_allocator( //
		RawAllocator&& alloc
	) {
#ifdef DEBUG_ENABLED
		return foonathan::memory::make_tracked_allocator(
			typename tracker<RawAllocator>::tracker {}, std::forward<RawAllocator>(alloc)
		);
#else
		return std::forward<RawAllocator>(alloc);
#endif
	}

	template<class RawAllocator>
	using allocator_reference = foonathan::memory::allocator_reference<RawAllocator>;

	template<class RawAllocator>
	allocator_reference<RawAllocator> make_allocator_reference(RawAllocator&& alloc) {
		return foonathan::memory::make_allocator_reference(alloc);
	}

	template<typename T, class RawAllocator>
	using allocator_deleter = foonathan::memory::allocator_deleter<T, RawAllocator>;

	template<class RawAllocator>
	struct will_optimize_deleter_for : std::false_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::heap_allocator> : std::true_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::malloc_allocator> : std::true_type {};

	template<>
	struct will_optimize_deleter_for<foonathan::memory::new_allocator> : std::true_type {};
}
