#pragma once

#include <scoped_allocator>

#include "openvic-simulation/core/memory/Tracker.hpp"
#include "openvic-simulation/core/portable/Deque.hpp"

#include <foonathan/memory/config.hpp>
#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory::utility {
	template<typename T, class RawAllocator>
	FOONATHAN_ALIAS_TEMPLATE(deque, OpenVic::utility::deque<T, ::foonathan::memory::std_allocator<T, RawAllocator>>);

	template<typename T, class RawAllocator>
	FOONATHAN_ALIAS_TEMPLATE(deque_scoped_alloc, OpenVic::utility::deque<T, std::scoped_allocator_adaptor<foonathan::memory::std_allocator<T, RawAllocator>>>);
}

namespace OpenVic::memory {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using deque = OpenVic::memory::utility::deque<T, tracker<RawAllocator>>;
}
