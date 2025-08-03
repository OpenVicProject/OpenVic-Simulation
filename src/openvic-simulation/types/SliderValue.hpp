#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

namespace OpenVic {

	struct SliderValue {
	private:
		fixed_point_t PROPERTY(min);
		fixed_point_t PROPERTY(max);
		STATE_PROPERTY(fixed_point_t, value);

	public:
		constexpr SliderValue() {};

		void set_value(fixed_point_t new_value) {
			if (min <= max) {
				value.set(std::clamp(new_value, min, max));
			} else {
				// You *can* actually have min > max in Victoria 2
				// Such a situation will result in only being able to be move between the max and min value.
				// This logic replicates this "feature"
				if (new_value > max) {
					value.set(min);
				} else {
					value.set(max);
				}
			}
		}

		void set_bounds(fixed_point_t new_min, fixed_point_t new_max) {
			min = new_min;
			max = new_max;
			set_value(value.get_untracked());
		}
	};
}
