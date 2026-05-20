#pragma once

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"
#include "openvic-simulation/core/stl/containers/RingBuffer.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using RingBuffer = ::OpenVic::stl::RingBuffer<T, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}
