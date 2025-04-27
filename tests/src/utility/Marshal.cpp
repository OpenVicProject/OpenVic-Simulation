#include "openvic-simulation/utility/Marshal.hpp"

#include <array>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>

#include <range/v3/algorithm/equal.hpp>
#include <range/v3/view/drop.hpp>

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_append.hpp>
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>

using namespace OpenVic;
using namespace OpenVic::utility;

using FundamentalTypes =
	snitch::type_list<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double>;

TEMPLATE_LIST_TEST_CASE(
	"Marshal encode then decode fundamental", "[utility][Marshal][Marshal-encode-decode-fundamental]", FundamentalTypes
) {
	std::array<uint8_t, sizeof(TestType)> buffer; // NOLINT
	size_t decode_count;

	SECTION("min") {
		static constexpr TestType min = std::numeric_limits<TestType>::min();
		CHECK(utility::encode(min, buffer) == buffer.size());
		CHECK(min == utility::decode<TestType>(buffer, decode_count));
	}

	SECTION("max") {
		static constexpr TestType max = std::numeric_limits<TestType>::max();
		CHECK(utility::encode(max, buffer) == buffer.size());
		CHECK(max == utility::decode<TestType>(buffer, decode_count));
	}

	SECTION("divided value") {
		static constexpr TestType div = TestType(11) / 2;
		CHECK(utility::encode(div, buffer) == buffer.size());
		CHECK(div == utility::decode<TestType>(buffer, decode_count));
	}

	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode half_float", "[utility][Marshal][Marshal-encode-decode-half_float]") {
	std::array<uint8_t, sizeof(half_float) / 2> buffer; // NOLINT
	size_t decode_count;

	SECTION("min") {
		static constexpr half_float min = { 65504 };
		CHECK(utility::encode(min, buffer) == buffer.size());
		half_float hf = utility::decode<half_float>(buffer, decode_count);
		CHECK(min.value == hf.value);
	}

	SECTION("max") {
		static constexpr half_float max = { -65504 };
		CHECK(utility::encode(max, buffer) == buffer.size());
		half_float hf = utility::decode<half_float>(buffer, decode_count);
		CHECK(max.value == hf.value);
	}

	SECTION("divided value") {
		static constexpr half_float div = { float(11) / 2 };
		CHECK(utility::encode(div, buffer) == buffer.size());
		half_float hf = utility::decode<half_float>(buffer, decode_count);
		CHECK(div.value == hf.value);
	}

	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode bool", "[utility][Marshal][Marshal-encode-decode-bool]") {
	std::array<uint8_t, sizeof(bool)> buffer; // NOLINT
	size_t decode_count;

	CHECK(utility::encode(true, buffer) == buffer.size());
	bool value = utility::decode<bool>(buffer, decode_count);
	CHECK(true == value);
	CHECK(decode_count == buffer.size());

	CHECK(utility::encode(false, buffer) == buffer.size());
	value = utility::decode<bool>(buffer, decode_count);
	CHECK(false == value);
	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode enum", "[utility][Marshal][Marshal-encode-decode-enum]") {
	enum TestEnum { First, Second, Third, Fifth = 5 };

	std::array<uint8_t, sizeof(TestEnum)> buffer; // NOLINT
	size_t decode_count;
	CHECK(utility::encode(TestEnum::First, buffer) == buffer.size());
	TestEnum value = utility::decode<TestEnum>(buffer, decode_count);
	CHECK(TestEnum::First == value);

	CHECK(decode_count == buffer.size());
	CHECK(utility::encode(TestEnum::Fifth, buffer) == buffer.size());
	value = utility::decode<TestEnum>(buffer, decode_count);
	CHECK(TestEnum::Fifth == value);
	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode string_view", "[utility][Marshal][Marshal-encode-decode-string_view]") {
	static constexpr std::string_view view = "a test string_view string";

	std::array<uint8_t, view.size() + sizeof(uint32_t)> buffer; // NOLINT
	size_t decode_count;

	CHECK(utility::encode(view, buffer) == buffer.size());
	CHECK(view == utility::decode<std::string_view>(buffer, decode_count));
	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode array", "[utility][Marshal][Marshal-encode-decode-array]") {
	static constexpr std::array array = std::to_array<uint8_t>({ 1, 2, 3, 4, 100, 50, 20 });

	std::array<uint8_t, array.size() + sizeof(uint32_t)> buffer; // NOLINT
	size_t decode_count;

	CHECK(utility::encode(array, buffer) == buffer.size());
	std::span<const uint8_t> span = utility::decode<std::span<uint8_t>>(buffer, decode_count);
	CHECK(ranges::equal(array, span));
	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode pair", "[utility][Marshal][Marshal-encode-decode-pair]") {
	static constexpr std::pair<int, std::string_view> pair = { 5, "this is a second pair value" };

	std::array<uint8_t, sizeof(int) + pair.second.size() + sizeof(uint32_t)> buffer; // NOLINT
	size_t decode_count;

	CHECK(utility::encode(pair, buffer) == buffer.size());
	decltype(pair) decoded = utility::decode<decltype(pair)>(buffer, decode_count);
	CHECK(std::get<0>(pair) == std::get<0>(decoded));
	CHECK(std::get<1>(pair) == std::get<1>(decoded));
	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode tuple", "[utility][Marshal][Marshal-encode-decode-tuple]") {
	static constexpr std::array<uint8_t, 2> array = { 3, 2 };
	static constexpr std::tuple<int, std::string_view, std::span<const uint8_t>> tuple = //
		{ //
		  5, "this is a second tuple value", array
		};

	std::array< // NOLINT
		uint8_t, sizeof(int) + std::get<1>(tuple).size() + sizeof(uint32_t) + std::get<2>(tuple).size() + sizeof(uint32_t)>
		buffer;
	size_t decode_count;

	CHECK(utility::encode(tuple, buffer) == buffer.size());
	decltype(tuple) decoded = utility::decode<decltype(tuple)>(buffer, decode_count);
	CHECK(std::get<0>(tuple) == std::get<0>(decoded));
	CHECK(std::get<1>(tuple) == std::get<1>(decoded));
	CHECK(ranges::equal(std::get<2>(tuple), std::get<2>(decoded)));
	CHECK(decode_count == buffer.size());
}

TEST_CASE("Marshal encode then decode variant", "[utility][Marshal][Marshal-encode-decode-variant]") {
	static constexpr std::string_view string = "variant string";

	std::variant<int, std::string_view, std::array<uint8_t, 2>> variant = 1;

	std::array<uint8_t, sizeof(uint32_t) + sizeof(uint32_t) + string.size()> buffer; // NOLINT
	size_t decode_count;

	CHECK(utility::encode(variant, buffer) == sizeof(int) + sizeof(uint32_t));
	decltype(variant) result = utility::decode<decltype(result)>(buffer, decode_count);
	CHECK(variant.index() == result.index());
	CHECK(variant == result);
	CHECK(decode_count == sizeof(int) + sizeof(uint32_t));

	variant = string;
	CHECK(utility::encode(variant, buffer) == string.size() + sizeof(uint32_t) + sizeof(uint32_t));
	result = utility::decode<decltype(result)>(buffer, decode_count);
	CHECK(variant.index() == result.index());
	CHECK(variant == result);
	CHECK(decode_count == string.size() + sizeof(uint32_t) + sizeof(uint32_t));

	variant = std::to_array<uint8_t>({ 3, 7 });
	CHECK(utility::encode(variant, buffer) == 2 + sizeof(uint32_t) + sizeof(uint32_t));
	result = utility::decode<decltype(result)>(buffer, decode_count);
	CHECK(variant.index() == result.index());
	CHECK(variant == result);
	CHECK(decode_count == 2 + sizeof(uint32_t) + sizeof(uint32_t));
}
