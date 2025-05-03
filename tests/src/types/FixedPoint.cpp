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
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::testing::approx_literal;
using namespace std::string_view_literals;

TEST_CASE("fixed_point_t Constructor methods", "[fixed_point_t][fixed_point_t-constructor]") {
	static constexpr fixed_point_t empty = fixed_point_t();
	static constexpr fixed_point_t zero = fixed_point_t(0);
	static constexpr fixed_point_t constant_zero = fixed_point_t::_0();
	static constexpr fixed_point_t one = 1;
	static constexpr fixed_point_t constant_one = fixed_point_t::_1();
	static constexpr fixed_point_t usable_max = 32768;
	static constexpr fixed_point_t constant_usable_max = fixed_point_t::usable_max();
	static constexpr fixed_point_t usable_min = -32768;
	static constexpr fixed_point_t constant_usable_min = fixed_point_t::usable_min();
	CONSTEXPR_CHECK(empty == zero);
	CONSTEXPR_CHECK(empty == constant_zero);
	CONSTEXPR_CHECK(zero == constant_zero);
	CONSTEXPR_CHECK(one == constant_one);
	CONSTEXPR_CHECK(usable_max == constant_usable_max);
	CONSTEXPR_CHECK(usable_min == constant_usable_min);
}

TEST_CASE("fixed_point_t Linear algebra methods", "[fixed_point_t][fixed_point_t-linear-algebra]") {
	static constexpr fixed_point_t one = 1;
	static constexpr fixed_point_t two = 2;
	static constexpr fixed_point_t three = 3;
	CONSTEXPR_CHECK(one.sin() == 0.8414306640625_a);
	CONSTEXPR_CHECK(two.sin() == 0.9093170166015625_a);
	CONSTEXPR_CHECK(three.sin() == 0.1412200927734375_a);
	CONSTEXPR_CHECK(one.cos() == 0.5403900146484375_a);
	CONSTEXPR_CHECK(two.cos() == -0.4160003662109375_a);
	CONSTEXPR_CHECK(three.cos() == -0.989959716796875_a);

	CONSTEXPR_CHECK(fixed_point_t::_0_50().sin() == 0.4793853759765625_a);
	CONSTEXPR_CHECK(fixed_point_t::_0_50().cos() == 0.87762451171875_a);
}

TEST_CASE("fixed_point_t Constant methods", "[fixed_point_t][fixed_point_t-constants]") {
	CONSTEXPR_CHECK(fixed_point_t::max().get_raw_value() == std::numeric_limits<int64_t>::max());
	CONSTEXPR_CHECK(fixed_point_t::min().get_raw_value() == std::numeric_limits<int64_t>::min());
	CONSTEXPR_CHECK(fixed_point_t::usable_max() == 32768);
	CONSTEXPR_CHECK(fixed_point_t::usable_min() == -32768);
	CONSTEXPR_CHECK(fixed_point_t::epsilon() == 0.0000152587890625_a);
	CONSTEXPR_CHECK(fixed_point_t::_0() == 0);
	CONSTEXPR_CHECK(fixed_point_t::_1() == 1);
	CONSTEXPR_CHECK(fixed_point_t::_2() == 2);
	CONSTEXPR_CHECK(fixed_point_t::_4() == 4);
	CONSTEXPR_CHECK(fixed_point_t::_10() == 10);
	CONSTEXPR_CHECK(fixed_point_t::_100() == 100);
	CONSTEXPR_CHECK(fixed_point_t::_0_01() == 0.01_a);
	CONSTEXPR_CHECK(fixed_point_t::_0_10() == 0.1_a);
	CONSTEXPR_CHECK(fixed_point_t::_0_20() == 0.2_a);
	CONSTEXPR_CHECK(fixed_point_t::_0_25() == 0.25_a);
	CONSTEXPR_CHECK(fixed_point_t::_0_50() == 0.5_a);
	CONSTEXPR_CHECK(fixed_point_t::_1_50() == 1.5_a);
	CONSTEXPR_CHECK(fixed_point_t::minus_one() == -1);
	CONSTEXPR_CHECK(fixed_point_t::pi() == testing::approx(std::numbers::pi));
	CONSTEXPR_CHECK(fixed_point_t::pi2() == testing::approx(std::numbers::pi * 2));
	CONSTEXPR_CHECK(fixed_point_t::pi_quarter() == testing::approx(std::numbers::pi / 4));
	CONSTEXPR_CHECK(fixed_point_t::pi_half() == testing::approx(std::numbers::pi / 2));
	CONSTEXPR_CHECK(fixed_point_t::one_div_pi2() == testing::approx(1 / (std::numbers::pi * 2)));
	CONSTEXPR_CHECK(fixed_point_t::deg2rad() == 0.0174407958984375_a);
	CONSTEXPR_CHECK(fixed_point_t::rad2deg() == 57.2957763671875_a);
	CONSTEXPR_CHECK(fixed_point_t::e() == testing::approx(std::numbers::e));
}

TEST_CASE("fixed_point_t Rounding methods", "[fixed_point_t][fixed_point_t-rounding]") {
	static constexpr fixed_point_t neg_one = -1;
	static constexpr fixed_point_t neg_two = -2;
	static constexpr fixed_point_t neg_three = -3;

	static constexpr fixed_point_t _2_55 = fixed_point_t::_1_50() + fixed_point_t::_1() + fixed_point_t::_1() / 20;
	static constexpr fixed_point_t neg_2_55 = -_2_55;

	CONSTEXPR_CHECK(neg_one.abs() == 1);
	CONSTEXPR_CHECK(neg_two.abs() == 2);
	CONSTEXPR_CHECK(neg_three.abs() == 3);
	CONSTEXPR_CHECK(neg_2_55.abs() == 2.55_a);

	CONSTEXPR_CHECK(_2_55.floor() == 2);
	CONSTEXPR_CHECK(_2_55.ceil() == 3);
	CONSTEXPR_CHECK(_2_55.round_down_to_multiple(3) == 0);
	CONSTEXPR_CHECK(_2_55.round_up_to_multiple(5) == 5);
	CONSTEXPR_CHECK(_2_55.round_down_to_multiple(fixed_point_t::_0_25()) == 2.50_a);
	CONSTEXPR_CHECK(_2_55.round_up_to_multiple(fixed_point_t::_1_50()) == 3);
}

TEST_CASE("fixed_point_t Parse methods", "[fixed_point_t][fixed_point_t-parse]") {
	CONSTEXPR_CHECK(fixed_point_t::parse(1) == 1);
	CONSTEXPR_CHECK(fixed_point_t::parse(2) == 2);
	CONSTEXPR_CHECK(fixed_point_t::parse(3) == 3);
	CONSTEXPR_CHECK(fixed_point_t::parse(4) == 4);
	CONSTEXPR_CHECK(fixed_point_t::parse(5) == 5);
	CONSTEXPR_CHECK(fixed_point_t::parse(6) == 6);
	CONSTEXPR_CHECK(fixed_point_t::parse(7) == 7);
	CONSTEXPR_CHECK(fixed_point_t::parse(8) == 8);
	CONSTEXPR_CHECK(fixed_point_t::parse(9) == 9);
	CONSTEXPR_CHECK(fixed_point_t::parse(10) == 10);
	CONSTEXPR_CHECK(fixed_point_t::parse_raw(10) == fixed_point_t::epsilon() * 10);
	CONSTEXPR_CHECK(fixed_point_t::parse(10) == 10);

	static constexpr std::string_view fixed_point_str = "4.5432"sv;
	CONSTEXPR_CHECK(fixed_point_t::parse(fixed_point_str) == 4.5432_a);
	bool fixed_point_str_success;
	CHECK(fixed_point_t::parse(fixed_point_str, &fixed_point_str_success) == 4.5432_a);
	CHECK(fixed_point_str_success);

	static constexpr std::string_view neg_fixed_point_str = "-4.5432"sv;
	CONSTEXPR_CHECK(fixed_point_t::parse(neg_fixed_point_str) == -4.5432_a);
	CHECK(fixed_point_t::parse(neg_fixed_point_str, &fixed_point_str_success) == -4.5432_a);
	CHECK(fixed_point_str_success);

	static constexpr std::string_view plus_fixed_point_str = "+4.5432"sv;
	CONSTEXPR_CHECK(fixed_point_t::parse(plus_fixed_point_str) == 4.5432_a);
	CHECK(fixed_point_t::parse(plus_fixed_point_str, &fixed_point_str_success) == 4.5432_a);
	CHECK(fixed_point_str_success);

	static constexpr std::string_view neg_zero_fixed_point_str = "-0"sv;
	CONSTEXPR_CHECK(fixed_point_t::parse(neg_zero_fixed_point_str) == 0);
	CHECK(fixed_point_t::parse(neg_zero_fixed_point_str, &fixed_point_str_success) == 0);
	CHECK(fixed_point_str_success);

	static constexpr std::string_view neg_0_25_fixed_point_str = "-0.25"sv;
	CONSTEXPR_CHECK(fixed_point_t::parse(neg_0_25_fixed_point_str) == -0.25_a);
	CHECK(fixed_point_t::parse(neg_0_25_fixed_point_str, &fixed_point_str_success) == -0.25_a);
	CHECK(fixed_point_str_success);

	fixed_point_t fp = fixed_point_t::_0();
	CHECK(
		fp.from_chars(plus_fixed_point_str.data(), plus_fixed_point_str.data() + plus_fixed_point_str.size()).ec ==
		std::errc::invalid_argument
	);
	CHECK(fp == 0.0_a);
	CHECK(
		fp.from_chars_with_plus(plus_fixed_point_str.data(), plus_fixed_point_str.data() + plus_fixed_point_str.size()).ec ==
		std::errc {}
	);
	CHECK(fp == 4.5432_a);
}

TEST_CASE("fixed_point_t string methods", "[fixed_point_t][fixed_point_t-string]") {
	static constexpr fixed_point_t constant_zero = fixed_point_t::_0();
	static constexpr fixed_point_t one = 1;
	static constexpr fixed_point_t constant_one = fixed_point_t::_1();
	static constexpr fixed_point_t neg_one = -1;
	static constexpr fixed_point_t neg_two = -2;
	static constexpr fixed_point_t neg_three = -3;
	static constexpr fixed_point_t _2_55 = fixed_point_t::_1_50() + fixed_point_t::_1() + fixed_point_t::_1() / 20;
	static constexpr fixed_point_t neg_2_55 = -_2_55;
	static constexpr fixed_point_t _0_55 = fixed_point_t::_0_50() + fixed_point_t::_1() / 20;
	static constexpr fixed_point_t neg_0_55 = -_0_55;

	CONSTEXPR_CHECK(constant_zero.to_array() == "0"sv);
	CONSTEXPR_CHECK(one.to_array() == "1"sv);
	CONSTEXPR_CHECK(constant_one.to_array() == "1"sv);
	CONSTEXPR_CHECK(neg_one.to_array() == "-1"sv);
	CONSTEXPR_CHECK(neg_two.to_array() == "-2"sv);
	CONSTEXPR_CHECK(neg_three.to_array() == "-3"sv);
	CONSTEXPR_CHECK(_2_55.to_array() == "2.54998779296875"sv);
	CONSTEXPR_CHECK(neg_2_55.to_array() == "-2.54998779296875"sv);
	CONSTEXPR_CHECK(_2_55.to_array(2) == "2.55"sv);
	CONSTEXPR_CHECK(neg_2_55.to_array(2) == "-2.55"sv);
	CONSTEXPR_CHECK(_0_55.to_array(2) == "0.55"sv);
	CONSTEXPR_CHECK(neg_0_55.to_array(2) == "-0.55"sv);
}

TEST_CASE("fixed_point_t Other methods", "[fixed_point_t][fixed_point_t-other]") {
	CONSTEXPR_CHECK_FALSE(fixed_point_t::_1().is_negative());
	CONSTEXPR_CHECK(fixed_point_t::minus_one().is_negative());
	CONSTEXPR_CHECK(fixed_point_t::_1().is_integer());
	CONSTEXPR_CHECK(fixed_point_t::_2().is_integer());
	CONSTEXPR_CHECK(fixed_point_t::_4().is_integer());
	CONSTEXPR_CHECK_FALSE(fixed_point_t::_1_50().is_integer());
	CONSTEXPR_CHECK_FALSE(fixed_point_t::_0_50().is_integer());
	CONSTEXPR_CHECK_FALSE(fixed_point_t::_0_01().is_integer());
	CONSTEXPR_CHECK(fixed_point_t::_0_50().sqrt() == 0.7071075439453125_a);
	CONSTEXPR_CHECK((-fixed_point_t::_0_50()).sqrt() == 0);
}

TEST_CASE("fixed_point_t Operators", "[fixed_point_t][fixed_point_t-operators]") {
	static constexpr fixed_point_t decimal1 = fixed_point_t::_2() + fixed_point_t::_0_20() + fixed_point_t::_0_10();
	static constexpr fixed_point_t decimal2 = fixed_point_t::_0_20() * 6;
	static constexpr fixed_point_t decimal3 = fixed_point_t::_4() + fixed_point_t::_0_50() + fixed_point_t::_0_20() * 2;
	static constexpr fixed_point_t decimal4 = fixed_point_t::_0_20() * 17;

	static constexpr fixed_point_t power1 = fixed_point_t::_0_25() * 3;
	static constexpr fixed_point_t power2 = fixed_point_t::_0_50();
	static constexpr fixed_point_t power3 = fixed_point_t::_1_50();
	static constexpr fixed_point_t power4 = fixed_point_t::_1() / 8;

	static constexpr fixed_point_t int1 = 4;
	static constexpr fixed_point_t int2 = 1;
	static constexpr fixed_point_t int3 = 5;
	static constexpr fixed_point_t int4 = 2;

	CONSTEXPR_CHECK(decimal1 + decimal2 == 3.5_a);
	CONSTEXPR_CHECK(decimal3 + decimal4 == 8.3_a);
	CONSTEXPR_CHECK(power1 + power2 == 1.25_a);
	CONSTEXPR_CHECK(power3 + power4 == 1.625_a);
	CONSTEXPR_CHECK(int1 + int2 == 5);
	CONSTEXPR_CHECK(int1 + int2 == 5.0);
	CONSTEXPR_CHECK(int3 + int4 == 7);
	CONSTEXPR_CHECK(int3 + int4 == 7.0);

	CONSTEXPR_CHECK(decimal1 - decimal2 == (1.1_a).epsilon(testing::INACCURATE_EPSILON));
	CONSTEXPR_CHECK(decimal3 - decimal4 == (1.5_a).epsilon(testing::INACCURATE_EPSILON));
	CONSTEXPR_CHECK(power1 - power2 == 0.25_a);
	CONSTEXPR_CHECK(power3 - power4 == 1.375_a);
	CONSTEXPR_CHECK(int1 - int2 == 3);
	CONSTEXPR_CHECK(int1 - int2 == 3.0);
	CONSTEXPR_CHECK(int4 - int3 == -3);
	CONSTEXPR_CHECK(int4 - int3 == -3.0);

	CONSTEXPR_CHECK(decimal1 * decimal2 == (2.76_a).epsilon(testing::INACCURATE_EPSILON));
	CONSTEXPR_CHECK(decimal3 * decimal4 == (16.66_a).epsilon(testing::INACCURATE_EPSILON));
	CONSTEXPR_CHECK(power1 * power2 == 0.375_a);
	CONSTEXPR_CHECK(power3 * power4 == 0.1875_a);
	CONSTEXPR_CHECK(int1 * int2 == 4);
	CONSTEXPR_CHECK(int3 * int4 == 10);

	CONSTEXPR_CHECK(decimal1 / decimal2 == 1.91666666666666666_a);
	CONSTEXPR_CHECK(decimal3 / decimal4 == 1.44117647058823529_a);
	CONSTEXPR_CHECK(power1 / power2 == 1.5_a);
	CONSTEXPR_CHECK(power3 / power4 == 12);
	CONSTEXPR_CHECK(power3 / power4 == 12.0);
	CONSTEXPR_CHECK(int1 / int2 == 4);
	CONSTEXPR_CHECK(int1 / int2 == 4.0);
	CONSTEXPR_CHECK(int3 / int4 == 2.5_a);

	CONSTEXPR_CHECK(decimal1 * 2 == 4.6_a);
	CONSTEXPR_CHECK(decimal3 * 2 == 9.8_a);
	CONSTEXPR_CHECK(power1 * 2 == 1.5_a);
	CONSTEXPR_CHECK(power3 * 2 == 3);
	CONSTEXPR_CHECK(power3 * 2 == 3.0);
	CONSTEXPR_CHECK(int1 * 2 == 8);
	CONSTEXPR_CHECK(int1 * 2 == 8.0);
	CONSTEXPR_CHECK(int3 * 2 == 10);
	CONSTEXPR_CHECK(int3 * 2 == 10.0);

	CONSTEXPR_CHECK(decimal1 / 2 == 1.15_a);
	CONSTEXPR_CHECK(decimal3 / 2 == 2.45_a);
	CONSTEXPR_CHECK(power1 / 2 == 0.375_a);
	CONSTEXPR_CHECK(power3 / 2 == 0.75_a);
	CONSTEXPR_CHECK(int1 / 2 == 2);
	CONSTEXPR_CHECK(int1 / 2 == 2.0);
	CONSTEXPR_CHECK(int3 / 2 == 2.5_a);

	CONSTEXPR_CHECK(((int32_t)decimal1) == 2);
	CONSTEXPR_CHECK(((int32_t)decimal2) == 1);
	CONSTEXPR_CHECK(((int32_t)decimal3) == 4);
	CONSTEXPR_CHECK(((int32_t)decimal4) == 3);

	CONSTEXPR_CHECK(
		fixed_point_t::mul_div(fixed_point_t::parse_raw(2), fixed_point_t::parse_raw(3), fixed_point_t::parse_raw(6)) ==
		fixed_point_t::parse_raw(1)
	);
}
