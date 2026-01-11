#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/core/random/WeightedSampling.hpp"

#include <cstdint>
#include <limits>
#include <boost/int128.hpp>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

constexpr uint32_t max_random_value = std::numeric_limits<uint32_t>().max();

TEST_CASE("WeightedSampling limits", "[WeightedSampling]") {
	const fixed_point_t weights_sum = fixed_point_t::usable_max;
	constexpr size_t perfect_divisor = 257; //4294967295 is perfectly divisible by 257
	std::array<fixed_point_t, perfect_divisor> weights {};
	assert(weights.size() < std::numeric_limits<int32_t>::max());
	constexpr uint32_t step_size = max_random_value / weights.size();
	weights.fill(weights_sum / static_cast<int32_t>(weights.size()));
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
	assert(weights.size() < std::numeric_limits<int32_t>::max());
	for (size_t i = 0; i < weights.size(); ++i) {
		const fixed_point_t weight = static_cast<int32_t>(1+i);
		weights_sum += weight;
		weights[i] = weight;
	}

	assert(weights_sum > 0 && weights_sum <= fixed_point_t::usable_max);

	fixed_point_t cumulative_weight = 0;
	for (size_t i = 0; i < weights.size(); ++i) {
		cumulative_weight += weights[i];
		const boost::int128::int128_t cumulative_weight_128 = cumulative_weight.get_raw_value();
		const boost::int128::int128_t max_random_value_128 = max_random_value;
		const boost::int128::int128_t weights_sum_128 = weights_sum.get_raw_value();
		const boost::int128::int128_t random_value = cumulative_weight_128 * max_random_value_128 / weights_sum_128;
		CHECK(sample_weighted_index(
			static_cast<uint32_t>(random_value),
			weights,
			weights_sum
		) == i);
	}
}