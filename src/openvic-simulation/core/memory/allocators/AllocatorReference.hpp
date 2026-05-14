#pragma once

#include <foonathan/memory/allocator_storage.hpp>

namespace OpenVic::memory {
	template<class RawAllocator>
	using allocator_reference = foonathan::memory::allocator_reference<RawAllocator>;
}