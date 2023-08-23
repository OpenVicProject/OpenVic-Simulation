#pragma once

#include "FP.hpp"

namespace OpenVic::FPMath {
	constexpr FP sin(FP number) {
		number %= FP::pi2();
		number *= FP::one_div_pi2();
		return FPLUT::sin(number.get_raw_value());
	}
}
