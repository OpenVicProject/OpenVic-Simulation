#pragma once

#include "openvic-simulation/core/memory/Tracker.hpp"

#include <foonathan/memory/container.hpp>
#include <foonathan/memory/default_allocator.hpp>

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using vector = foonathan::memory::vector<T, tracker<RawAllocator>>;
}
