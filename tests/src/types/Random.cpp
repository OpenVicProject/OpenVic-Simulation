#include "openvic-simulation/types/fixed_point/Random.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include <concepts>
#include <cstdint>
#include <limits>

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("uniform_real_distribution Constructor methods", "[uniform_real_distribution]") {
	OpenVic::uniform_real_distribution default_constructed;
	CHECK(default_constructed.min() == 0);
	CHECK(default_constructed.max() == fixed_point_t::usable_max);
	
	const fixed_point_t explicit_min = fixed_point_t::_0_50;
	OpenVic::uniform_real_distribution constructed_with_explicit_min { explicit_min };
	CHECK(constructed_with_explicit_min.min() == explicit_min);
	CHECK(constructed_with_explicit_min.max() == fixed_point_t::usable_max);
	
	const fixed_point_t explicit_max = fixed_point_t::_1_50;
	OpenVic::uniform_real_distribution constructed_with_explicit_min_max { explicit_min, explicit_max };
	CHECK(constructed_with_explicit_min_max.min() == explicit_min);
	CHECK(constructed_with_explicit_min_max.max() == explicit_max);
}

template<std::integral T>
struct mock_generator {
public:
	T next_value;
	T operator() () {
		return next_value;
	}

	static constexpr T max() {
		return std::numeric_limits<T>::max();
	}

	static constexpr T min() {
		return std::numeric_limits<T>::min();
	}
};

TEST_CASE("uniform_real_distribution () operator", "[uniform_real_distribution]") {
	const fixed_point_t explicit_min = fixed_point_t::_0_50;
	const fixed_point_t explicit_max = fixed_point_t::_1_50;
	OpenVic::uniform_real_distribution constructed_with_explicit_min_max { explicit_min, explicit_max };
	mock_generator<uint32_t> random_number_generator{};

	random_number_generator.next_value = mock_generator<uint32_t>::min();
	const fixed_point_t number_from_uint32_min = constructed_with_explicit_min_max(random_number_generator);
	CHECK(number_from_uint32_min == explicit_min);
	
	random_number_generator.next_value = mock_generator<uint32_t>::max()/2 + mock_generator<uint32_t>::min()/2;
	const fixed_point_t number_from_uint32_middle = constructed_with_explicit_min_max(random_number_generator);
	CHECK(number_from_uint32_middle == (explicit_max + explicit_min)/2 - fixed_point_t::epsilon); //-epsilon as (uint32_t max)/2 gets truncated

	random_number_generator.next_value = mock_generator<uint32_t>::max();
	const fixed_point_t number_from_uint32_max = constructed_with_explicit_min_max(random_number_generator);
	CHECK(number_from_uint32_max == explicit_max);
}