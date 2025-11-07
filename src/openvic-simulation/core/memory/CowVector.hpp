#pragma once

#include "openvic-simulation/core/container/CowVector.hpp"
#include "openvic-simulation/core/memory/Tracker.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using cow_vector = cow_vector<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}
