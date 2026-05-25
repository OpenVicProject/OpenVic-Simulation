#pragma once

#include <concepts>
#include <ranges>

namespace OpenVic {
	template<typename ValueType, typename RangeType>
	concept RangeOfType = std::ranges::input_range<RangeType>
		&& std::convertible_to<std::ranges::range_reference_t<RangeType>, ValueType>;
}