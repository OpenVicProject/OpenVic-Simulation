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
		MutableState<fixed_point_t> STATE_PROPERTY(current);

	public:
		constexpr SliderValue() = default;
		SliderValue(SliderValue&&) = default;
		SliderValue(SliderValue const&) = delete;
		SliderValue& operator=(SliderValue&&) = default;
		SliderValue& operator=(SliderValue const&) = delete;

		void set_value(fixed_point_t new_value) {
			if (min <= max) {
				current.set(std::clamp(new_value, min, max));
			} else {
				// You *can* actually have min > max in Victoria 2
				// Such a situation will result in only being able to be move between the max and min value.
				// This logic replicates this "feature"
				if (new_value > max) {
					current.set(min);
				} else {
					current.set(max);
				}
			}
		}

		void set_bounds(fixed_point_t new_min, fixed_point_t new_max) {
			min = new_min;
			max = new_max;
			set_value(current.get());
		}
	};
}
