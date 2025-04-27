#include <cmath>
#include <cstdlib>
#include <string_view>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/utility/Marshal.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

namespace snitch {
	[[nodiscard]] inline static constexpr bool append(snitch::small_string_span ss, OpenVic::Timespan const& s) {
		return append(ss, s.to_int());
	}
}

using namespace OpenVic;
using namespace std::string_view_literals;

TEST_CASE("Timespan Constructor method", "[Timespan][Timespan-constructor]") {
	static constexpr Timespan empty = Timespan {};
	static constexpr Timespan zero = Timespan { 0 };

	CONSTEXPR_CHECK(empty == zero);

	static constexpr Timespan _30 = Timespan { 30 };

	CONSTEXPR_CHECK(_30 == 30);

	static constexpr Timespan _days = Timespan::from_days(5);
	static constexpr Timespan _months = Timespan::from_months(20);
	static constexpr Timespan _years = Timespan::from_years(4);

	CONSTEXPR_CHECK(_days == 5);
	CONSTEXPR_CHECK(_months == 608);
	CONSTEXPR_CHECK(_years == 365 * 4);
}

TEST_CASE("Timespan Operators", "[Timespan][Timespan-operators]") {
	static constexpr Timespan timespan1 = { 20 };
	static constexpr Timespan timespan2 = { 35 };

	CONSTEXPR_CHECK((timespan1 + timespan2) == Timespan { 55 });
	CONSTEXPR_CHECK((timespan2 - timespan1) == Timespan { 15 });
	CONSTEXPR_CHECK((timespan1 * 3) == Timespan { 60 });
	CONSTEXPR_CHECK((timespan1 / 5) == Timespan { 4 });

	Timespan Timespan_move = timespan1;
	CHECK(Timespan_move++ == Timespan { 20 });
	CHECK(Timespan_move-- == Timespan { 21 });
	CHECK(++Timespan_move == Timespan { 21 });
	CHECK(--Timespan_move == Timespan { 20 });
	CHECK((Timespan_move += 5) == Timespan { 25 });
	CHECK((Timespan_move -= 5) == Timespan { 20 });

	CONSTEXPR_CHECK(timespan1 < timespan2);
	CONSTEXPR_CHECK(timespan2 > timespan1);
	CONSTEXPR_CHECK(timespan1 != timespan2);
}

TEST_CASE("Timespan Conversion methods", "[Timespan][Timespan-conversion]") {
	static constexpr Timespan timespan1 = 10;

	CONSTEXPR_CHECK(timespan1.to_int() == 10);
	CHECK(timespan1.to_string() == "10"sv);
}

TEST_CASE(
	"Timespan Marshal encode then decode",
	"[Timespan][Timespan-encode-decode][utility][Marshal][Marshal-encode-decode-Timespan]"
) {
	static constexpr Timespan timespan1 = 10;

	std::array<uint8_t, sizeof(timespan1)> buffer; // NOLINT
	utility::encode(timespan1, buffer);
	size_t decode_count;
	CHECK(timespan1 == utility::decode<Timespan>(buffer, decode_count));
}
