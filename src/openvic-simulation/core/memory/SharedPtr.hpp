#pragma once

#include <memory>
#include <utility>

#include "openvic-simulation/core/memory/Allocator.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/smart_ptr.hpp>

namespace OpenVic::memory {
	template<typename T, class RawAllocator, typename... Args>
	std::shared_ptr<T> allocate_shared(RawAllocator&& alloc, Args&&... args) {
		return foonathan::memory::allocate_shared<T>(
			make_tracked_allocator(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...
		);
	}

	template<typename T, class RawAllocator = foonathan::memory::default_allocator, typename... Args>
	static inline std::shared_ptr<T> make_shared(Args&&... args) {
		return allocate_shared<T>(RawAllocator {}, std::forward<Args>(args)...);
	}
}
