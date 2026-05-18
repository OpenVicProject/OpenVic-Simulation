#pragma once

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"
#include "openvic-simulation/types/CowPtr.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using cow_ptr = ::OpenVic::cow_ptr<
		T,
		foonathan::memory::std_allocator<T, tracker<RawAllocator>>
	>;
}