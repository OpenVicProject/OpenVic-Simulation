#pragma once

#include <sstream>
#include <string>

#include "openvic-simulation/core/memory/Tracker.hpp"

#include <foonathan/memory/default_allocator.hpp>
#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory {
	template<typename T, typename CharTraits = std::char_traits<T>, class RawAllocator = foonathan::memory::default_allocator>
	using basic_stringstream =
		std::basic_stringstream<T, CharTraits, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;

	template<class RawAllocator = foonathan::memory::default_allocator>
	using stringstream_alloc = basic_stringstream<char, std::char_traits<char>, RawAllocator>;
	template<class RawAllocator = foonathan::memory::default_allocator>
	using wstringstream_alloc = basic_stringstream<wchar_t, std::char_traits<wchar_t>, RawAllocator>;

	using stringstream = stringstream_alloc<>;
	using wstringstream = wstringstream_alloc<>;
}
