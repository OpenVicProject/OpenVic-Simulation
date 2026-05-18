#pragma once

#include <foonathan/memory/container.hpp>
#include <foonathan/memory/default_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using vector = foonathan::memory::vector<T, tracker<RawAllocator>>;
}