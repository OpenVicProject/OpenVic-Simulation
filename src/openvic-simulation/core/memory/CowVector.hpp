#pragma once

#include "openvic-simulation/core/container/CowVector.hpp"
#include "openvic-simulation/utility/MemoryTracker.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	template<
		typename T, typename RawAllocator = foonathan::memory::default_allocator,
		typename Allocator = foonathan::memory::std_allocator<T, memory::tracker<RawAllocator>>>
	using cow_vector = cow_vector<T, Allocator>;
}
