#include <cmath>
#include <cstdlib>

#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

#include "Approx.hpp"
#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp"
#include "Vector.hpp"
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::testing::approx_literal;

using VectorValueTypes = snitch::type_list<int32_t, OpenVic::fixed_point_t, double>;

TEMPLATE_LIST_TEST_CASE("vec2_t Constructor methods", "[vec2_t][vec2_t-constructor]", VectorValueTypes) {
	static constexpr vec2_t<TestType> vector_empty = vec2_t<TestType>();
	static constexpr vec2_t<TestType> vector_zero = vec2_t<TestType>(0, 0);
	CONSTEXPR_CHECK(vector_empty == vector_zero);
}

TEMPLATE_LIST_TEST_CASE("vec2_t Length methods", "[vec2_t][vec2_t-length]", VectorValueTypes) {
	static constexpr vec2_t<TestType> vector1 = vec2_t<TestType>(10, 10);
	static constexpr vec2_t<TestType> vector2 = vec2_t<TestType>(20, 30);
	CONSTEXPR_CHECK(vector1.length_squared() == 200);
	CONSTEXPR_CHECK(vector2.length_squared() == 1300);
	CONSTEXPR_CHECK(vector1.distance_squared(vector2) == 500);
}

TEMPLATE_LIST_TEST_CASE("vec2_t Operators", "[vec2_t][vec2_t-operators]", VectorValueTypes) {
	static constexpr vec2_t<TestType> int1 = vec2_t<TestType>(4, 10);
	static constexpr vec2_t<TestType> int2 = vec2_t<TestType>(2, 5);

	CONSTEXPR_CHECK((int1 + int2) == vec2_t<TestType>(6, 15));
	CONSTEXPR_CHECK((int1 - int2) == vec2_t<TestType>(2, 5));
	CONSTEXPR_CHECK((int1 * int2) == vec2_t<TestType>(8, 50));
	CONSTEXPR_CHECK((int1 / int2) == vec2_t<TestType>(2, 2));

	CONSTEXPR_CHECK((int1 * 2) == vec2_t<TestType>(8, 20));
	CONSTEXPR_CHECK((int1 / 2) == vec2_t<TestType>(2, 5));

	CONSTEXPR_CHECK(vec2_t<TestType>(ivec2_t(1, 2)) == vec2_t<TestType>(1, 2));

	CONSTEXPR_CHECK(int1 > int2);
	CHECK_FALSE(int1 == int2);
	CONSTEXPR_CHECK(int1 != int2);
	CONSTEXPR_CHECK(int2 < int1);

	CHECK_FALSE(vec2_t<TestType> { 421, 2160 }.is_within_bound(vec2_t<TestType> { 5616, 2160 }));
}

TEMPLATE_LIST_TEST_CASE(
	"vec2_t Marshal encode then decode", "[vec2_t][vec2_t-encode-decode][utility][Marshal][Marshal-encode-decode-vec2_t]",
	VectorValueTypes
) {
	static constexpr vec2_t<TestType> int1 = vec2_t<TestType>(4, 10);

	std::array<uint8_t, sizeof(int1)> buffer; // NOLINT
	size_t decode_count;
	CHECK(utility::encode(int1, buffer) == buffer.size());
	CHECK(int1 == utility::decode<vec2_t<TestType>>(buffer, decode_count));
	CHECK(decode_count == buffer.size());
}

TEMPLATE_LIST_TEST_CASE("vec2_t Rounding methods", "[vec2_t][vec2_t-rounding]", VectorValueTypes) {
	static constexpr vec2_t<TestType> vector1 = vec2_t<TestType>(1, 5);
	static constexpr vec2_t<TestType> vector2 = vec2_t<TestType>(1, -5);
	CONSTEXPR_CHECK(vector1.abs() == vector1);
	CONSTEXPR_CHECK(vector2.abs() == vector1);
}

TEMPLATE_LIST_TEST_CASE("vec2_t Linear algebra methods", "[vec2_t][vec2_t-linear-algebra]", VectorValueTypes) {
	static constexpr vec2_t<TestType> vector_x = vec2_t<TestType>(1, 0);
	static constexpr vec2_t<TestType> vector_y = vec2_t<TestType>(0, 1);
	static constexpr vec2_t<TestType> a = vec2_t<TestType>(3, 8);
	static constexpr vec2_t<TestType> b = vec2_t<TestType>(5, 4);
	CONSTEXPR_CHECK(vector_x.dot(vector_y) == 0);
	CONSTEXPR_CHECK(vector_x.dot(vector_x) == 1);
	CONSTEXPR_CHECK((vector_x * 10).dot(vector_x * 10) == 100);
	CONSTEXPR_CHECK(a.dot(b) == 47);
	CONSTEXPR_CHECK(vec2_t<TestType>(-a.x, a.y).dot(vec2_t<TestType>(b.x, -b.y)) == -47);
}

TEST_CASE("fvec2_t Length methods", "[vec2_t][fvec2_t][fvec2_t-length]") {
	static constexpr fvec2_t vector1 = fvec2_t(10, 10);
	static constexpr fvec2_t vector2 = fvec2_t(20, 30);
	CONSTEXPR_CHECK(vector1.length_squared().sqrt() == testing::approx(testing::SQRT2 * 10));
	CONSTEXPR_CHECK(vector2.length_squared().sqrt() == testing::approx(36.05551275463989293119));
	CONSTEXPR_CHECK(vector1.distance_squared(vector2).sqrt() == testing::approx(22.36067977499789696409));
}

TEST_CASE("dvec2_t Length methods", "[vec2_t][dvec2_t][dvec2_t-length]") {
	static constexpr dvec2_t vector1 = dvec2_t(10, 10);
	static constexpr dvec2_t vector2 = dvec2_t(20, 30);
	CHECK(std::sqrt(vector1.length_squared()) == testing::approx(testing::SQRT2 * 10));
	CHECK(std::sqrt(vector2.length_squared()) == testing::approx(36.05551275463989293119));
	CHECK(std::sqrt(vector1.distance_squared(vector2)) == testing::approx(22.36067977499789696409));
}

TEST_CASE("fvec2_t Operators", "[vec2_t][fvec2_t][fvec2_t-operators]") {
	static constexpr fixed_point_t _2_30 = fixed_point_t::_2 + fixed_point_t::_0_20 + fixed_point_t::_0_10;
	static constexpr fixed_point_t _4_90 = fixed_point_t::_4 + fixed_point_t::_0_50 + fixed_point_t::_0_20 * 2;
	static constexpr fixed_point_t _1_20 = fixed_point_t::_0_20 * 6;
	static constexpr fixed_point_t _3_40 = fixed_point_t::_0_20 * 17;
	static constexpr fixed_point_t _0_75 = fixed_point_t::_0_25 * 3;
	static constexpr fixed_point_t _0_125 = fixed_point_t::_1 / 8;

	static constexpr fvec2_t decimal1 = { _2_30, _4_90 };
	static constexpr fvec2_t decimal2 = { _1_20, _3_40 };
	static constexpr fvec2_t power1 = { _0_75, fixed_point_t::_1_50 };
	static constexpr fvec2_t power2 = { fixed_point_t::_0_50, _0_125 };
	static constexpr fvec2_t int1 = fvec2_t(4, 5);
	static constexpr fvec2_t int2 = fvec2_t(1, 2);

	CONSTEXPR_CHECK(decimal1 + decimal2 == testing::approx_vec2 { 3.5, 8.3 });
	CONSTEXPR_CHECK(power1 + power2 == testing::approx_vec2 { 1.25, 1.625 });

	CONSTEXPR_CHECK(
		decimal1 - decimal2 ==
		testing::approx_vec2 {
			(1.1_a).epsilon(testing::INACCURATE_EPSILON), //
			(1.5_a).epsilon(testing::INACCURATE_EPSILON) //
		}
	);
	CONSTEXPR_CHECK(power1 - power2 == testing::approx_vec2 { 0.25, 1.375 });

	CONSTEXPR_CHECK(
		decimal1 * decimal2 ==
		testing::approx_vec2 {
			(2.76_a).epsilon(testing::INACCURATE_EPSILON), //
			(16.66_a).epsilon(testing::INACCURATE_EPSILON) //
		}
	);
	CONSTEXPR_CHECK(power1 * power2 == testing::approx_vec2 { 0.375, 0.1875 });

	CONSTEXPR_CHECK(decimal1 / decimal2 == testing::approx_vec2 { 1.91666666666666666, 1.44117647058823529 });
	CONSTEXPR_CHECK(power1 / power2 == testing::approx_vec2 { 1.5, 12 });
	CONSTEXPR_CHECK(int1 / int2 == testing::approx_vec2 { 4, 2.5 });

	CONSTEXPR_CHECK(decimal1 * 2 == testing::approx_vec2 { 4.6, 9.8 });
	CONSTEXPR_CHECK(power1 * 2 == testing::approx_vec2 { 1.5, 3 });

	CONSTEXPR_CHECK(decimal1 / 2 == testing::approx_vec2 { 1.15, 2.45 });
	CONSTEXPR_CHECK(power1 / 2 == testing::approx_vec2 { 0.375, 0.75 });
	CONSTEXPR_CHECK(int1 / 2 == testing::approx_vec2 { 2, 2.5 });

	CONSTEXPR_CHECK(((ivec2_t)decimal1) == ivec2_t(2, 4));
	CONSTEXPR_CHECK(((ivec2_t)decimal2) == ivec2_t(1, 3));
}

TEST_CASE("dvec2_t Operators", "[vec2_t][dvec2_t][dvec2_t-operators]") {
	static constexpr dvec2_t decimal1 = { 2.3, 4.9 };
	static constexpr dvec2_t decimal2 = { 1.2, 3.4 };
	static constexpr dvec2_t power1 = { 0.75, 1.5 };
	static constexpr dvec2_t power2 = { 0.5, 0.125 };
	static constexpr dvec2_t int1 = dvec2_t(4, 5);
	static constexpr dvec2_t int2 = dvec2_t(1, 2);

	CONSTEXPR_CHECK(decimal1 + decimal2 == testing::approx_vec2 { 3.5, 8.3 });
	CONSTEXPR_CHECK(power1 + power2 == testing::approx_vec2 { 1.25, 1.625 });

	CONSTEXPR_CHECK(decimal1 - decimal2 == testing::approx_vec2 { 1.1, 1.5 });
	CONSTEXPR_CHECK(power1 - power2 == testing::approx_vec2 { 0.25, 1.375 });

	CONSTEXPR_CHECK(decimal1 * decimal2 == testing::approx_vec2 { 2.76, 16.66 });
	CONSTEXPR_CHECK(power1 * power2 == testing::approx_vec2 { 0.375, 0.1875 });

	CONSTEXPR_CHECK(decimal1 / decimal2 == testing::approx_vec2 { 1.91666666666666666, 1.44117647058823529 });
	CONSTEXPR_CHECK(power1 / power2 == testing::approx_vec2 { 1.5, 12 });
	CONSTEXPR_CHECK(int1 / int2 == testing::approx_vec2 { 4, 2.5 });

	CONSTEXPR_CHECK(decimal1 * 2 == testing::approx_vec2 { 4.6, 9.8 });
	CONSTEXPR_CHECK(power1 * 2 == testing::approx_vec2 { 1.5, 3 });

	CONSTEXPR_CHECK(decimal1 / 2 == testing::approx_vec2 { 1.15, 2.45 });
	CONSTEXPR_CHECK(power1 / 2 == testing::approx_vec2 { 0.375, 0.75 });
	CONSTEXPR_CHECK(int1 / 2 == testing::approx_vec2 { 2, 2.5 });

	CONSTEXPR_CHECK(((ivec2_t)decimal1) == ivec2_t(2, 4));
	CONSTEXPR_CHECK(((ivec2_t)decimal2) == ivec2_t(1, 3));
}
