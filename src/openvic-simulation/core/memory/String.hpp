#pragma once

#include <string>

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<typename CharT, class RawAllocator = foonathan::memory::default_allocator>
	using basic_string = std::basic_string<
		CharT,
		std::char_traits<CharT>,
		foonathan::memory::std_allocator<CharT, tracker<RawAllocator>>
	>;

	template<class RawAllocator = foonathan::memory::default_allocator>
	using string_alloc = basic_string<char, RawAllocator>;

	using string = string_alloc<>;

	template<class RawAllocator = foonathan::memory::default_allocator>
	using wstring_alloc = basic_string<wchar_t, RawAllocator>;

	using wstring = wstring_alloc<>;
}