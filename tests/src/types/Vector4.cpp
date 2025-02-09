#include <cmath>
#include <cstdlib>

#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include "Approx.hpp"
#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp"
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::testing::approx_literal;

using VectorValueTypes = snitch::type_list<int32_t, OpenVic::fixed_point_t, double>;

TEMPLATE_LIST_TEST_CASE("vec4_t Constructor methods", "[vec4_t][vec4_t-constructor]", VectorValueTypes) {
	const vec4_t<TestType> vector_empty = vec4_t<TestType>();
	const vec4_t<TestType> vector_zero = vec4_t<TestType>(0, 0, 0, 0);
	CHECK(vector_empty == vector_zero);
}

TEMPLATE_LIST_TEST_CASE("vec4_t Length methods", "[vec4_t][vec4_t-length]", VectorValueTypes) {
	const vec4_t<TestType> vector1 = vec4_t<TestType>(10, 10, 10, 10);
	const vec4_t<TestType> vector2 = vec4_t<TestType>(20, 30, 40, 50);
	CHECK(vector1.length_squared() == 400);
	CHECK(vector2.length_squared() == 5400);
	CHECK(vector1.distance_squared(vector2) == 3000);
}

TEMPLATE_LIST_TEST_CASE("vec4_t Operators", "[vec4_t][vec4_t-operators]", VectorValueTypes) {
	const vec4_t<TestType> int1 = vec4_t<TestType>(4, 5, 9, 2);
	const vec4_t<TestType> int2 = vec4_t<TestType>(1, 2, 3, 1);

	CHECK((int1 + int2) == vec4_t<TestType>(5, 7, 12, 3));
	CHECK((int1 - int2) == vec4_t<TestType>(3, 3, 6, 1));
	CHECK((int1 * int2) == vec4_t<TestType>(4, 10, 27, 2));
	CHECK((int1 * 2) == vec4_t<TestType>(8, 10, 18, 4));
	CHECK(vec4_t<TestType>(ivec4_t(1, 2, 3, 4)) == vec4_t<TestType>(1, 2, 3, 4));
}

TEMPLATE_LIST_TEST_CASE("vec4_t Rounding methods", "[vec4_t][vec4_t-rounding]", VectorValueTypes) {
	const vec4_t<TestType> vector1 = vec4_t<TestType>(1, 3, 5, 1);
	const vec4_t<TestType> vector2 = vec4_t<TestType>(1, -3, -5, -1);
	CHECK(vector1.abs() == vector1);
	CHECK(vector2.abs() == vector1);
}

TEMPLATE_LIST_TEST_CASE("vec4_t Linear algebra methods", "[vec4_t][vec4_t-linear-algebra]", VectorValueTypes) {
	const vec4_t<TestType> vector_x = vec4_t<TestType>(1, 0, 0, 0);
	const vec4_t<TestType> vector_y = vec4_t<TestType>(0, 1, 0, 0);
	const vec4_t<TestType> a = vec4_t<TestType>(3, 8, 2, 9);
	const vec4_t<TestType> b = vec4_t<TestType>(5, 4, 7, 2);

	CHECK(vector_x.dot(vector_y) == 0);
	CHECK(vector_x.dot(vector_x) == 1);
	CHECK((vector_x * 10).dot(vector_x * 10) == 100);
	CHECK(a.dot(b) == 79);
	CHECK(vec4_t<TestType>(-a.x, a.y, -a.z, a.w).dot(vec4_t<TestType>(b.x, -b.y, b.z, b.w)) == -43);
}

TEST_CASE("fvec4_t Length methods", "[vec4_t][fvec4_t][fvec4_t-length]") {
	const fvec4_t vector1 = fvec4_t(10, 10, 10, 10);
	const fvec4_t vector2 = fvec4_t(20, 30, 40, 50);
	CHECK(vector1.length_squared().sqrt() == 20);
	CHECK(vector2.length_squared().sqrt() == testing::approx(73.484692283495));
	CHECK(vector1.distance_squared(vector2).sqrt() == testing::approx(54.772255750517));
}

TEST_CASE("dvec4_t Length methods", "[vec4_t][dvec4_t][dvec4_t-length]") {
	const dvec4_t vector1 = dvec4_t(10, 10, 10, 10);
	const dvec4_t vector2 = dvec4_t(20, 30, 40, 50);
	CHECK(std::sqrt(vector1.length_squared()) == 20);
	CHECK(std::sqrt(vector2.length_squared()) == testing::approx(73.484692283495));
	CHECK(std::sqrt(vector1.distance_squared(vector2)) == testing::approx(54.772255750517));
}

TEST_CASE("fvec4_t Operators", "[vec4_t][fvec4_t][fvec4_t-operators]") {
	const fixed_point_t _2_30 = fixed_point_t::_2() + fixed_point_t::_0_20() + fixed_point_t::_0_10();
	const fixed_point_t _4_90 = fixed_point_t::_4() + fixed_point_t::_0_50() + fixed_point_t::_0_20() * 2;
	const fixed_point_t _7_80 = //
		fixed_point_t::_4() + fixed_point_t::_2() + fixed_point_t::_1() + fixed_point_t::_0_50() + fixed_point_t::_0_20() +
		fixed_point_t::_0_10();
	const fixed_point_t _3_20 = fixed_point_t::_2() + fixed_point_t::_1() + fixed_point_t::_0_20();
	const fixed_point_t _1_20 = fixed_point_t::_0_20() * 6;
	const fixed_point_t _3_40 = fixed_point_t::_0_20() * 17;
	const fixed_point_t _5_60 = fixed_point_t::_4() + fixed_point_t::_1_50() + fixed_point_t::_0_10();
	const fixed_point_t _1_70 = fixed_point_t::_1_50() + fixed_point_t::_0_20();
	const fixed_point_t _0_75 = fixed_point_t::_0_25() * 3;
	const fixed_point_t _0_125 = fixed_point_t::_1() / 8;
	const fixed_point_t _0_625 = fixed_point_t::_0_50() + _0_125;

	const fvec4_t decimal1 = { _2_30, _4_90, _7_80, _3_20 };
	const fvec4_t decimal2 = { _1_20, _3_40, _5_60, _1_70 };
	const fvec4_t power1 = { _0_75, fixed_point_t::_1_50(), _0_625, _0_125 };
	const fvec4_t power2 = { fixed_point_t::_0_50(), _0_125, fixed_point_t::_0_25(), _0_75 };
	const fvec4_t int1 = fvec4_t(4, 5, 9, 2);
	const fvec4_t int2 = fvec4_t(1, 2, 3, 1);

	CHECK(decimal1 + decimal2 == testing::approx_vec4(3.5, 8.3, 13.4, 4.9));
	CHECK(power1 + power2 == testing::approx_vec4(1.25, 1.625, 0.875, 0.875));

	CHECK(
		decimal1 - decimal2 ==
		testing::approx_vec4 {
			(1.1_a).epsilon(testing::INACCURATE_EPSILON), //
			(1.5_a).epsilon(testing::INACCURATE_EPSILON), //
			(2.2_a).epsilon(testing::INACCURATE_EPSILON), //
			(1.5_a).epsilon(testing::INACCURATE_EPSILON) //

		}
	);
	CHECK(power1 - power2 == testing::approx_vec4(0.25, 1.375, 0.375, -0.625));

	CHECK(
		decimal1 * decimal2 ==
		testing::approx_vec4 {
			(2.76_a).epsilon(testing::INACCURATE_EPSILON), //
			(16.66_a).epsilon(testing::INACCURATE_EPSILON), //
			(43.68_a).epsilon(testing::INACCURATE_EPSILON), //
			(5.44_a).epsilon(testing::INACCURATE_EPSILON) //
		}
	);
	CHECK(power1 * power2 == testing::approx_vec4(0.375, 0.1875, 0.15625, 0.09375));

	CHECK(int1 / int2 == testing::approx_vec4(4, 2.5, 3, 2));
	CHECK(
		decimal1 / decimal2 ==
		testing::approx_vec4(1.91666666666666666, 1.44117647058823529, 1.39285714285714286, 1.88235294118)
	);
	CHECK(power1 / power2 == testing::approx_vec4(1.5, 12.0, 2.5, 1.0 / 6.0));

	CHECK(decimal1 * 2 == testing::approx_vec4(4.6, 9.8, 15.6, 6.4));
	CHECK(power1 * 2 == testing::approx_vec4(1.5, 3, 1.25, 0.25));

	CHECK(int1 / 2 == testing::approx_vec4(2, 2.5, 4.5, 1));
	CHECK(decimal1 / 2 == testing::approx_vec4(1.15, 2.45, 3.9, 1.6));
	CHECK(power1 / 2 == testing::approx_vec4(0.375, 0.75, 0.3125, 0.0625));

	CHECK(((ivec4_t)decimal1) == ivec4_t(2, 4, 7, 3));
	CHECK(((ivec4_t)decimal2) == ivec4_t(1, 3, 5, 1));
}

TEST_CASE("dvec4_t Operators", "[vec4_t][dvec4_t][dvec4_t-operators]") {
	const dvec4_t decimal1 = dvec4_t(2.3, 4.9, 7.8, 3.2);
	const dvec4_t decimal2 = dvec4_t(1.2, 3.4, 5.6, 1.7);
	const dvec4_t power1 = dvec4_t(0.75, 1.5, 0.625, 0.125);
	const dvec4_t power2 = dvec4_t(0.5, 0.125, 0.25, 0.75);
	const dvec4_t int1 = dvec4_t(4, 5, 9, 2);
	const dvec4_t int2 = dvec4_t(1, 2, 3, 1);

	CHECK(decimal1 + decimal2 == testing::approx_vec4(3.5, 8.3, 13.4, 4.9));
	CHECK(power1 + power2 == dvec4_t(1.25, 1.625, 0.875, 0.875));

	CHECK(decimal1 - decimal2 == testing::approx_vec4(1.1, 1.5, 2.2, 1.5));
	CHECK(power1 - power2 == dvec4_t(0.25, 1.375, 0.375, -0.625));

	CHECK(decimal1 * decimal2 == testing::approx_vec4(2.76, 16.66, 43.68, 5.44));
	CHECK(power1 * power2 == dvec4_t(0.375, 0.1875, 0.15625, 0.09375));

	CHECK(int1 / int2 == dvec4_t(4, 2.5, 3, 2));
	CHECK(
		decimal1 / decimal2 == testing::approx_vec4(1.91666666666666666, 1.44117647058823529, 1.39285714285714286, 1.8823529411)
	);
	CHECK(power1 / power2 == dvec4_t(1.5, 12.0, 2.5, 1.0 / 6.0));

	CHECK(decimal1 * 2 == testing::approx_vec4(4.6, 9.8, 15.6, 6.4));
	CHECK(power1 * 2 == dvec4_t(1.5, 3, 1.25, 0.25));

	CHECK(int1 / 2 == dvec4_t(2, 2.5, 4.5, 1));
	CHECK(decimal1 / 2 == testing::approx_vec4(1.15, 2.45, 3.9, 1.6));
	CHECK(power1 / 2 == dvec4_t(0.375, 0.75, 0.3125, 0.0625));

	CHECK(((ivec4_t)decimal1) == ivec4_t(2, 4, 7, 3));
	CHECK(((ivec4_t)decimal2) == ivec4_t(1, 3, 5, 1));
}
