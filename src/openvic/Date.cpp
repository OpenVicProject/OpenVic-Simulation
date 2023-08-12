#include "Date.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>

#include "utility/Logger.hpp"

using namespace OpenVic;

Timespan::Timespan(day_t value) : days { value } {}

bool Timespan::operator<(Timespan other) const { return days < other.days; };
bool Timespan::operator>(Timespan other) const { return days > other.days; };
bool Timespan::operator<=(Timespan other) const { return days <= other.days; };
bool Timespan::operator>=(Timespan other) const { return days >= other.days; };
bool Timespan::operator==(Timespan other) const { return days == other.days; };
bool Timespan::operator!=(Timespan other) const { return days != other.days; };

Timespan Timespan::operator+(Timespan other) const { return days + other.days; }

Timespan Timespan::operator-(Timespan other) const { return days - other.days; }

Timespan Timespan::operator*(day_t factor) const { return days * factor; }

Timespan Timespan::operator/(day_t factor) const { return days / factor; }

Timespan& Timespan::operator+=(Timespan other) {
	days += other.days;
	return *this;
}

Timespan& Timespan::operator-=(Timespan other) {
	days -= other.days;
	return *this;
}

Timespan& Timespan::operator++() {
	days++;
	return *this;
}

Timespan Timespan::operator++(int) {
	Timespan old = *this;
	++(*this);
	return old;
}

Timespan::operator day_t() const {
	return days;
}

Timespan::operator double() const {
	return days;
}

Timespan::operator std::string() const {
	return std::to_string(days);
}

std::ostream& OpenVic::operator<<(std::ostream& out, Timespan const& timespan) {
	return out << static_cast<std::string>(timespan);
}

Timespan Date::_dateToTimespan(year_t year, month_t month, day_t day) {
	month = std::clamp<month_t>(month, 1, MONTHS_IN_YEAR);
	day = std::clamp<day_t>(day, 1, DAYS_IN_MONTH[month - 1]);
	return year * DAYS_IN_YEAR + DAYS_UP_TO_MONTH[month - 1] + day - 1;
}

Timespan::day_t const* Date::DAYS_UP_TO_MONTH = generate_days_up_to_month();

Timespan::day_t const* Date::generate_days_up_to_month() {
	static Timespan::day_t days_up_to_month[MONTHS_IN_YEAR];
	Timespan::day_t days = 0;
	for (int month = 0; month < MONTHS_IN_YEAR;
		days_up_to_month[month] = days, days += DAYS_IN_MONTH[month++]);
	assert(days == DAYS_IN_YEAR);
	return days_up_to_month;
}

Date::month_t const* Date::MONTH_FROM_DAY_IN_YEAR = generate_month_from_day_in_year();

Date::month_t const* Date::generate_month_from_day_in_year() {
	static month_t month_from_day_in_year[DAYS_IN_YEAR];
	Timespan::day_t days_left = 0;
	for (int day = 0, month = 0; day < DAYS_IN_YEAR;
		days_left = (days_left > 0 ? days_left : DAYS_IN_MONTH[month++]) - 1,
		month_from_day_in_year[day++] = month);
	assert(days_left == 0);
	assert(month_from_day_in_year[DAYS_IN_YEAR - 1] == MONTHS_IN_YEAR);
	return month_from_day_in_year;
}

Date::Date(Timespan total_days) : timespan { total_days } {
	if (timespan < 0) {
		Logger::error("Invalid timespan for date: ", timespan, " (cannot be negative)");
		timespan = 0;
	}
}

Date::Date(year_t year, month_t month, day_t day) : timespan { _dateToTimespan(year, month, day) } {}

Date::year_t Date::getYear() const {
	return static_cast<Timespan::day_t>(timespan) / DAYS_IN_YEAR;
}

Date::month_t Date::getMonth() const {
	return MONTH_FROM_DAY_IN_YEAR[static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR];
}

Date::day_t Date::getDay() const {
	return (static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR) - DAYS_UP_TO_MONTH[getMonth() - 1] + 1;
}

bool Date::operator<(Date other) const { return timespan < other.timespan; };
bool Date::operator>(Date other) const { return timespan > other.timespan; };
bool Date::operator<=(Date other) const { return timespan <= other.timespan; };
bool Date::operator>=(Date other) const { return timespan >= other.timespan; };
bool Date::operator==(Date other) const { return timespan == other.timespan; };
bool Date::operator!=(Date other) const { return timespan != other.timespan; };

Date Date::operator+(Timespan other) const { return timespan + other; }

Timespan Date::operator-(Date other) const { return timespan - other.timespan; }

Date& Date::operator+=(Timespan other) {
	timespan += other;
	return *this;
}

Date& Date::operator-=(Timespan other) {
	timespan -= other;
	return *this;
}

Date& Date::operator++() {
	timespan++;
	return *this;
}

Date Date::operator++(int) {
	Date old = *this;
	++(*this);
	return old;
}

Date::operator std::string() const {
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

std::ostream& OpenVic::operator<<(std::ostream& out, Date const& date) {
	return out << static_cast<int>(date.getYear()) << '.' << static_cast<int>(date.getMonth()) << '.' << static_cast<int>(date.getDay());
}

// Parsed from string of the form YYYY.MM.DD
Date Date::from_string(const std::string_view date) {
	size_t first_pos = 0;
	while (first_pos < date.length() && std::isdigit(date[first_pos])) {
		first_pos++;
	}

	if (first_pos == 0) {
		Logger::error("Failed to find year digits in date: ", date);
		return {};
	}

	int val = 0;
	char const* start = date.data();
	char const* end = start + first_pos;
	std::from_chars_result result = std::from_chars(start, end, val);
	if (result.ec != std::errc{} || result.ptr != end || val < 0 || val >= 1 << (8 * sizeof(year_t))) {
		Logger::error("Failed to read year: ", date);
		return {};
	}
	year_t year = val;
	month_t month = 1;
	day_t day = 1;
	if (first_pos < date.length()) {
		if (date[first_pos] == '.') {
			size_t second_pos = ++first_pos;
			while (second_pos < date.length() && std::isdigit(date[second_pos])) {
				second_pos++;
			}
			if (first_pos == second_pos) {
				Logger::error("Failed to find month digits in date: ", date);
			} else {
				start = date.data() + first_pos;
				end = date.data() + second_pos;
				result = std::from_chars(start, end, val);
				if (result.ec != std::errc{} || result.ptr != end || val < 1 || val > MONTHS_IN_YEAR) {
					Logger::error("Failed to read month: ", date);
				} else {
					month = val;
					if (second_pos < date.length()) {
						if (date[second_pos] == '.') {
							size_t third_pos = ++second_pos;
							while (third_pos < date.length() && std::isdigit(date[third_pos])) {
								third_pos++;
							}
							if (second_pos == third_pos) {
								Logger::error("Failed to find day digits in date: ", date);
							} else {
								start = date.data() + second_pos;
								end = date.data() + third_pos;
								result = std::from_chars(start, end, val);
								if (result.ec != std::errc{} || result.ptr != end || val < 1 || val > DAYS_IN_MONTH[month - 1]) {
									Logger::error("Failed to read day: ", date);
								} else {
									day = val;
									if (third_pos < date.length()) {
										Logger::error("Unexpected string \"", date.substr(third_pos), "\" at the end of date ", date);
									}
								}
							}
						} else Logger::error("Unexpected character \"", date[second_pos], "\" in month of date ", date);
					}
				}
			}
		} else Logger::error("Unexpected character \"", date[first_pos], "\" in year of date ", date);
	}
	return _dateToTimespan(year, month, day);
};
