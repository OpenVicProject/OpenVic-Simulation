#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/WeightedSampling.hpp"

#include "utility/ExtendedMath.hpp"

#include <cstdint>
#include <limits>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::testing;

constexpr uint32_t max_random_value = std::numeric_limits<uint32_t>().max();

TEST_CASE("WeightedSampling limits", "[WeightedSampling]") {
	const fixed_point_t weights_sum = fixed_point_t::usable_max;
	constexpr size_t perfect_divisor = 257; //4294967295 is perfectly divisible by 257
	std::array<fixed_point_t, perfect_divisor> weights {};
	constexpr uint32_t step_size = max_random_value / weights.size();
	weights.fill(weights_sum / weights.size());
	for (size_t i = 0; i < weights.size(); ++i) {
		CHECK(sample_weighted_index(
			i * step_size,
			weights,
			weights_sum
		) == i);
	}
}
TEST_CASE("WeightedSampling weights", "[WeightedSampling]") {
	constexpr size_t max_length = 255; //weights_sum = (size^2+size)/2, this needs to be <= fixed_point_t::usable_max
	std::array<fixed_point_t, max_length> weights {};
	
	fixed_point_t weights_sum = 0;
	for (size_t i = 0; i < weights.size(); ++i) {
		const fixed_point_t weight = 1+i;
		weights_sum += weight;
		weights[i] = weight;
	}

	assert(weights_sum > 0 && weights_sum <= fixed_point_t::usable_max);

	fixed_point_t cumulative_weight = 0;
	for (size_t i = 0; i < weights.size(); ++i) {
		cumulative_weight += weights[i];
		const Int96DivisionResult random_value = portable_int96_div_int64(
			portable_int64_mult_uint32_96bit(
				cumulative_weight.get_raw_value(),
				max_random_value
			),
			weights_sum.get_raw_value()
		);
		assert(!random_value.quotient_overflow);
		CHECK(sample_weighted_index(
			static_cast<uint32_t>(random_value.quotient),
			weights,
			weights_sum
		) == i);
	}
}