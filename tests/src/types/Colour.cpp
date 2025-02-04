#include "openvic-simulation/types/Colour.hpp"

#include <cmath>
#include <cstdlib>
#include <string_view>
#include <type_traits>

#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

namespace snitch {
	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	inline static bool append( //
		snitch::small_string_span ss, OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits> const& s
	) {
		if constexpr (std::decay_t<decltype(s)>::colour_traits::has_alpha) {
			return append(ss, "(", (int64_t)s.red, ",", (int64_t)s.green, ",", (int64_t)s.blue, ",", (int64_t)s.alpha, ")");
		} else {
			return append(ss, "(", (int64_t)s.red, ",", (uint64_t)s.green, ",", (int64_t)s.blue, ")");
		}
	}
}

using namespace OpenVic;
using namespace std::string_view_literals;
using namespace OpenVic::colour_literals;

using ColourTypes = snitch::type_list<colour_rgb_t, colour_argb_t>;

TEMPLATE_LIST_TEST_CASE("basic_colour_t Constructor methods", "[basic_colour_t][basic_colour_t-constructor]", ColourTypes) {
	using value_t = TestType::value_type;

	const TestType blue_rgb = TestType(
		(value_t)(63.0 / 255 * TestType::max_value), //
		(value_t)(96.0 / 255 * TestType::max_value), //
		(value_t)(1.0 * TestType::max_value)
	);
	const TestType blue_float = TestType::from_floats(0.25098, 0.376471, 1);

	CHECK(blue_rgb == blue_float);

	const TestType invalid = TestType::null();

	CHECK(invalid == TestType());
	CHECK(invalid.is_null());
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Operators", "[basic_colour_t][basic_colour_t-operators]", ColourTypes) {
	const TestType blue = TestType::from_floats(0.2, 0.2, 1);

	CHECK(-blue == TestType::from_floats(0.8, 0.8, 0));
	CHECK(blue.invert() == TestType::from_floats(0.8, 0.8, 0));
	CHECK(blue.contrast() == TestType::from_floats(0, 0, 0));
	CHECK(blue.invert().contrast() == TestType::from_floats(1, 1, 1));
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Conversion methods", "[basic_colour_t][basic_colour_t-conversion]", ColourTypes) {
	const TestType cyan = TestType::from_floats(0, 1, 1);

	CHECK(cyan.as_rgb() == 0x00FFFF);
	CHECK(cyan.as_argb() == 0xFF00FFFF);
	CHECK(cyan.as_rgba() == 0x00FFFFFF);
	CHECK(cyan.redf() == 0);
	CHECK(cyan.greenf() == 1);
	CHECK(cyan.bluef() == 1);
	CHECK(cyan.alphaf() == 1);
	CHECK(cyan.to_hex_string(true) == "00FFFFFF");
	CHECK(cyan.to_hex_string(false) == "00FFFF");
	CHECK(cyan.to_argb_hex_string() == "FF00FFFF");
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Parse methods", "[basic_colour_t][basic_colour_t-parse]", ColourTypes) {
	static constexpr std::string_view yellow_rgb = "FFFF00"sv;
	static constexpr std::string_view yellow_rgba = "FFFF00FF"sv;
	static constexpr std::string_view yellow_argb = "FFFFFF00"sv;
	static constexpr std::string_view hex_yellow_rgb = "0xFFFF00"sv;
	static constexpr std::string_view hex_yellow_rgba = "0xFFFF00FF"sv;
	static constexpr std::string_view hex_yellow_argb = "0xFFFFFF00"sv;
	const TestType yellow = TestType::from_floats(1, 1, 0);

	CHECK(TestType::from_rgb_string(yellow_rgb) == yellow);
	CHECK(TestType::from_rgba_string(yellow_rgba) == yellow);
	CHECK(TestType::from_argb_string(yellow_argb) == yellow);
	CHECK(TestType::from_rgb_string(hex_yellow_rgb) == yellow);
	CHECK(TestType::from_rgba_string(hex_yellow_rgba) == yellow);
	CHECK(TestType::from_argb_string(hex_yellow_argb) == yellow);
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Other methods", "[basic_colour_t][basic_colour_t-other]", ColourTypes) {
	const TestType blue = TestType::from_floats(0.2, 0.2, 1);

	CHECK(blue.with_red(255) == TestType::from_floats(1, 0.2, 1));
	CHECK(blue.with_green(255) == TestType::from_floats(0.2, 1, 1));
	CHECK(blue.with_blue(0) == TestType::from_floats(0.2, 0.2, 0));
}

TEST_CASE("colour_rgb_t Constructor methods", "[basic_colour_t][colour_rgb_t][colour_rgb_t-constructor]") {
	const colour_rgb_t blue_rgb = colour_rgb_t(
		63.0 / 255 * 255, //
		96.0 / 255 * 255, //
		1.0 * 255
	);
	const colour_rgb_t blue_int = colour_rgb_t::from_integer(0x3F'60'FF);
	const colour_rgb_t blue_rgba = colour_rgb_t::from_rgba(0x3F'60'FF'FF);
	const colour_rgb_t blue_argb = colour_rgb_t::from_argb(0xFF'3F'60'FF);
	const colour_rgb_t blue_udl = 0x3F'60'FF_rgb;

	CHECK(blue_rgb == blue_int);
	CHECK(blue_rgb == blue_rgba);
	CHECK(blue_rgb == blue_argb);
	CHECK(blue_rgb == blue_udl);

	const colour_rgb_t fill = colour_rgb_t::fill_as(100);

	CHECK(fill == colour_rgb_t::from_floats(100.0 / 255, 100.0 / 255, 100.0 / 255));
}

TEST_CASE("colour_argb_t Constructor methods", "[basic_colour_t][colour_argb_t][colour_argb_t-constructor]") {
	const colour_argb_t blue_rgb = colour_argb_t(
		63.0 / 255 * 255, //
		96.0 / 255 * 255, //
		1.0 * 255
	);
	const colour_argb_t blue_int = colour_argb_t::from_integer(0x3F'60'FF'FF);
	const colour_argb_t blue_rgba = colour_argb_t::from_rgba(0x3F'60'FF'FF);
	const colour_argb_t blue_argb = colour_argb_t::from_argb(0xFF'3F'60'FF);
	const colour_argb_t blue_rgb2 = colour_argb_t::from_rgb(0x3F'60'FF);
	const colour_argb_t blue_udl = 0x3F'60'FF'FF_rgba;
	const colour_argb_t blue_argb_udl = 0xFF'3F'60'FF_argb;

	CHECK(blue_rgb == blue_int);
	CHECK(blue_rgb == blue_rgba);
	CHECK(blue_rgb == blue_argb);
	CHECK(blue_rgb == blue_rgb2);
	CHECK(blue_rgb == blue_udl);
	CHECK(blue_rgb == blue_argb_udl);

	const colour_argb_t cyan_rgb = colour_argb_t::from_floats(0, 1, 1);
	const colour_argb_t cyan_int = colour_argb_t::from_integer(0x00'FF'FF'FF);
	const colour_argb_t cyan_rgba = colour_argb_t::from_rgba(0x00'FF'FF'FF);
	const colour_argb_t cyan_argb = colour_argb_t::from_argb(0xFF'00'FF'FF);
	const colour_argb_t cyan_rgb2 = colour_argb_t::from_rgb(0x00'FF'FF);
	const colour_argb_t cyan_udl = 0x00'FF'FF'FF_rgba;
	const colour_argb_t cyan_argb_udl = 0xFF'00'FF'FF_argb;

	CHECK(cyan_rgb == cyan_int);
	CHECK(cyan_rgb == cyan_rgba);
	CHECK(cyan_rgb == cyan_argb);
	CHECK(cyan_rgb == cyan_rgb2);
	CHECK(cyan_rgb == cyan_udl);
	CHECK(cyan_rgb == cyan_argb_udl);

	const colour_argb_t fill = colour_argb_t::fill_as(100);

	CHECK(fill == colour_argb_t::from_floats(100.0 / 255, 100.0 / 255, 100.0 / 255, 100.0 / 255));
}

TEST_CASE("colour_rgb_t Parse methods", "[basic_colour_t][colour_rgb_t][colour_rgb_t-parse]") {
	static constexpr std::string_view yellow_rgb = "FFFF00"sv;
	static constexpr std::string_view hex_yellow_rgb = "FFFF00"sv;
	const colour_rgb_t yellow = colour_rgb_t::from_floats(1, 1, 0);

	CHECK(colour_rgb_t::from_string(yellow_rgb) == yellow);
	CHECK(colour_rgb_t::from_string(hex_yellow_rgb) == yellow);
}

TEST_CASE("colour_argb_t Parse methods", "[basic_colour_t][colour_argb_t][colour_argb_t-parse]") {
	static constexpr std::string_view yellow_rgba = "FFFF00FF"sv;
	static constexpr std::string_view hex_yellow_rgba = "0xFFFF00FF"sv;
	const colour_argb_t yellow = colour_argb_t::from_floats(1, 1, 0);

	CHECK(colour_argb_t::from_string(yellow_rgba) == yellow);
	CHECK(colour_argb_t::from_string(hex_yellow_rgba) == yellow);
}

TEST_CASE("colour_argb_t Other methods", "[basic_colour_t][colour_argb_t][colour_argb_t-other]") {
	const colour_argb_t blue = colour_argb_t::from_floats(0.2, 0.2, 1);

	CHECK(blue.with_alpha(1) == colour_argb_t::from_floats(0.2, 0.2, 1, 1.0 / 255));
}
