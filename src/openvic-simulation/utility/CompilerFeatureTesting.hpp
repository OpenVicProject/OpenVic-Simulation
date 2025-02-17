#pragma once

#include <execution>

namespace OpenVic {
	template <class InputIt, class UnaryFunc>
	inline constexpr void try_parallel_for_each(
		InputIt first,
		InputIt last,
		UnaryFunc f
	) {
	#ifndef __cpp_lib_execution
		std::for_each(first, last, f);
	#else
		#if defined(__unix__) || defined(__MINGW32__)
			// We cannot use std::execution::par here as its implementation uses exceptions, which we have disabled
			std::for_each(first, last, f);
		#else
			if constexpr (__cpp_lib_execution >= 201603L) {
				std::for_each(std::execution::par, first, last, f);
			} else {
				std::for_each(first, last, f);
			}
		#endif
	#endif
	}
}