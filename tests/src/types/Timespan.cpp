#include <cmath>
#include <cstdlib>
#include <string_view>

#include "openvic-simulation/types/Date.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

namespace snitch {
	inline static bool append(snitch::small_string_span ss, OpenVic::Timespan const& s) {
		return append(ss, s.to_int());
	}
}

using namespace OpenVic;
using namespace std::string_view_literals;

TEST_CASE("Timespan Constructor method", "[Timespan][Timespan-constructor]") {
	const Timespan empty = Timespan {};
	const Timespan zero = Timespan { 0 };

	CHECK(empty == zero);

	const Timespan _30 = Timespan { 30 };

	CHECK(_30 == 30);

	const Timespan _days = Timespan::from_days(5);
	const Timespan _months = Timespan::from_months(20);
	const Timespan _years = Timespan::from_years(4);

	CHECK(_days == 5);
	CHECK(_months == 608);
	CHECK(_years == 365 * 4);
}

TEST_CASE("Timespan Operators", "[Timespan][Timespan-operators]") {
	const Timespan timespan1 = { 20 };
	const Timespan timespan2 = { 35 };

	CHECK((timespan1 + timespan2) == Timespan { 55 });
	CHECK((timespan2 - timespan1) == Timespan { 15 });
	CHECK((timespan1 * 3) == Timespan { 60 });
	CHECK((timespan1 / 5) == Timespan { 4 });

	Timespan Timespan_move = timespan1;
	CHECK(Timespan_move++ == Timespan { 20 });
	CHECK(Timespan_move-- == Timespan { 21 });
	CHECK(++Timespan_move == Timespan { 21 });
	CHECK(--Timespan_move == Timespan { 20 });
	CHECK((Timespan_move += 5) == Timespan { 25 });
	CHECK((Timespan_move -= 5) == Timespan { 20 });

	CHECK(timespan1 < timespan2);
	CHECK(timespan2 > timespan1);
	CHECK(timespan1 != timespan2);
}

TEST_CASE("Timespan Conversion methods", "[Timespan][Timespan-conversion]") {
	const Timespan timespan1 = 10;

	CHECK(timespan1.to_int() == 10);
	CHECK(timespan1.to_string() == "10"sv);
}
