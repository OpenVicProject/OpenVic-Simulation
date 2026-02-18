#include "openvic-simulation/core/string/StringLiteral.hpp"

#include <cstddef>
#include <iterator>
#include <string_view>

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
using namespace std::string_view_literals;

namespace snitch {
	template<std::size_t N, typename CharT, class Traits>
	[[nodiscard]] inline static constexpr bool append( //
		snitch::small_string_span ss, OpenVic::string_literal<N, CharT, Traits> const& s
	) noexcept {
		return append(ss, s.as_string_view());
	}
}

TEST_CASE("string_literal Constructor methods", "[string_literal][string_literal-constructor]") {
	static constexpr string_literal empty {};
	CONSTEXPR_CHECK(empty.empty());

	static constexpr string_literal char_value = 'c';
	CONSTEXPR_CHECK(char_value.size() == 1);
	CONSTEXPR_CHECK(char_value[0] == 'c');

	static constexpr string_literal value = "value";
	CONSTEXPR_CHECK(value.size() == 5);
	CONSTEXPR_CHECK(value[0] == 'v');
	CONSTEXPR_CHECK(value[1] == 'a');
	CONSTEXPR_CHECK(value[2] == 'l');
	CONSTEXPR_CHECK(value[3] == 'u');
	CONSTEXPR_CHECK(value[4] == 'e');
}

TEST_CASE("string_literal Operators", "[string_literal][string_literal-operators]") {
	static constexpr string_literal check1 = "check1";
	CONSTEXPR_CHECK(check1[0] == 'c');
	CONSTEXPR_CHECK(check1[1] == 'h');
	CONSTEXPR_CHECK(check1[2] == 'e');
	CONSTEXPR_CHECK(check1[3] == 'c');
	CONSTEXPR_CHECK(check1[4] == 'k');
	CONSTEXPR_CHECK(check1[5] == '1');
	CONSTEXPR_CHECK(check1 == check1);

	static constexpr string_literal check2 = "check2";
	CONSTEXPR_CHECK(check1 != check2);
	CONSTEXPR_CHECK(static_cast<std::string_view>(check2) == "check2"sv);
	CONSTEXPR_CHECK(static_cast<char const*>(check2) == "check2"sv);
	CONSTEXPR_CHECK((check1 + check2) == "check1check2"sv);
	CONSTEXPR_CHECK((check1 + "check2") == "check1check2"sv);
	CONSTEXPR_CHECK(("check1" + check2) == "check1check2"sv);
}

TEST_CASE("string_literal Iterator methods", "[string_literal][string_literal-iterator]") {
	static constexpr string_literal iterator = "iterator";
	CONSTEXPR_CHECK(*iterator.begin() == 'i');
	CONSTEXPR_CHECK(*(iterator.begin() + 1) == 't');
	CONSTEXPR_CHECK(*(iterator.begin() + 2) == 'e');
	CONSTEXPR_CHECK(*(iterator.begin() + 3) == 'r');
	CONSTEXPR_CHECK(*(iterator.begin() + 4) == 'a');
	CONSTEXPR_CHECK(*(iterator.begin() + 5) == 't');
	CONSTEXPR_CHECK(*(iterator.begin() + 6) == 'o');
	CONSTEXPR_CHECK(*(iterator.begin() + 7) == 'r');
	CONSTEXPR_CHECK((iterator.begin() + 8) == iterator.end());

	CONSTEXPR_CHECK(*iterator.rbegin() == 'r');
	CONSTEXPR_CHECK(*(iterator.rbegin() + 1) == 'o');
	CONSTEXPR_CHECK(*(iterator.rbegin() + 2) == 't');
	CONSTEXPR_CHECK(*(iterator.rbegin() + 3) == 'a');
	CONSTEXPR_CHECK(*(iterator.rbegin() + 4) == 'r');
	CONSTEXPR_CHECK(*(iterator.rbegin() + 5) == 'e');
	CONSTEXPR_CHECK(*(iterator.rbegin() + 6) == 't');
	CONSTEXPR_CHECK(*(iterator.rbegin() + 7) == 'i');
	CONSTEXPR_CHECK((iterator.rbegin() + 8) == iterator.rend());

	CONSTEXPR_CHECK(std::reverse_iterator { iterator.begin() } == iterator.rend());
	CONSTEXPR_CHECK(std::reverse_iterator { iterator.end() } == iterator.rbegin());
	CONSTEXPR_CHECK(iterator.cbegin() == iterator.begin());
	CONSTEXPR_CHECK(iterator.cend() == iterator.end());
	CONSTEXPR_CHECK(iterator.crbegin() == iterator.rbegin());
	CONSTEXPR_CHECK(iterator.crend() == iterator.rend());
}

TEST_CASE("string_literal Access methods", "[string_literal][string_literal-access]") {
	static constexpr string_literal access = "access";
	CONSTEXPR_CHECK(access.at(0) == 'a');
	CONSTEXPR_CHECK(access.at(1) == 'c');
	CONSTEXPR_CHECK(access.at(2) == 'c');
	CONSTEXPR_CHECK(access.at(3) == 'e');
	CONSTEXPR_CHECK(access.at(4) == 's');
	CONSTEXPR_CHECK(access.at(5) == 's');

	CONSTEXPR_CHECK(access.front() == 'a');
	CONSTEXPR_CHECK(access.back() == 's');

	CONSTEXPR_CHECK(access.data()[0] == 'a');
	CONSTEXPR_CHECK(access.data()[1] == 'c');
	CONSTEXPR_CHECK(access.data()[2] == 'c');
	CONSTEXPR_CHECK(access.data()[3] == 'e');
	CONSTEXPR_CHECK(access.data()[4] == 's');
	CONSTEXPR_CHECK(access.data()[5] == 's');

	CONSTEXPR_CHECK(access.c_str() == "access"sv);
	CONSTEXPR_CHECK(access.c_str()[6] == '\0');
}

TEST_CASE("string_literal Inspect methods", "[string_literal][string_literal-Inspect]") {
	static constexpr string_literal inspect = "inspect";
	CONSTEXPR_CHECK(inspect.compare("inspect"sv) == 0);
	CONSTEXPR_CHECK(inspect.compare("inspect") == 0);
	CONSTEXPR_CHECK(inspect.compare("notinspect"sv) < 0);
	CONSTEXPR_CHECK(inspect.compare("notinspect") < 0);
	CONSTEXPR_CHECK(inspect.compare("aotinspect"sv) > 0);
	CONSTEXPR_CHECK(inspect.compare("aotinspect") > 0);

	CONSTEXPR_CHECK(inspect.starts_with("in"sv));
	CONSTEXPR_CHECK(inspect.starts_with('i'));
	CONSTEXPR_CHECK(inspect.starts_with("in"));
	CONSTEXPR_CHECK_FALSE(inspect.starts_with("et"sv));
	CONSTEXPR_CHECK_FALSE(inspect.starts_with('b'));
	CONSTEXPR_CHECK_FALSE(inspect.starts_with("da"));

	static constexpr string_literal in = "in";
	CONSTEXPR_CHECK(inspect.starts_with(in));

	static constexpr string_literal not_inspect = "not inspect";
	CONSTEXPR_CHECK_FALSE(inspect.starts_with(not_inspect));

	CONSTEXPR_CHECK(inspect.ends_with("ect"sv)); // spellchecker:disable-line
	CONSTEXPR_CHECK(inspect.ends_with('t'));
	CONSTEXPR_CHECK(inspect.ends_with("pect"));
	CONSTEXPR_CHECK_FALSE(inspect.ends_with("nb"sv));
	CONSTEXPR_CHECK_FALSE(inspect.ends_with('q'));
	CONSTEXPR_CHECK_FALSE(inspect.ends_with("rp"));

	static constexpr string_literal ct = "ct";
	CONSTEXPR_CHECK(inspect.ends_with(ct));

	CONSTEXPR_CHECK_FALSE(inspect.ends_with(not_inspect));

	CONSTEXPR_CHECK(inspect.find("ec") == 4);
	CONSTEXPR_CHECK(inspect.find("ec"sv) == 4);
	CONSTEXPR_CHECK(inspect.find("ec", 3) == 4);
	CONSTEXPR_CHECK(inspect.find("ec"sv, 2) == 4);
	CONSTEXPR_CHECK(inspect.find("non") == inspect.npos);
	CONSTEXPR_CHECK(inspect.find("non"sv) == inspect.npos);

	CONSTEXPR_CHECK(inspect.rfind("ec") == 4);
	CONSTEXPR_CHECK(inspect.rfind("ec"sv) == 4);
	CONSTEXPR_CHECK(inspect.rfind("ns", 1) == 1);
	CONSTEXPR_CHECK(inspect.rfind("ns"sv, 2) == 1);
	CONSTEXPR_CHECK(inspect.rfind("non") == inspect.npos);
	CONSTEXPR_CHECK(inspect.rfind("non"sv) == inspect.npos);
}

TEST_CASE("string_literal Modifier methods", "[string_literal][string_literal-modifier]") {
	static constexpr string_literal modifier = "modifier";
	CONSTEXPR_CHECK(modifier.clear().empty());
	CONSTEXPR_CHECK(modifier.push_back('a') == "modifiera"sv);
	CONSTEXPR_CHECK(modifier.pop_back() == "modifie"sv);
	CONSTEXPR_CHECK(modifier.append<3>('b') == "modifierbbb"sv);
	CONSTEXPR_CHECK(modifier.append("hey") == "modifierhey"sv);
	CONSTEXPR_CHECK(modifier.append(modifier) == "modifiermodifier"sv);
	CONSTEXPR_CHECK(modifier.substr<1, 3>() == "odi"sv);
	CONSTEXPR_CHECK(modifier.substr<1>() == "odifier"sv);
	CONSTEXPR_CHECK(modifier.substr() == modifier);
	CONSTEXPR_CHECK(modifier.substr(2, 3) == "dif"sv);
	CONSTEXPR_CHECK(modifier.substr(3) == "ifier"sv);
}
