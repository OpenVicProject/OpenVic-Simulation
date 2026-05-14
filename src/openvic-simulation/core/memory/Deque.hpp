#pragma once

#include <scoped_allocator>

#include <foonathan/memory/config.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"
#include "openvic-simulation/core/portable/Deque.hpp"

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	FOONATHAN_ALIAS_TEMPLATE(
		deque,
		OpenVic::utility::deque<
			T,
			foonathan::memory::std_allocator<T, tracker<RawAllocator>>
		>
	);

	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	FOONATHAN_ALIAS_TEMPLATE(
		deque_scoped_alloc,
		OpenVic::utility::deque<
			T,
			std::scoped_allocator_adaptor<foonathan::memory::std_allocator<T, tracker<RawAllocator>>>
		>
	);
}