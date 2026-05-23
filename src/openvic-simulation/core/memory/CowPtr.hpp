#pragma once

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"
#include "openvic-simulation/core/stl/containers/CowPtr.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using cow_ptr = ::OpenVic::stl::cow_ptr<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}
