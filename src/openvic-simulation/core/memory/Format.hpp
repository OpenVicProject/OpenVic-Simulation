#pragma once

#include <iterator>

#include <fmt/base.h>
#include <fmt/format.h>

#include "openvic-simulation/core/memory/String.hpp"

#include <foonathan/memory/std_allocator.hpp>

namespace OpenVic::memory::fmt {
	template<typename T, class RawAllocator = foonathan::memory::default_allocator>
	using basic_memory_buffer =
		::fmt::basic_memory_buffer<T, ::fmt::inline_buffer_size, foonathan::memory::std_allocator<T, tracker<RawAllocator>>>;

	inline static memory::string vformat(::fmt::string_view fmt, ::fmt::format_args args) {
		memory::fmt::basic_memory_buffer<char> buf {};
		::fmt::vformat_to(std::back_inserter(buf), fmt, args);
		return memory::string(buf.data(), buf.size());
	}

	template<typename... Args>
	memory::string format(::fmt::string_view fmt, const Args&... args) {
		return memory::fmt::vformat(fmt, ::fmt::make_format_args(args...));
	}
}
