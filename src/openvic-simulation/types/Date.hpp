#pragma once

#include <climits>
#include <cstdint>
#include <ostream>
#include <string>

#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	// A relative period between points in time, measured in days
	struct Timespan : ReturnByValueProperty {
		using day_t = int64_t;

	private:
		day_t days;

	public:
		Timespan(day_t value = 0);

		bool operator<(Timespan other) const;
		bool operator>(Timespan other) const;
		bool operator<=(Timespan other) const;
		bool operator>=(Timespan other) const;
		bool operator==(Timespan other) const;
		bool operator!=(Timespan other) const;

		Timespan operator+(Timespan other) const;
		Timespan operator-(Timespan other) const;
		Timespan operator*(day_t factor) const;
		Timespan operator/(day_t factor) const;
		Timespan& operator+=(Timespan other);
		Timespan& operator-=(Timespan other);
		Timespan& operator++();
		Timespan operator++(int);

		day_t to_int() const;
		explicit operator day_t() const;
		std::string to_string() const;
		explicit operator std::string() const;

		static Timespan from_years(day_t num);
		static Timespan from_months(day_t num);
		static Timespan from_days(day_t num);
	};
	std::ostream& operator<<(std::ostream& out, Timespan const& timespan);

	// Represents an in-game date
	// Note: Current implementation does not account for leap-years, or dates before Year 0
	struct Date : ReturnByValueProperty {
		using year_t = uint16_t;
		using month_t = uint8_t;
		using day_t = uint8_t;

		static constexpr Timespan::day_t MONTHS_IN_YEAR = 12;
		static constexpr Timespan::day_t DAYS_IN_YEAR = 365;
		static constexpr Timespan::day_t DAYS_IN_MONTH[MONTHS_IN_YEAR] { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		static Timespan::day_t const* DAYS_UP_TO_MONTH;
		static month_t const* MONTH_FROM_DAY_IN_YEAR;

		static constexpr char SEPARATOR_CHARACTER = '.';
		static const std::string MONTH_NAMES[MONTHS_IN_YEAR];

	private:
		// Number of days since Jan 1st, Year 0
		Timespan timespan;

		static Timespan _date_to_timespan(year_t year, month_t month, day_t day);
		static Timespan::day_t const* generate_days_up_to_month();
		static month_t const* generate_month_from_day_in_year();

	public:
		// The Timespan is considered to be the number of days since Jan 1st, Year 0
		Date(Timespan total_days);
		// Year month day specification
		Date(year_t year = 0, month_t month = 1, day_t day = 1);

		year_t get_year() const;
		month_t get_month() const;
		day_t get_day() const;

		bool operator<(Date other) const;
		bool operator>(Date other) const;
		bool operator<=(Date other) const;
		bool operator>=(Date other) const;
		bool operator==(Date other) const;
		bool operator!=(Date other) const;

		Date operator+(Timespan other) const;
		Timespan operator-(Date other) const;
		Date& operator+=(Timespan other);
		Date& operator-=(Timespan other);
		Date& operator++();
		Date operator++(int);

		bool in_range(Date start, Date end) const;

		/* This returns a std::string const&, rather than a std::string_view, as it needs to be converted to a
		 * godot::StringName in order to be localised, and std::string_view to a godot::StringName conversion requires an
		 * intermediary std::string. */
		std::string const& get_month_name() const;
		std::string to_string() const;
		explicit operator std::string() const;
		// Parsed from string of the form YYYY.MM.DD
		static Date from_string(char const* str, char const* end, bool* successful = nullptr, bool quiet = false);
		static Date from_string(char const* str, size_t length, bool* successful = nullptr, bool quiet = false);
		static Date from_string(std::string_view str, bool* successful = nullptr, bool quiet = false);
	};
	std::ostream& operator<<(std::ostream& out, Date date);
}

namespace std {
	template<>
	struct hash<OpenVic::Date> {
		[[nodiscard]] size_t operator()(OpenVic::Date date) const {
			size_t result = 0;
			OpenVic::utility::perfect_hash(result, date.get_day(), date.get_month(), date.get_year());
			return result;
		}
	};
}