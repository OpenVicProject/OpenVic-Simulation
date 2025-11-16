#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Typedefs.hpp"

namespace OpenVic {
	struct Fraction {
		fixed_point_t numerator;
		fixed_point_t denominator = fixed_point_t::_1;

		OV_SPEED_INLINE constexpr friend fixed_point_t operator*(Fraction const& lhs, fixed_point_t const& rhs) {
			return fixed_point_t::mul_div(rhs, lhs.numerator, lhs.denominator);
		}

		OV_SPEED_INLINE constexpr friend fixed_point_t operator*(fixed_point_t const& lhs, Fraction const& rhs) {
			return fixed_point_t::mul_div(lhs, rhs.numerator, rhs.denominator);
		}
	};
}