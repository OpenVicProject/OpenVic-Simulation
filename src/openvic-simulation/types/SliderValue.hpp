#pragma once

#include <algorithm>
#include <cstdint>

#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	using slider_value_int_type = int32_t;

	template<slider_value_int_type STRICT_MIN, slider_value_int_type STRICT_MAX>
	struct SliderValue {
		using int_type = slider_value_int_type;

		static constexpr int_type strict_min = STRICT_MIN;
		static constexpr int_type strict_max = STRICT_MAX;

		static_assert(strict_min < strict_max);

	private:
		int_type PROPERTY(min);
		int_type PROPERTY(max);
		int_type PROPERTY(value);

	public:
		constexpr SliderValue(
			int_type new_value = (strict_max - strict_min) / 2,
			int_type new_min = strict_min,
			int_type new_max = strict_max
		) {
			set_bounds(new_max, new_min);
			set_value(new_value);
		}

		constexpr void set_value(int_type new_value) {
			if (min <= max) {
				value = std::clamp(new_value, min, max);
			} else {
				// You *can* actually have min > max in Victoria 2
				// Such a situation will result in only being able to be move between the max and min value.
				// This logic replicates this "feature"
				if (new_value > max) {
					value = min;
				} else {
					value = max;
				}
			}
		}

		constexpr void set_bounds(int_type new_max, int_type new_min) {
			min = std::clamp(new_min, strict_min, strict_max);
			max = std::clamp(new_max, strict_min, strict_max);
			set_value(value);
		}
	};
}
