#pragma once

#include <execution>

namespace OpenVic {
	template <class InputIt, class UnaryFunc>
	inline constexpr void try_parallel_for_each(
		InputIt first,
		InputIt last,
		UnaryFunc f
	) {
	#ifdef __cpp_lib_execution
		if constexpr (__cpp_lib_execution >= 201603L) {
			std::for_each(std::execution::par, first, last, f);
		} else {
			std::for_each(first, last, f);
		}
	#else
		std::for_each(first, last, f);
	#endif
	}
}