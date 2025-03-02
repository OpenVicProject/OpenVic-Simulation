#include "Date.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <limits>
#include <ostream>
#include <string_view>

#include <fmt/format.h>

#include <range/v3/algorithm/max_element.hpp>

using namespace OpenVic;

std::string Timespan::to_string() const {
	// Maximum number of digits + 1 for potential minus sign
	static constexpr size_t array_length = fmt::detail::count_digits(uint64_t(std::numeric_limits<day_t>::max())) + 1;

	std::array<char, array_length> str_array {};
	std::to_chars_result result = to_chars(str_array.data(), str_array.data() + str_array.size());
	if (result.ec != std::errc {}) {
		return {};
	}

	return { str_array.data(), result.ptr };
}

Timespan::operator std::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Timespan const& timespan) {
	return out << timespan.to_string();
}

std::string Date::to_string(bool pad_year, bool pad_month, bool pad_day) const {
	static constexpr size_t array_length = //
		fmt::detail::count_digits(uint64_t(std::numeric_limits<year_t>::max())) +
		fmt::detail::count_digits(uint64_t(MONTHS_IN_YEAR)) + fmt::detail::count_digits(uint64_t(MAX_DAYS_IN_MONTH)) + 4;

	std::array<char, array_length> str_array {};
	std::to_chars_result result = to_chars(str_array.data(), str_array.data() + str_array.size(), pad_year, pad_month, pad_day);
	if (result.ec != std::errc {}) {
		return {};
	}

	return { str_array.data(), result.ptr };
}

Date::operator std::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Date date) {
	static constexpr size_t array_length = //
		fmt::detail::count_digits(uint64_t(std::numeric_limits<Date::year_t>::max())) +
		fmt::detail::count_digits(uint64_t(Date::MONTHS_IN_YEAR)) +
		fmt::detail::count_digits(uint64_t(Date::MAX_DAYS_IN_MONTH)) + 4;

	std::array<char, array_length> str_array {};
	std::to_chars_result result = date.to_chars(str_array.data(), str_array.data() + str_array.size(), false, false, false);
	if (result.ec != std::errc {}) {
		return out;
	}

	std::string_view view { str_array.data(), result.ptr };
	return out << view;
}
