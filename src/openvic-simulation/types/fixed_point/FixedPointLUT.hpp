#pragma once

#include <cstdint>

namespace OpenVic::FPLUT {

#include "openvic-simulation/types/fixed_point/FixedPointLUT_sin.hpp"

	constexpr int32_t SHIFT = SIN_LUT_PRECISION - SIN_LUT_COUNT_LOG2;

	constexpr int64_t sin(int64_t value) {
		int sign = 1;
		if (value < 0) {
			value = -value;
			sign = -1;
		}

		int index = value >> SHIFT;
		int64_t a = SIN_LUT[index];
		int64_t b = SIN_LUT[index + 1];
		int64_t fraction = (value - (index << SHIFT)) << SIN_LUT_COUNT_LOG2;
		int64_t result = a + (((b - a) * fraction) >> SIN_LUT_PRECISION);
		return result * sign;
	}
}
