#pragma once

#include "openvic-simulation/core/container/FixedVector.hpp"
#include "openvic-simulation/core/memory/Tracker.hpp"

#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using FixedVector = FixedVector<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}
