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

inline Date::stack_string Date::to_array(bool pad_year, bool pad_month, bool pad_day) const {
	stack_string str {};
	std::to_chars_result result = to_chars(str.array.data(), str.array.data() + str.array.size(), pad_year, pad_month, pad_day);
	str.string_size = result.ptr - str.data();
	return str;
}

std::string Date::to_string(bool pad_year, bool pad_month, bool pad_day) const {
	stack_string result = to_array(pad_year, pad_month, pad_day);
	if (OV_unlikely(result.empty())) {
		return {};
	}

	return result;
}

Date::operator std::string() const {
	return to_string();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Date date) {
	Date::stack_string result = date.to_array(false, false, false);
	if (OV_unlikely(result.empty())) {
		return out;
	}

	return out << static_cast<std::string_view>(result);
}

auto fmt::formatter<Date>::format(Date d, format_context& ctx) const -> format_context::iterator {
	Date::stack_string result = d.to_array();
	if (OV_unlikely(result.empty())) {
		return formatter<string_view>::format(string_view {}, ctx);
	}

	return formatter<string_view>::format(string_view { result.data(), result.size() }, ctx);
}
