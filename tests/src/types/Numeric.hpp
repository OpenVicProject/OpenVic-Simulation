#pragma once

#include <cmath>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

namespace OpenVic::testing {
	// Use if the fixed_point_t is not expected to be very close to the floating point value
	static constexpr double INACCURATE_EPSILON = static_cast<double>(std::numeric_limits<float>::epsilon()) * 1000;

	inline double approx_value(fixed_point_t value, double compare) {
		return std::abs(static_cast<double>(value) - compare);
	}

	inline double approx_value(double value, double compare) {
		return std::abs(value - compare);
	}
}

namespace snitch {
	[[nodiscard]] inline static constexpr bool append(snitch::small_string_span ss, OpenVic::fixed_point_t const& s) {
		return append(ss, static_cast<double>(s));
	}
}
