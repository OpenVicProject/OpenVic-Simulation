#include "openvic-simulation/core/object/Colour.hpp"

#include <string_view>

#include "Colour.hpp" // IWYU pragma: keep
#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace std::string_view_literals;
using namespace OpenVic::colour_literals;

using ColourTypes = snitch::type_list<colour_rgb_t, colour_argb_t>;

TEMPLATE_LIST_TEST_CASE("basic_colour_t Constructor methods", "[basic_colour_t][basic_colour_t-constructor]", ColourTypes) {
	using value_t = TestType::value_type;

	static constexpr TestType blue_rgb = TestType(
		(value_t)(63.0 / 255 * TestType::max_value), //
		(value_t)(96.0 / 255 * TestType::max_value), //
		(value_t)(1.0 * TestType::max_value)
	);
	static constexpr TestType blue_float = TestType::from_floats(0.25098, 0.376471, 1);

	CONSTEXPR_CHECK(blue_rgb == blue_float);

	static constexpr TestType invalid = TestType::null();

	CONSTEXPR_CHECK(invalid == TestType());
	CONSTEXPR_CHECK(invalid.is_null());
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Operators", "[basic_colour_t][basic_colour_t-operators]", ColourTypes) {
	static constexpr TestType blue = TestType::from_floats(0.2, 0.2, 1);

	CONSTEXPR_CHECK(-blue == TestType::from_floats(0.8, 0.8, 0));
	CONSTEXPR_CHECK(blue.invert() == TestType::from_floats(0.8, 0.8, 0));
	CHECK(blue.contrast() == TestType::from_floats(0, 0, 0));
	CHECK(blue.invert().contrast() == TestType::from_floats(1, 1, 1));
	CONSTEXPR_CHECK(blue[0] == 51);
	CONSTEXPR_CHECK(blue[1] == 51);
	CONSTEXPR_CHECK(blue[2] == 255);
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Conversion methods", "[basic_colour_t][basic_colour_t-conversion]", ColourTypes) {
	static constexpr TestType cyan = TestType::from_floats(0, 1, 1);

	CONSTEXPR_CHECK(cyan.as_rgb() == 0x00FFFF);
	CONSTEXPR_CHECK(cyan.as_argb() == 0xFF00FFFF);
	CONSTEXPR_CHECK(cyan.as_rgba() == 0x00FFFFFF);
	CONSTEXPR_CHECK(cyan.redf() == 0);
	CONSTEXPR_CHECK(cyan.greenf() == 1);
	CONSTEXPR_CHECK(cyan.bluef() == 1);
	CONSTEXPR_CHECK(cyan.alphaf() == 1);
	CONSTEXPR_CHECK(cyan.to_hex_array(true) == "00FFFFFF"sv);
	CONSTEXPR_CHECK(cyan.to_hex_array(false) == "00FFFF"sv);
	CONSTEXPR_CHECK(cyan.to_argb_hex_array() == "FF00FFFF"sv);
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Parse methods", "[basic_colour_t][basic_colour_t-parse]", ColourTypes) {
	static constexpr std::string_view yellow_rgb = "FFFF00"sv;
	static constexpr std::string_view yellow_rgba = "FFFF00FF"sv;
	static constexpr std::string_view yellow_argb = "FFFFFF00"sv;
	static constexpr std::string_view hex_yellow_rgb = "0xFFFF00"sv;
	static constexpr std::string_view hex_yellow_rgba = "0xFFFF00FF"sv;
	static constexpr std::string_view hex_yellow_argb = "0xFFFFFF00"sv;
	static constexpr TestType yellow = TestType::from_floats(1, 1, 0);

	CONSTEXPR_CHECK(TestType::from_rgb_string(yellow_rgb) == yellow);
	CONSTEXPR_CHECK(TestType::from_rgba_string(yellow_rgba) == yellow);
	CONSTEXPR_CHECK(TestType::from_argb_string(yellow_argb) == yellow);
	CONSTEXPR_CHECK(TestType::from_rgb_string(hex_yellow_rgb) == yellow);
	CONSTEXPR_CHECK(TestType::from_rgba_string(hex_yellow_rgba) == yellow);
	CONSTEXPR_CHECK(TestType::from_argb_string(hex_yellow_argb) == yellow);
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Other methods", "[basic_colour_t][basic_colour_t-other]", ColourTypes) {
	static constexpr TestType blue = TestType::from_floats(0.2, 0.2, 1);

	CONSTEXPR_CHECK(blue.with_red(255) == TestType::from_floats(1, 0.2, 1));
	CONSTEXPR_CHECK(blue.with_green(255) == TestType::from_floats(0.2, 1, 1));
	CONSTEXPR_CHECK(blue.with_blue(0) == TestType::from_floats(0.2, 0.2, 0));
}

TEMPLATE_LIST_TEST_CASE("basic_colour_t Formatting", "[basic_colour_t][basic_colour_t-formatting]", ColourTypes) {
	static constexpr TestType blue = TestType::from_floats(0.2, 0.2, 1);

	CHECK(fmt::format("{:s}", blue) == "3333FF"sv);
	CHECK(fmt::format("{:a}", blue) == "FF3333FF"sv);
	CHECK(fmt::format("{:r}", blue) == "3333FFFF"sv);

	CHECK(fmt::format("{:Q<16r}", blue) == "3333FFFFQQQQQQQQ"sv);
	CHECK(fmt::format("{:Q^16r}", blue) == "QQQQ3333FFFFQQQQ"sv);
	CHECK(fmt::format("{:Q>16r}", blue) == "QQQQQQQQ3333FFFF"sv);

	int width = 16;
	CHECK(fmt::format("{:Q>{}r}", blue, width) == "QQQQQQQQ3333FFFF"sv);
	width -= 2;
	CHECK(fmt::format("{:Q>{}r}", blue, width) == "QQQQQQ3333FFFF"sv);
	width += 6;
	CHECK(fmt::format("{:Q>{}r}", blue, width) == "QQQQQQQQQQQQ3333FFFF"sv);
	CHECK(fmt::format("{1:Q>{0}r}", width, blue) == "QQQQQQQQQQQQ3333FFFF"sv);

	CHECK(fmt::format("{:vs}", blue) == "(51, 51, 255)"sv);
	CHECK(fmt::format("{:va}", blue) == "(255, 51, 51, 255)"sv);
	CHECK(fmt::format("{:vr}", blue) == "(51, 51, 255, 255)"sv);

	CHECK(fmt::format("{:nvs}", blue) == "51, 51, 255"sv);
	CHECK(fmt::format("{:nva}", blue) == "255, 51, 51, 255"sv);
	CHECK(fmt::format("{:nvr}", blue) == "51, 51, 255, 255"sv);

	CHECK(fmt::format("{:ns}", blue) == "51, 51, 255"sv);
	CHECK(fmt::format("{:na}", blue) == "255, 51, 51, 255"sv);
	CHECK(fmt::format("{:nr}", blue) == "51, 51, 255, 255"sv);

	CHECK(fmt::format("{:#s}", blue) == "#3333FF"sv);
	CHECK(fmt::format("{:xa}", blue) == "ff3333ff"sv);
	CHECK(fmt::format("{:Xr}", blue) == "3333FFFF"sv);
	CHECK(fmt::format("{:#xs}", blue) == "0x3333ff"sv);
	CHECK(fmt::format("{:#Xa}", blue) == "0XFF3333FF"sv);
}

TEST_CASE("colour_rgb_t Constructor methods", "[basic_colour_t][colour_rgb_t][colour_rgb_t-constructor]") {
	static constexpr colour_rgb_t blue_rgb = colour_rgb_t(
		63.0 / 255 * 255, //
		96.0 / 255 * 255, //
		1.0 * 255
	);
	static constexpr colour_rgb_t blue_int = colour_rgb_t::from_integer(0x3F'60'FF);
	static constexpr colour_rgb_t blue_rgba = colour_rgb_t::from_rgba(0x3F'60'FF'FF);
	static constexpr colour_rgb_t blue_argb = colour_rgb_t::from_argb(0xFF'3F'60'FF);
	static constexpr colour_rgb_t blue_udl = 0x3F'60'FF_rgb;

	CONSTEXPR_CHECK(blue_rgb == blue_int);
	CONSTEXPR_CHECK(blue_rgb == blue_rgba);
	CONSTEXPR_CHECK(blue_rgb == blue_argb);
	CONSTEXPR_CHECK(blue_rgb == blue_udl);

	static constexpr colour_rgb_t fill = colour_rgb_t::fill_as(100);

	CONSTEXPR_CHECK(fill == colour_rgb_t::from_floats(100.0 / 255, 100.0 / 255, 100.0 / 255));
}

TEST_CASE("colour_argb_t Constructor methods", "[basic_colour_t][colour_argb_t][colour_argb_t-constructor]") {
	static constexpr colour_argb_t blue_rgb = colour_argb_t(
		63.0 / 255 * 255, //
		96.0 / 255 * 255, //
		1.0 * 255
	);
	static constexpr colour_argb_t blue_int = colour_argb_t::from_integer(0x3F'60'FF'FF);
	static constexpr colour_argb_t blue_rgba = colour_argb_t::from_rgba(0x3F'60'FF'FF);
	static constexpr colour_argb_t blue_argb = colour_argb_t::from_argb(0xFF'3F'60'FF);
	static constexpr colour_argb_t blue_rgb2 = colour_argb_t::from_rgb(0x3F'60'FF);
	static constexpr colour_argb_t blue_udl = 0x3F'60'FF'FF_rgba;
	static constexpr colour_argb_t blue_argb_udl = 0xFF'3F'60'FF_argb;

	CONSTEXPR_CHECK(blue_rgb == blue_int);
	CONSTEXPR_CHECK(blue_rgb == blue_rgba);
	CONSTEXPR_CHECK(blue_rgb == blue_argb);
	CONSTEXPR_CHECK(blue_rgb == blue_rgb2);
	CONSTEXPR_CHECK(blue_rgb == blue_udl);
	CONSTEXPR_CHECK(blue_rgb == blue_argb_udl);

	static constexpr colour_argb_t cyan_rgb = colour_argb_t::from_floats(0, 1, 1);
	static constexpr colour_argb_t cyan_int = colour_argb_t::from_integer(0x00'FF'FF'FF);
	static constexpr colour_argb_t cyan_rgba = colour_argb_t::from_rgba(0x00'FF'FF'FF);
	static constexpr colour_argb_t cyan_argb = colour_argb_t::from_argb(0xFF'00'FF'FF);
	static constexpr colour_argb_t cyan_rgb2 = colour_argb_t::from_rgb(0x00'FF'FF);
	static constexpr colour_argb_t cyan_udl = 0x00'FF'FF'FF_rgba;
	static constexpr colour_argb_t cyan_argb_udl = 0xFF'00'FF'FF_argb;

	CONSTEXPR_CHECK(cyan_rgb == cyan_int);
	CONSTEXPR_CHECK(cyan_rgb == cyan_rgba);
	CONSTEXPR_CHECK(cyan_rgb == cyan_argb);
	CONSTEXPR_CHECK(cyan_rgb == cyan_rgb2);
	CONSTEXPR_CHECK(cyan_rgb == cyan_udl);
	CONSTEXPR_CHECK(cyan_rgb == cyan_argb_udl);

	static constexpr colour_argb_t fill = colour_argb_t::fill_as(100);

	CONSTEXPR_CHECK(fill == colour_argb_t::from_floats(100.0 / 255, 100.0 / 255, 100.0 / 255, 100.0 / 255));
}

TEST_CASE("colour_rgb_t Operators", "[basic_colour_t][colour_rgb_t][colour_rgb_t-operators]") {
	static constexpr colour_rgb_t blue = colour_rgb_t::from_floats(0.2, 0.2, 1);

	CONSTEXPR_CHECK(static_cast<uint32_t>(blue) == blue.as_rgb());
}

TEST_CASE("colour_rgb_t alpha access", "[basic_colour_t][colour_rgb_t][colour_rgb_t-alpha-access]") {
	static constexpr colour_rgb_t blue = colour_rgb_t::from_floats(0.2, 0.2, 1);

	CONSTEXPR_CHECK(blue.alpha == 255);
}

TEST_CASE("colour_rgb_t Parse methods", "[basic_colour_t][colour_rgb_t][colour_rgb_t-parse]") {
	static constexpr std::string_view yellow_rgb = "FFFF00"sv;
	static constexpr std::string_view hex_yellow_rgb = "FFFF00"sv;
	static constexpr colour_rgb_t yellow = colour_rgb_t::from_floats(1, 1, 0);

	CONSTEXPR_CHECK(colour_rgb_t::from_string(yellow_rgb) == yellow);
	CONSTEXPR_CHECK(colour_rgb_t::from_string(hex_yellow_rgb) == yellow);
}

TEST_CASE("colour_rgb_t Formatting", "[basic_colour_t][colour_rgb_t][colour_rgb_t-formatting]") {
	static constexpr colour_rgb_t blue = colour_rgb_t::from_floats(0.2, 0.2, 1);

	CHECK(fmt::format("{}", blue) == "3333FF"sv);
	CHECK(fmt::format("{:n}", blue) == "51, 51, 255"sv);
	CHECK(fmt::format("{::}", blue) == "(51, 51, 255)"sv);
	CHECK(fmt::format("{:i}", blue) == "(51, 51, 255)"sv);
	CHECK(fmt::format("{:f}", blue) == "(0.2, 0.2, 1)"sv);

	CHECK(fmt::format("{:t}", blue) == "3333FF"sv);
	CHECK(fmt::format("{:vt}", blue) == "(51, 51, 255)"sv);
	CHECK(fmt::format("{:nvt}", blue) == "51, 51, 255"sv);
	CHECK(fmt::format("{:nt}", blue) == "51, 51, 255"sv);

	CHECK(fmt::format("{:#}", blue) == "#3333FF"sv);
	CHECK(fmt::format("{:x}", blue) == "3333ff"sv);
	CHECK(fmt::format("{:X}", blue) == "3333FF"sv);
	CHECK(fmt::format("{:#x}", blue) == "0x3333ff"sv);
	CHECK(fmt::format("{:#X}", blue) == "0X3333FF"sv);
}

TEST_CASE("colour_argb_t Operators", "[basic_colour_t][colour_argb_t][colour_argb_t-operators]") {
	static constexpr colour_argb_t blue = colour_argb_t::from_floats(0.2, 0.2, 1);
	static constexpr colour_argb_t transparent = colour_argb_t::from_floats(0.2, 0.2, 1, 0);

	CONSTEXPR_CHECK(static_cast<uint32_t>(blue) == blue.as_rgba());
	CONSTEXPR_CHECK(blue[3] == 255);
	CONSTEXPR_CHECK(transparent[0] == 51);
	CONSTEXPR_CHECK(transparent[1] == 51);
	CONSTEXPR_CHECK(transparent[2] == 255);
	CONSTEXPR_CHECK(transparent[3] == 0);
}

TEST_CASE("colour_argb_t Parse methods", "[basic_colour_t][colour_argb_t][colour_argb_t-parse]") {
	static constexpr std::string_view yellow_rgba = "FFFF00FF"sv;
	static constexpr std::string_view hex_yellow_rgba = "0xFFFF00FF"sv;
	static constexpr colour_argb_t yellow = colour_argb_t::from_floats(1, 1, 0);

	CONSTEXPR_CHECK(colour_argb_t::from_string(yellow_rgba) == yellow);
	CONSTEXPR_CHECK(colour_argb_t::from_string(hex_yellow_rgba) == yellow);
}

TEST_CASE("colour_argb_t Other methods", "[basic_colour_t][colour_argb_t][colour_argb_t-other]") {
	static constexpr colour_argb_t blue = colour_argb_t::from_floats(0.2, 0.2, 1);

	CONSTEXPR_CHECK(blue.with_alpha(1) == colour_argb_t::from_floats(0.2, 0.2, 1, 1.0 / 255));
}

TEST_CASE("colour_argb_t Formatting", "[basic_colour_t][colour_argb_t][colour_argb_t-formatting]") {
	static constexpr colour_argb_t blue = colour_argb_t::from_floats(0.2, 0.2, 1);

	CHECK(fmt::format("{}", blue) == "3333FFFF"sv);
	CHECK(fmt::format("{:n}", blue) == "51, 51, 255, 255"sv);
	CHECK(fmt::format("{::}", blue) == "(51, 51, 255, 255)"sv);
	CHECK(fmt::format("{:i}", blue) == "(51, 51, 255, 255)"sv);
	CHECK(fmt::format("{:f}", blue) == "(0.2, 0.2, 1, 1)"sv);

	CHECK(fmt::format("{:t}", blue) == "3333FFFF"sv);
	CHECK(fmt::format("{:vt}", blue) == "(51, 51, 255, 255)"sv);
	CHECK(fmt::format("{:nvt}", blue) == "51, 51, 255, 255"sv);
	CHECK(fmt::format("{:nt}", blue) == "51, 51, 255, 255"sv);

	CHECK(fmt::format("{:#}", blue) == "#3333FFFF"sv);
	CHECK(fmt::format("{:x}", blue) == "3333ffff"sv);
	CHECK(fmt::format("{:X}", blue) == "3333FFFF"sv);
	CHECK(fmt::format("{:#x}", blue) == "0x3333ffff"sv);
	CHECK(fmt::format("{:#X}", blue) == "0X3333FFFF"sv);
}
