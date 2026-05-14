#pragma once

#include <utility>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<class RawAllocator>
	tracker<RawAllocator> make_tracked_allocator(
		RawAllocator&& alloc
	) {
#ifdef DEBUG_ENABLED
		return foonathan::memory::make_tracked_allocator(
			typename tracker<RawAllocator>::tracker {},
			std::forward<RawAllocator>(alloc)
		);
#else
		return std::forward<RawAllocator>(alloc);
#endif
	}
}