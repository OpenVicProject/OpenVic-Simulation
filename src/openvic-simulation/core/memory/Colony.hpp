#pragma once

#include <plf_colony.h>

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using colony = plf::colony<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}