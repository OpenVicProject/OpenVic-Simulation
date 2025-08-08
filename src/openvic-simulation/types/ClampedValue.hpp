#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/reactive/MutableState.hpp"

namespace OpenVic {
	struct ReadOnlyClampedValue {
	protected:
		fixed_point_t PROPERTY_ACCESS(min, protected);
		fixed_point_t PROPERTY_ACCESS(max, protected);
		STATE_PROPERTY_ACCESS(fixed_point_t, value, protected);
		constexpr ReadOnlyClampedValue() {};
		ReadOnlyClampedValue(
			const fixed_point_t new_min,
			const fixed_point_t new_max
		) : min { new_min }, max { new_max } {};
	};

	struct ClampedValue : public ReadOnlyClampedValue {
	public:
		constexpr ClampedValue() {};
		ClampedValue(
			const fixed_point_t new_min,
			const fixed_point_t new_max,
			const fixed_point_t new_value
		) : ReadOnlyClampedValue(new_min, new_max) {
			set_value(new_value);
		};

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

#define CLAMPED_PROPERTY(NAME, ...) CLAMPED_PROPERTY_ACCESS(NAME, private, __VA_ARGS__)
#define CLAMPED_PROPERTY_ACCESS(NAME, ACCESS, ...) \
	ClampedValue NAME __VA_OPT__({) __VA_ARGS__ __VA_OPT__(}); \
\
public: \
	[[nodiscard]] ReadOnlyClampedValue& get_##NAME() { \
		return NAME; \
	} \
	[[nodiscard]] ReadOnlyClampedValue const& get_##NAME() const { \
		return NAME; \
	} \
	ACCESS:
