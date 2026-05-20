#pragma once

#include <sstream>
#include <string>

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include "openvic-simulation/core/memory/MemoryTracker.hpp"

namespace OpenVic::memory {
	template<typename CharT, typename CharTraits = std::char_traits<CharT>, class RawAllocator = foonathan::memory::default_allocator>
	using basic_stringstream = std::basic_stringstream<
		CharT,
		CharTraits,
		foonathan::memory::std_allocator<CharT, tracker<RawAllocator>>
	>;

	template<class RawAllocator = foonathan::memory::default_allocator>
	using stringstream_alloc = basic_stringstream<char, std::char_traits<char>, RawAllocator>;
	template<class RawAllocator = foonathan::memory::default_allocator>
	using wstringstream_alloc = basic_stringstream<wchar_t, std::char_traits<wchar_t>, RawAllocator>;

	using stringstream = stringstream_alloc<>;
	using wstringstream = wstringstream_alloc<>;
}
