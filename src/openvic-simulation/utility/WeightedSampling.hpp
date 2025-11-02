#pragma once

#include <cstdint>
#include <limits>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	size_t sample_weighted_index(
		const uint32_t random_value, 
		const std::span<fixed_point_t> positive_weights,
		const fixed_point_t weights_sum
	) {
		if (positive_weights.empty() || weights_sum <= 0) {
			return -1;
		}

		constexpr uint32_t max = std::numeric_limits<uint32_t>().max();
		const fixed_point_t scaled_random = weights_sum * random_value;

		fixed_point_t cumulative_weight = 0;
		for (size_t i = 0; i < positive_weights.size(); ++i) {
			cumulative_weight += positive_weights[i];

			if (cumulative_weight * max >= scaled_random) {
				return static_cast<size_t>(i);
			}
		}

		return static_cast<size_t>(positive_weights.size() - 1);
	}
}