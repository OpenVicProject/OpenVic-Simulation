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
		#ifdef __unix__
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