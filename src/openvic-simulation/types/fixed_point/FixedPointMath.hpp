#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic::FPMath {
	constexpr fixed_point_t sin(fixed_point_t number) {
		number %= fixed_point_t::pi2();
		number *= fixed_point_t::one_div_pi2();
		return FPLUT::sin(number.get_raw_value());
	}
}
