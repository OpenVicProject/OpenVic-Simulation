#pragma once

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"
#include "openvic-simulation/core/stl/containers/FixedVector.hpp"

namespace OpenVic::memory {
	template<typename T, typename SizeT = std::size_t, class RawAllocator = foonathan::memory::default_allocator>
	using FixedVector = ::OpenVic::stl::FixedVector<T, SizeT, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;
}
