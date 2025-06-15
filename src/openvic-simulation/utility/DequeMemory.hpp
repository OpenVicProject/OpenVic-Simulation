#pragma once

#include <scoped_allocator>

#include "openvic-simulation/utility/Deque.hpp"

#include <foonathan/memory/config.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::utility::memory {
	template<typename T, class RawAllocator>
	FOONATHAN_ALIAS_TEMPLATE(deque, OpenVic::utility::deque<T, foonathan::memory::std_allocator<T, RawAllocator>>);

	template<typename T, class RawAllocator>
	FOONATHAN_ALIAS_TEMPLATE(deque_scoped_alloc, OpenVic::utility::deque<T, std::scoped_allocator_adaptor<foonathan::memory::std_allocator<T, RawAllocator>>>);
}
