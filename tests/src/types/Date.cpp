#include "openvic-simulation/types/Date.hpp"

#include <cmath>
#include <cstdlib>
#include <string_view>

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

namespace snitch {
	inline static bool append(snitch::small_string_span ss, OpenVic::Date const& s) {
		return append(ss, s.to_string(true));
	}
}

using namespace OpenVic;
using namespace std::string_view_literals;

TEST_CASE("Date Constructor method", "[Date][Date-constructor]") {
	const Date empty = Date {};
	const Date zero = Date { 0, 1, 1 };
	const Date zero_timespan = Date { Timespan { 0 } };

	CHECK(empty == zero);
	CHECK(empty == zero_timespan);

	const Date _0_1_31 = Date { 0, 1, 31 };
	const Date _timespan_0_1_31 = Date { Timespan { 30 } };

	CHECK(_0_1_31 == _timespan_0_1_31);
}

TEST_CASE("Date Operators", "[Date][Date-operators]") {
	const Date date1 = { 0, 5, 20 };
	const Date date2 = { 40, 10, 5 };

	CHECK((Date {} + (date2 - date1)) == Date { 40, 5, 19 });

	Date date_move = date1;
	CHECK(date_move++ == Date { 0, 5, 20 });
	CHECK(date_move-- == Date { 0, 5, 21 });
	CHECK(++date_move == Date { 0, 5, 21 });
	CHECK(--date_move == Date { 0, 5, 20 });
	CHECK((date_move += 5) == Date { 0, 5, 25 });
	CHECK((date_move -= 5) == Date { 0, 5, 20 });

	CHECK(date1 < date2);
	CHECK(date2 > date1);
	CHECK(date1 != date2);
}

TEST_CASE("Date Conversion methods", "[Date][Date-conversion]") {
	const Date date = { 5, 4, 10 };
	const Date date2 = { 5, 3, 2 };

	CHECK(date.get_year() == 5);
	CHECK(date.get_month() == 4);
	CHECK(date.get_month_name() == "April"sv);
	CHECK(date.get_day() == 10);

	CHECK(date.to_string() == "5.04.10");
	CHECK(date.to_string(true) == "0005.04.10");
	CHECK(date.to_string(false, false) == "5.4.10");
	CHECK(date.to_string(true, false) == "0005.4.10");

	CHECK(date2.to_string() == "5.03.02");
	CHECK(date2.to_string(true) == "0005.03.02");
	CHECK(date2.to_string(true, false, false) == "0005.3.2");
	CHECK(date2.to_string(true, true, false) == "0005.03.2");
	CHECK(date2.to_string(true, false, true) == "0005.3.02");
	CHECK(date2.to_string(false, false, false) == "5.3.2");
	CHECK(date2.to_string(false, false, true) == "5.3.02");
}

TEST_CASE("Date Parse methods", "[Date][Date-parse]") {
	CHECK(Date::from_string("1.2.3") == Date { 1, 2, 3 });
	CHECK(Date::from_string("01.2.3") == Date { 1, 2, 3 });
	CHECK(Date::from_string("1.02.03") == Date { 1, 2, 3 });
	CHECK(Date::from_string("001.2.3") == Date { 1, 2, 3 });
	CHECK(Date::from_string("001.02.3") == Date { 1, 2, 3 });
	CHECK(Date::from_string("001.2.03") == Date { 1, 2, 3 });
	CHECK(Date::from_string("001.02.03") == Date { 1, 2, 3 });
	CHECK(Date::from_string("0001.02.3") == Date { 1, 2, 3 });
	CHECK(Date::from_string("0001.2.03") == Date { 1, 2, 3 });
	CHECK(Date::from_string("0001.02.03") == Date { 1, 2, 3 });
	CHECK(Date::from_string("20.6.13") == Date { 20, 6, 13 });
	CHECK(Date::from_string("0020.06.13") == Date { 20, 6, 13 });
	CHECK(Date::from_string("1815.8.20") == Date { 1815, 8, 20 });

	Date::from_chars_result result;

	static constexpr std::string_view empty = ""sv;
	CHECK(Date::from_string(empty, &result) == Date {});
	CHECK(result.type == Date::errc_type::year);
	CHECK(result.type_first == empty.data());
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view dot = "."sv;
	CHECK(Date::from_string(dot, &result) == Date {});
	CHECK(result.type == Date::errc_type::year);
	CHECK(result.type_first == dot.data());
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view year_only = "2."sv;
	CHECK(Date::from_string(year_only, &result) == Date { 2 });
	CHECK(result.type == Date::errc_type::month);
	CHECK(result.type_first == year_only.data() + year_only.size());
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view year_sep_only = "2.."sv;
	CHECK(Date::from_string(year_sep_only, &result) == Date { 2 });
	CHECK(result.type == Date::errc_type::month);
	CHECK(result.type_first == year_sep_only.data() + year_sep_only.size() - 1);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view year_sep_month = "2..4"sv;
	CHECK(Date::from_string(year_sep_month, &result) == Date { 2 });
	CHECK(result.type == Date::errc_type::month);
	CHECK(result.type_first == year_sep_month.data() + year_sep_month.size() - 2);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view year_month = "2.4"sv;
	CHECK(Date::from_string(year_month, &result) == Date { 2, 4 });
	CHECK(result.type == Date::errc_type::month);
	CHECK(result.type_first == year_month.data() + 2);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::result_out_of_range);

	static constexpr std::string_view year_month_sep = "2.4."sv;
	CHECK(Date::from_string(year_month_sep, &result) == Date { 2, 4 });
	CHECK(result.type == Date::errc_type::day);
	CHECK(result.type_first == year_month_sep.data() + 4);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view year_month_sep_sep = "2.4.."sv;
	CHECK(Date::from_string(year_month_sep_sep, &result) == Date { 2, 4 });
	CHECK(result.type == Date::errc_type::day);
	CHECK(result.type_first == year_month_sep_sep.data() + 4);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view negative_year = "-1"sv;
	CHECK(Date::from_string(negative_year, &result) == Date {});
	CHECK(result.type == Date::errc_type::year);
	CHECK(result.type_first == negative_year.data());
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::not_supported);

	static constexpr std::string_view zero_month = "0.0"sv;
	CHECK(Date::from_string(zero_month, &result) == Date {});
	CHECK(result.type == Date::errc_type::month);
	CHECK(result.type_first == zero_month.data() + 2);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::not_supported);

	static constexpr std::string_view negative_month = "0.-1"sv;
	CHECK(Date::from_string(negative_month, &result) == Date {});
	CHECK(result.type == Date::errc_type::month);
	CHECK(result.type_first == negative_month.data() + 2);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view zero_day = "0.1.0"sv;
	CHECK(Date::from_string(zero_day, &result) == Date {});
	CHECK(result.type == Date::errc_type::day);
	CHECK(result.type_first == zero_day.data() + 4);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::not_supported);

	static constexpr std::string_view negative_day = "0.1.-1"sv;
	CHECK(Date::from_string(negative_day, &result) == Date {});
	CHECK(result.type == Date::errc_type::day);
	CHECK(result.type_first == negative_day.data() + 4);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::invalid_argument);

	static constexpr std::string_view thirteenth_month = "0.13"sv;
	CHECK(Date::from_string(thirteenth_month, &result) == Date {});
	CHECK(result.type == Date::errc_type::month);
	CHECK(result.type_first == thirteenth_month.data() + 2);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::value_too_large);

	static constexpr std::string_view invalid_dec_day = "0.12.32"sv;
	CHECK(Date::from_string(invalid_dec_day, &result) == Date { 0, 12 });
	CHECK(result.type == Date::errc_type::day);
	CHECK(result.type_first == invalid_dec_day.data() + 5);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::value_too_large);

	static constexpr std::string_view invalid_feb_day = "0.2.29"sv;
	CHECK(Date::from_string(invalid_feb_day, &result) == Date { 0, 2 });
	CHECK(result.type == Date::errc_type::day);
	CHECK(result.type_first == invalid_feb_day.data() + 4);
	CHECK(result.ptr == result.type_first);
	CHECK(result.ec == std::errc::value_too_large);
}

TEST_CASE("Date Other methods", "[Date][Date-other]") {
	const Date start = { 5, 3, 2 };
	const Date end = { 5, 4, 10 };
	const Date inside = { 5, 3, 5 };
	const Date outside = { 10, 4, 2 };

	CHECK(inside.in_range(start, end));
	CHECK_FALSE(outside.in_range(start, end));
	CHECK_FALSE(start.is_month_start());
	CHECK(Date { 532, 6, 1 }.is_month_start());
}
