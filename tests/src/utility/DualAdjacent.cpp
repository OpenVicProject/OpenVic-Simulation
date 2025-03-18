#include <array>
#include <functional>

#include <range/v3/algorithm/equal.hpp>
#include <range/v3/view/drop.hpp>

#include "openvic-simulation/utility/Utility.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include <snitch/snitch_append.hpp>
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>

using namespace OpenVic;
using namespace OpenVic::utility;

namespace snitch {
	template<typename T, size_t Size>
	[[nodiscard]] inline static constexpr bool append( //
		snitch::small_string_span ss, std::array<T, Size> array
	) noexcept {
		if (array.empty()) {
			return append(ss, "{}");
		}
		bool result = append(ss, "{", array.front());
		for (const T& v : array | ranges::views::drop(1)) {
			result &= append(ss, ",", v);
		}
		return append(ss, "}") && result;
	}

	template<typename T>
	[[nodiscard]] inline static constexpr bool append( //
		snitch::small_string_span ss, std::vector<T> vector
	) noexcept {
		if (vector.empty()) {
			return append(ss, "{}");
		}
		bool result = append(ss, "{", vector.front());
		for (const T& v : vector | ranges::views::drop(1)) {
			result &= append(ss, ",", v);
		}
		return append(ss, "}") && result;
	}
}

TEST_CASE("find_if_dual_adjacent", "[utility][dual-adjacent][find_if_dual_adjacent]") {
	static constexpr auto array_up = std::to_array<int>({ 1, 2, 3, 4, 5, 6, 7, 8, 9 });
	static constexpr auto array_down = std::to_array<int>({ 9, 8, 7, 6, 5, 4, 3, 2, 1 });
	static constexpr auto array_spread = std::to_array<int>({ 5, 3, 4, 9, 7, 1, 8, 2, 6 });

	static constexpr auto callback_up = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 3 && cur == 4 && next == 5;
	};
	static constexpr auto callback_down = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 5 && cur == 4 && next == 3;
	};
	static constexpr auto callback_spread = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 4 && cur == 9 && next == 7;
	};
	static constexpr auto callback_none = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 9 && cur == 9 && next == 9;
	};
	static constexpr auto callback_bind = [](int const& compare, int const& prev, int const& cur, int const& next) -> bool {
		return cur == compare;
	};

	CONSTEXPR_CHECK(find_if_dual_adjacent(array_up.begin(), array_up.end(), callback_up) == array_up.begin() + 3);
	CONSTEXPR_CHECK(find_if_dual_adjacent(array_down.begin(), array_down.end(), callback_up) == array_down.end());
	CONSTEXPR_CHECK(find_if_dual_adjacent(array_spread.begin(), array_spread.end(), callback_up) == array_spread.end());

	CONSTEXPR_CHECK(find_if_dual_adjacent(array_up.begin(), array_up.end(), callback_down) == array_up.end());
	CONSTEXPR_CHECK(find_if_dual_adjacent(array_down.begin(), array_down.end(), callback_down) == array_down.begin() + 5);
	CONSTEXPR_CHECK(find_if_dual_adjacent(array_spread.begin(), array_spread.end(), callback_down) == array_spread.end());

	CONSTEXPR_CHECK(find_if_dual_adjacent(array_up.begin(), array_up.end(), callback_spread) == array_up.end());
	CONSTEXPR_CHECK(find_if_dual_adjacent(array_down.begin(), array_down.end(), callback_spread) == array_down.end());
	CONSTEXPR_CHECK(
		find_if_dual_adjacent(array_spread.begin(), array_spread.end(), callback_spread) == array_spread.begin() + 3
	);

	CONSTEXPR_CHECK(find_if_dual_adjacent(array_up.begin(), array_up.end(), callback_none) == array_up.end());
	CONSTEXPR_CHECK(find_if_dual_adjacent(array_down.begin(), array_down.end(), callback_none) == array_down.end());
	CONSTEXPR_CHECK(find_if_dual_adjacent(array_spread.begin(), array_spread.end(), callback_none) == array_spread.end());

	CONSTEXPR_CHECK(
		find_if_dual_adjacent(array_up.begin(), array_up.end(), std::bind_front(callback_bind, 1)) == array_up.end()
	);
	CONSTEXPR_CHECK(
		find_if_dual_adjacent(array_down.begin(), array_down.end(), std::bind_front(callback_bind, 9)) == array_down.end()
	);
	CONSTEXPR_CHECK(
		find_if_dual_adjacent(array_spread.begin(), array_spread.end(), std::bind_front(callback_bind, 5)) == array_spread.end()
	);

	CONSTEXPR_CHECK(
		find_if_dual_adjacent(array_up.begin(), array_up.end(), std::bind_front(callback_bind, 9)) == array_up.end()
	);
	CONSTEXPR_CHECK(
		find_if_dual_adjacent(array_down.begin(), array_down.end(), std::bind_front(callback_bind, 1)) == array_down.end()
	);
	CONSTEXPR_CHECK(
		find_if_dual_adjacent(array_spread.begin(), array_spread.end(), std::bind_front(callback_bind, 6)) == array_spread.end()
	);
}

TEST_CASE("remove_if_dual_adjacent", "[utility][dual-adjacent][remove_if_dual_adjacent]") {
	std::vector<int> vector_up { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	std::vector<int> vector_down { 9, 8, 7, 6, 5, 4, 3, 2, 1 };
	std::vector<int> vector_spread { 5, 3, 4, 9, 7, 1, 8, 2, 6 };

	static constexpr auto callback_up = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 3 && cur == 4 && next == 5;
	};
	static constexpr auto callback_down = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 5 && cur == 4 && next == 3;
	};
	static constexpr auto callback_spread = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 4 && cur == 9 && next == 7;
	};
	static constexpr auto callback_none = [](int const& prev, int const& cur, int const& next) -> bool {
		return prev == 9 && cur == 9 && next == 9;
	};
	static constexpr auto callback_bind = [](int const& compare, int const& prev, int const& cur, int const& next) -> bool {
		return cur == compare;
	};

	auto remove_up_it = remove_if_dual_adjacent(vector_up.begin(), vector_up.end(), callback_up);
	CHECK(remove_up_it == vector_up.begin() + 8);
	CHECK(remove_if_dual_adjacent(vector_down.begin(), vector_down.end(), callback_up) == vector_down.end());
	CHECK(remove_if_dual_adjacent(vector_spread.begin(), vector_spread.end(), callback_up) == vector_spread.end());

	auto remove_down_it = remove_if_dual_adjacent(vector_down.begin(), vector_down.end(), callback_down);
	CHECK(remove_if_dual_adjacent(vector_up.begin(), vector_up.end(), callback_down) == vector_up.end());
	CHECK(remove_down_it == vector_down.begin() + 8);
	CHECK(remove_if_dual_adjacent(vector_spread.begin(), vector_spread.end(), callback_down) == vector_spread.end());

	auto remove_spread_it = remove_if_dual_adjacent(vector_spread.begin(), vector_spread.end(), callback_spread);
	CHECK(remove_if_dual_adjacent(vector_up.begin(), vector_up.end(), callback_spread) == vector_up.end());
	CHECK(remove_if_dual_adjacent(vector_down.begin(), vector_down.end(), callback_spread) == vector_down.end());
	CHECK(remove_spread_it == vector_spread.begin() + 8);

	CHECK(remove_if_dual_adjacent(vector_up.begin(), vector_up.end(), callback_none) == vector_up.end());
	CHECK(remove_if_dual_adjacent(vector_down.begin(), vector_down.end(), callback_none) == vector_down.end());
	CHECK(remove_if_dual_adjacent(vector_spread.begin(), vector_spread.end(), callback_none) == vector_spread.end());

	CHECK( //
		remove_if_dual_adjacent(vector_up.begin(), vector_up.end(), std::bind_front(callback_bind, 1)) == vector_up.end()
	);
	CHECK(
		remove_if_dual_adjacent(vector_down.begin(), vector_down.end(), std::bind_front(callback_bind, 9)) == vector_down.end()
	);
	CHECK(
		remove_if_dual_adjacent(vector_spread.begin(), vector_spread.end(), std::bind_front(callback_bind, 5)) ==
		vector_spread.end()
	);

	CHECK( //
		remove_if_dual_adjacent(vector_up.begin(), vector_up.end(), std::bind_front(callback_bind, 9)) == vector_up.end() - 1
	);
	CHECK(
		remove_if_dual_adjacent(vector_down.begin(), vector_down.end(), std::bind_front(callback_bind, 1)) ==
		vector_down.end() - 1
	);
	CHECK(
		remove_if_dual_adjacent(vector_spread.begin(), vector_spread.end(), std::bind_front(callback_bind, 6)) ==
		vector_spread.end() - 1
	);

	static constexpr auto removed_vector_up = std::to_array<int>({ 1, 2, 3, 5, 6, 7, 8, 9, 9 });
	static constexpr auto removed_vector_down = std::to_array<int>({ 9, 8, 7, 6, 5, 3, 2, 1, 1 });
	static constexpr auto removed_vector_spread = std::to_array<int>({ 5, 3, 4, 7, 1, 8, 2, 6, 6 });

	{
		CAPTURE(vector_up);
		CAPTURE(removed_vector_up);
		CHECK(ranges::equal(vector_up, removed_vector_up));
	}
	{
		CAPTURE(vector_down);
		CAPTURE(removed_vector_down);
		CHECK(ranges::equal(vector_down, removed_vector_down));
	}
	{
		CAPTURE(vector_spread);
		CAPTURE(removed_vector_spread);
		CHECK(ranges::equal(vector_spread, removed_vector_spread));
	}

	auto erase_up_it = vector_up.erase(remove_up_it, vector_up.end());
	auto erase_down_it = vector_down.erase(remove_down_it, vector_down.end());
	auto erase_spread_it = vector_spread.erase(remove_spread_it, vector_spread.end());

	static constexpr auto erased_vector_up = std::to_array<int>({ 1, 2, 3, 5, 6, 7, 8, 9 });
	static constexpr auto erased_vector_down = std::to_array<int>({ 9, 8, 7, 6, 5, 3, 2, 1 });
	static constexpr auto erased_vector_spread = std::to_array<int>({ 5, 3, 4, 7, 1, 8, 2, 6 });

	{
		CAPTURE(vector_up);
		CAPTURE(erased_vector_up);
		CHECK(erase_up_it == vector_up.end());
		CHECK(ranges::equal(vector_up, erased_vector_up));
	}
	{
		CAPTURE(vector_down);
		CAPTURE(erased_vector_down);
		CHECK(erase_down_it == vector_down.end());
		CHECK(ranges::equal(vector_down, erased_vector_down));
	}
	{
		CAPTURE(vector_spread);
		CAPTURE(erased_vector_spread);
		CHECK(erase_spread_it == vector_spread.end());
		CHECK(ranges::equal(vector_spread, erased_vector_spread));
	}

	std::vector<int> vector_above { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	std::vector<int> vector_some { 5, 3, 4, 9, 7, 1, 8, 2, 6 };

	auto remove_above_it = remove_if_dual_adjacent(vector_above.begin(), vector_above.end(), [](auto&, const int& cur, auto&) {
		return cur >= 6;
	});
	CHECK(remove_above_it == vector_above.begin() + 6);

	auto remove_some_it = remove_if_dual_adjacent(vector_some.begin(), vector_some.end(), [](auto&, const int& cur, auto&) {
		return cur >= 5;
	});
	CHECK(remove_some_it == vector_some.begin() + 6);

	static constexpr auto removed_vector_above = std::to_array<int>({ 1, 2, 3, 4, 5, 9, 7, 8, 9 });
	static constexpr auto removed_vector_some = std::to_array<int>({ 5, 3, 4, 1, 2, 6, 8, 2, 6 });

	{
		CAPTURE(vector_above);
		CAPTURE(removed_vector_above);
		CHECK(ranges::equal(vector_above, removed_vector_above));
	}
	{
		CAPTURE(vector_some);
		CAPTURE(removed_vector_some);
		CHECK(ranges::equal(vector_some, removed_vector_some));
	}
}
