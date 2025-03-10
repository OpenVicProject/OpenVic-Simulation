#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include <cmath>
#include <cstdlib>
#include <numbers>
#include <string_view>
#include <system_error>

#include "Approx.hpp"
#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::testing::approx_literal;
using namespace std::string_view_literals;

TEST_CASE("fixed_point_t Constructor methods", "[fixed_point_t][fixed_point_t-constructor]") {
	const fixed_point_t empty = fixed_point_t();
	const fixed_point_t zero = fixed_point_t(0);
	const fixed_point_t constant_zero = fixed_point_t::_0();
	const fixed_point_t one = 1;
	const fixed_point_t constant_one = fixed_point_t::_1();
	const fixed_point_t usable_max = 32768;
	const fixed_point_t constant_usable_max = fixed_point_t::usable_max();
	const fixed_point_t usable_min = -32768;
	const fixed_point_t constant_usable_min = fixed_point_t::usable_min();
	CHECK(empty == zero);
	CHECK(empty == constant_zero);
	CHECK(zero == constant_zero);
	CHECK(one == constant_one);
	CHECK(usable_max == constant_usable_max);
	CHECK(usable_min == constant_usable_min);
}

TEST_CASE("fixed_point_t Linear algebra methods", "[fixed_point_t][fixed_point_t-linear-algebra]") {
	const fixed_point_t one = 1;
	const fixed_point_t two = 2;
	const fixed_point_t three = 3;
	CHECK(one.sin() == 0.8414306640625_a);
	CHECK(two.sin() == 0.9093170166015625_a);
	CHECK(three.sin() == 0.1412200927734375_a);
	CHECK(one.cos() == 0.5403900146484375_a);
	CHECK(two.cos() == -0.4160003662109375_a);
	CHECK(three.cos() == -0.989959716796875_a);

	CHECK(fixed_point_t::_0_50().sin() == 0.4793853759765625_a);
	CHECK(fixed_point_t::_0_50().cos() == 0.87762451171875_a);
}

TEST_CASE("fixed_point_t Constant methods", "[fixed_point_t][fixed_point_t-constants]") {
	CHECK(fixed_point_t::max().get_raw_value() == std::numeric_limits<int64_t>::max());
	CHECK(fixed_point_t::min().get_raw_value() == std::numeric_limits<int64_t>::min());
	CHECK(fixed_point_t::usable_max() == 32768);
	CHECK(fixed_point_t::usable_min() == -32768);
	CHECK(fixed_point_t::epsilon() == 0.0000152587890625_a);
	CHECK(fixed_point_t::_0() == 0);
	CHECK(fixed_point_t::_1() == 1);
	CHECK(fixed_point_t::_2() == 2);
	CHECK(fixed_point_t::_4() == 4);
	CHECK(fixed_point_t::_10() == 10);
	CHECK(fixed_point_t::_100() == 100);
	CHECK(fixed_point_t::_0_01() == 0.01_a);
	CHECK(fixed_point_t::_0_10() == 0.1_a);
	CHECK(fixed_point_t::_0_20() == 0.2_a);
	CHECK(fixed_point_t::_0_25() == 0.25_a);
	CHECK(fixed_point_t::_0_50() == 0.5_a);
	CHECK(fixed_point_t::_1_50() == 1.5_a);
	CHECK(fixed_point_t::minus_one() == -1);
	CHECK(fixed_point_t::pi() == testing::approx(std::numbers::pi));
	CHECK(fixed_point_t::pi2() == testing::approx(std::numbers::pi * 2));
	CHECK(fixed_point_t::pi_quarter() == testing::approx(std::numbers::pi / 4));
	CHECK(fixed_point_t::pi_half() == testing::approx(std::numbers::pi / 2));
	CHECK(fixed_point_t::one_div_pi2() == testing::approx(1 / (std::numbers::pi * 2)));
	CHECK(fixed_point_t::deg2rad() == 0.0174407958984375_a);
	CHECK(fixed_point_t::rad2deg() == 57.2957763671875_a);
	CHECK(fixed_point_t::e() == testing::approx(std::numbers::e));
}

TEST_CASE("fixed_point_t Rounding methods", "[fixed_point_t][fixed_point_t-rounding]") {
	const fixed_point_t neg_one = -1;
	const fixed_point_t neg_two = -2;
	const fixed_point_t neg_three = -3;

	const fixed_point_t _2_55 = fixed_point_t::_1_50() + fixed_point_t::_1() + fixed_point_t::_1() / 20;
	const fixed_point_t neg_2_55 = -_2_55;

	CHECK(neg_one.abs() == 1);
	CHECK(neg_two.abs() == 2);
	CHECK(neg_three.abs() == 3);
	CHECK(neg_2_55.abs() == 2.55_a);

	CHECK(_2_55.floor() == 2);
	CHECK(_2_55.ceil() == 3);
	CHECK(_2_55.round_down_to_multiple(3) == 0);
	CHECK(_2_55.round_up_to_multiple(5) == 5);
	CHECK(_2_55.round_down_to_multiple(fixed_point_t::_0_25()) == 2.50_a);
	CHECK(_2_55.round_up_to_multiple(fixed_point_t::_1_50()) == 3);
}

TEST_CASE("fixed_point_t Parse methods", "[fixed_point_t][fixed_point_t-parse]") {
	CHECK(fixed_point_t::parse(1) == 1);
	CHECK(fixed_point_t::parse(2) == 2);
	CHECK(fixed_point_t::parse(3) == 3);
	CHECK(fixed_point_t::parse(4) == 4);
	CHECK(fixed_point_t::parse(5) == 5);
	CHECK(fixed_point_t::parse(6) == 6);
	CHECK(fixed_point_t::parse(7) == 7);
	CHECK(fixed_point_t::parse(8) == 8);
	CHECK(fixed_point_t::parse(9) == 9);
	CHECK(fixed_point_t::parse(10) == 10);
	CHECK(fixed_point_t::parse_raw(10) == fixed_point_t::epsilon() * 10);
	CHECK(fixed_point_t::parse(10) == 10);

	static constexpr std::string_view fixed_point_str = "4.5432"sv;
	bool fixed_point_str_success;
	CHECK(fixed_point_t::parse(fixed_point_str, &fixed_point_str_success) == 4.5432_a);
	CHECK(fixed_point_str_success);

	static constexpr std::string_view neg_fixed_point_str = "-4.5432"sv;
	CHECK(fixed_point_t::parse(neg_fixed_point_str, &fixed_point_str_success) == -4.5432_a);
	CHECK(fixed_point_str_success);

	static constexpr std::string_view plus_fixed_point_str = "+4.5432"sv;
	CHECK(fixed_point_t::parse(plus_fixed_point_str, &fixed_point_str_success) == 4.5432_a);
	CHECK(fixed_point_str_success);

	fixed_point_t fp = fixed_point_t::_0();
	CHECK(fp.from_chars(plus_fixed_point_str.data(), plus_fixed_point_str.data() + plus_fixed_point_str.size()).ec == std::errc::invalid_argument);
	CHECK(fp == 0.0_a);
	CHECK(fp.from_chars_with_plus(plus_fixed_point_str.data(), plus_fixed_point_str.data() + plus_fixed_point_str.size()).ec == std::errc{});
	CHECK(fp == 4.5432_a);
}

TEST_CASE("fixed_point_t Other methods", "[fixed_point_t][fixed_point_t-other]") {
	CHECK_FALSE(fixed_point_t::_1().is_negative());
	CHECK(fixed_point_t::minus_one().is_negative());
	CHECK(fixed_point_t::_1().is_integer());
	CHECK(fixed_point_t::_2().is_integer());
	CHECK(fixed_point_t::_4().is_integer());
	CHECK_FALSE(fixed_point_t::_1_50().is_integer());
	CHECK_FALSE(fixed_point_t::_0_50().is_integer());
	CHECK_FALSE(fixed_point_t::_0_01().is_integer());
	CHECK(fixed_point_t::_0_50().sqrt() == 0.7071075439453125_a);
	CHECK((-fixed_point_t::_0_50()).sqrt() == 0);
}

TEST_CASE("fixed_point_t Operators", "[fixed_point_t][fixed_point_t-operators]") {
	const fixed_point_t decimal1 = fixed_point_t::_2() + fixed_point_t::_0_20() + fixed_point_t::_0_10();
	const fixed_point_t decimal2 = fixed_point_t::_0_20() * 6;
	const fixed_point_t decimal3 = fixed_point_t::_4() + fixed_point_t::_0_50() + fixed_point_t::_0_20() * 2;
	const fixed_point_t decimal4 = fixed_point_t::_0_20() * 17;

	const fixed_point_t power1 = fixed_point_t::_0_25() * 3;
	const fixed_point_t power2 = fixed_point_t::_0_50();
	const fixed_point_t power3 = fixed_point_t::_1_50();
	const fixed_point_t power4 = fixed_point_t::_1() / 8;

	const fixed_point_t int1 = 4;
	const fixed_point_t int2 = 1;
	const fixed_point_t int3 = 5;
	const fixed_point_t int4 = 2;

	CHECK(decimal1 + decimal2 == 3.5_a);
	CHECK(decimal3 + decimal4 == 8.3_a);
	CHECK(power1 + power2 == 1.25_a);
	CHECK(power3 + power4 == 1.625_a);
	CHECK(int1 + int2 == 5);
	CHECK(int1 + int2 == 5.0);
	CHECK(int3 + int4 == 7);
	CHECK(int3 + int4 == 7.0);

	CHECK(decimal1 - decimal2 == (1.1_a).epsilon(testing::INACCURATE_EPSILON));
	CHECK(decimal3 - decimal4 == (1.5_a).epsilon(testing::INACCURATE_EPSILON));
	CHECK(power1 - power2 == 0.25_a);
	CHECK(power3 - power4 == 1.375_a);
	CHECK(int1 - int2 == 3);
	CHECK(int1 - int2 == 3.0);
	CHECK(int4 - int3 == -3);
	CHECK(int4 - int3 == -3.0);

	CHECK(decimal1 * decimal2 == (2.76_a).epsilon(testing::INACCURATE_EPSILON));
	CHECK(decimal3 * decimal4 == (16.66_a).epsilon(testing::INACCURATE_EPSILON));
	CHECK(power1 * power2 == 0.375_a);
	CHECK(power3 * power4 == 0.1875_a);
	CHECK(int1 * int2 == 4);
	CHECK(int3 * int4 == 10);

	CHECK(decimal1 / decimal2 == 1.91666666666666666_a);
	CHECK(decimal3 / decimal4 == 1.44117647058823529_a);
	CHECK(power1 / power2 == 1.5_a);
	CHECK(power3 / power4 == 12);
	CHECK(power3 / power4 == 12.0);
	CHECK(int1 / int2 == 4);
	CHECK(int1 / int2 == 4.0);
	CHECK(int3 / int4 == 2.5_a);

	CHECK(decimal1 * 2 == 4.6_a);
	CHECK(decimal3 * 2 == 9.8_a);
	CHECK(power1 * 2 == 1.5_a);
	CHECK(power3 * 2 == 3);
	CHECK(power3 * 2 == 3.0);
	CHECK(int1 * 2 == 8);
	CHECK(int1 * 2 == 8.0);
	CHECK(int3 * 2 == 10);
	CHECK(int3 * 2 == 10.0);

	CHECK(decimal1 / 2 == 1.15_a);
	CHECK(decimal3 / 2 == 2.45_a);
	CHECK(power1 / 2 == 0.375_a);
	CHECK(power3 / 2 == 0.75_a);
	CHECK(int1 / 2 == 2);
	CHECK(int1 / 2 == 2.0);
	CHECK(int3 / 2 == 2.5_a);

	CHECK(((int32_t)decimal1) == 2);
	CHECK(((int32_t)decimal2) == 1);
	CHECK(((int32_t)decimal3) == 4);
	CHECK(((int32_t)decimal4) == 3);

	CHECK(fixed_point_t::mul_div(
		fixed_point_t::parse_raw(2),
		fixed_point_t::parse_raw(3),
		fixed_point_t::parse_raw(6)
	) == fixed_point_t::parse_raw(1));
}
