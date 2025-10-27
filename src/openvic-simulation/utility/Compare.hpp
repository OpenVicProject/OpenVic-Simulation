#pragma once

#include <compare>
#include <type_traits>

#include "openvic-simulation/utility/Concepts.hpp"

namespace OpenVic {
	[[nodiscard]] inline constexpr auto three_way_compare(auto&& left, auto&& right)
	requires requires {
		{ left < right } -> boolean_testable;
		{ right < left } -> boolean_testable;
	}
	{
// This is Apple's fault again
#if __cpp_lib_three_way_comparison >= 201907L
		if constexpr (std::three_way_comparable_with<std::decay_t<decltype(left)>, std::decay_t<decltype(right)>>) {
			return left <=> right;
		} else
#endif
		{
			if (left < right) {
				return std::weak_ordering::less;
			} else if (right < left) {
				return std::weak_ordering::greater;
			} else {
				return std::weak_ordering::equivalent;
			}
		}
	};
}
