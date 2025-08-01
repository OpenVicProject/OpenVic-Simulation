#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

	struct SliderValue {
	private:
		fixed_point_t PROPERTY(min);
		fixed_point_t PROPERTY(max);
		fixed_point_t PROPERTY(value);

	public:
		constexpr SliderValue() {};

		constexpr void set_value(fixed_point_t new_value) {
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

		constexpr void set_bounds(fixed_point_t new_min, fixed_point_t new_max) {
			min = new_min;
			max = new_max;
			set_value(value);
		}
	};
}
