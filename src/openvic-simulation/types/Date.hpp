#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <ostream>
#include <string>

#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	// A relative period between points in time, measured in days
	struct Timespan {
		/* PROPERTY generated getter functions will return timespans by value, rather than const reference. */
		using ov_return_by_value = void;

		using day_t = int64_t;

	private:
		day_t days;

	public:
		constexpr Timespan(day_t value = 0) : days { value } {}

		constexpr bool operator<(Timespan other) const {
			return days < other.days;
		};
		constexpr bool operator>(Timespan other) const {
			return days > other.days;
		};
		constexpr bool operator<=(Timespan other) const {
			return days <= other.days;
		};
		constexpr bool operator>=(Timespan other) const {
			return days >= other.days;
		};
		constexpr bool operator==(Timespan other) const {
			return days == other.days;
		};
		constexpr bool operator!=(Timespan other) const {
			return days != other.days;
		};

		constexpr Timespan operator+(Timespan other) const {
			return days + other.days;
		}
		constexpr Timespan operator-(Timespan other) const {
			return days - other.days;
		}
		constexpr Timespan operator*(day_t factor) const {
			return days * factor;
		}
		constexpr Timespan operator/(day_t factor) const {
			return days / factor;
		}
		constexpr Timespan& operator+=(Timespan other) {
			days += other.days;
			return *this;
		}
		constexpr Timespan& operator-=(Timespan other) {
			days -= other.days;
			return *this;
		}
		constexpr Timespan& operator++() {
			days++;
			return *this;
		}
		constexpr Timespan operator++(int) {
			Timespan old = *this;
			++(*this);
			return old;
		}

		constexpr day_t to_int() const {
			return days;
		}
		explicit constexpr operator day_t() const {
			return days;
		}

		std::string to_string() const;
		explicit operator std::string() const;

		static constexpr Timespan from_years(day_t num);
		static constexpr Timespan from_months(day_t num);
		static constexpr Timespan from_days(day_t num);
	};
	std::ostream& operator<<(std::ostream& out, Timespan const& timespan);

	// Represents an in-game date
	// Note: Current implementation does not account for leap-years, or dates before Year 0
	struct Date {
		/* PROPERTY generated getter functions will return dates by value, rather than const reference. */
		using ov_return_by_value = void;

		using year_t = uint16_t;
		using month_t = uint8_t;
		using day_t = uint8_t;

		static constexpr Timespan::day_t MONTHS_IN_YEAR = 12;

		static constexpr std::array<Timespan::day_t, MONTHS_IN_YEAR> DAYS_IN_MONTH {
			31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
		};

		static constexpr Timespan::day_t DAYS_IN_YEAR { []() {
			Timespan::day_t days = 0;
			for (Timespan::day_t days_in_month : DAYS_IN_MONTH) {
				days += days_in_month;
			}
			return days;
		}() };
		static_assert(DAYS_IN_YEAR == 365);

		static constexpr std::array<Timespan::day_t, MONTHS_IN_YEAR> DAYS_UP_TO_MONTH { []() {
			std::array<Timespan::day_t, MONTHS_IN_YEAR> days_up_to_month;
			Timespan::day_t days = 0;
			for (Timespan::day_t month = 0; month < MONTHS_IN_YEAR; month++) {
				days_up_to_month[month] = days;
				days += DAYS_IN_MONTH[month];
			}
			return days_up_to_month;
		}() };

		static constexpr std::array<month_t, DAYS_IN_YEAR> MONTH_FROM_DAY_IN_YEAR { []() {
			std::array<month_t, DAYS_IN_YEAR> month_from_day_in_year;
			Timespan::day_t days_left = 0;
			month_t month = 0;
			for (Timespan::day_t day = 0; day < DAYS_IN_YEAR; day++) {
				days_left = (days_left > 0 ? days_left : DAYS_IN_MONTH[month++]) - 1;
				month_from_day_in_year[day] = month;
			}
			return month_from_day_in_year;
		}() };

		static constexpr char SEPARATOR_CHARACTER = '.';

		static constexpr std::array<std::string_view, MONTHS_IN_YEAR> MONTH_NAMES {
			"January", "February", "March", "April", "May", "June",
			"July", "August", "September", "October", "November",
			"December"
		};
		static constexpr std::string_view INVALID_MONTH_NAME = "Invalid Month";

	private:
		// Number of days since Jan 1st, Year 0
		Timespan timespan;

		static constexpr Timespan _date_to_timespan(year_t year, month_t month, day_t day) {
			month = std::clamp<month_t>(month, 1, MONTHS_IN_YEAR);
			day = std::clamp<day_t>(day, 1, DAYS_IN_MONTH[month - 1]);
			return year * DAYS_IN_YEAR + DAYS_UP_TO_MONTH[month - 1] + day - 1;
		}

	public:
		// The Timespan is considered to be the number of days since Jan 1st, Year 0
		constexpr Date(Timespan total_days) : timespan { total_days >= 0 ? total_days : 0 } {}
		// Year month day specification
		constexpr Date(year_t year = 0, month_t month = 1, day_t day = 1) : timespan { _date_to_timespan(year, month, day) } {}

		constexpr year_t get_year() const {
			return static_cast<Timespan::day_t>(timespan) / DAYS_IN_YEAR;
		}
		constexpr month_t get_month() const {
			return MONTH_FROM_DAY_IN_YEAR[static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR];
		}
		constexpr day_t get_day() const {
			return (static_cast<Timespan::day_t>(timespan) % DAYS_IN_YEAR) - DAYS_UP_TO_MONTH[get_month() - 1] + 1;
		}

		constexpr bool operator<(Date other) const {
			return timespan < other.timespan;
		};
		constexpr bool operator>(Date other) const {
			return timespan > other.timespan;
		};
		constexpr bool operator<=(Date other) const {
			return timespan <= other.timespan;
		};
		constexpr bool operator>=(Date other) const {
			return timespan >= other.timespan;
		};
		constexpr bool operator==(Date other) const {
			return timespan == other.timespan;
		};
		constexpr bool operator!=(Date other) const {
			return timespan != other.timespan;
		};

		constexpr Date operator+(Timespan other) const {
			return timespan + other;
		}
		constexpr Timespan operator-(Date other) const {
			return timespan - other.timespan;
		}
		constexpr Date& operator+=(Timespan other) {
			timespan += other;
			return *this;
		}
		constexpr Date& operator-=(Timespan other) {
			timespan -= other;
			return *this;
		}
		constexpr Date& operator++() {
			timespan++;
			return *this;
		}
		constexpr Date operator++(int) {
			Date old = *this;
			++(*this);
			return old;
		}

		constexpr bool in_range(Date start, Date end) const {
			return start <= *this && *this <= end;
		}

		constexpr std::string_view get_month_name() const {
			const month_t month = get_month();
			if (1 <= month && month <= MONTHS_IN_YEAR) {
				return MONTH_NAMES[month - 1];
			}
			return INVALID_MONTH_NAME;
		}

		std::string to_string() const;
		explicit operator std::string() const;

		// Parsed from string of the form YYYY.MM.DD
		static Date from_string(char const* str, char const* end, bool* successful = nullptr, bool quiet = false);
		static Date from_string(char const* str, std::size_t length, bool* successful = nullptr, bool quiet = false);
		static Date from_string(std::string_view str, bool* successful = nullptr, bool quiet = false);
	};
	std::ostream& operator<<(std::ostream& out, Date date);

	constexpr Timespan Timespan::from_years(day_t num) {
		return num * Date::DAYS_IN_YEAR;
	}
	constexpr Timespan Timespan::from_months(day_t num) {
		return (num / Date::MONTHS_IN_YEAR) * Date::DAYS_IN_YEAR + Date::DAYS_UP_TO_MONTH[num % Date::MONTHS_IN_YEAR];
	}
	constexpr Timespan Timespan::from_days(day_t num) {
		return num;
	}
}

namespace std {
	template<>
	struct hash<OpenVic::Date> {
		[[nodiscard]] std::size_t operator()(OpenVic::Date date) const {
			std::size_t result = 0;
			OpenVic::utility::perfect_hash(result, date.get_day(), date.get_month(), date.get_year());
			return result;
		}
	};
}
