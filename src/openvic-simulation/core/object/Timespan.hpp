#pragma once

#include <charconv>
#include <cstdint>
#include <numeric>
#include <ostream>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/Typedefs.hpp"

namespace OpenVic {
	// A relative period between points in time, measured in days
	class Timespan {
	public:
		/* PROPERTY generated getter functions will return timespans by value, rather than const reference. */
		using ov_return_by_value = void;

		using year_t = int16_t;
		using month_t = uint8_t;
		using day_t = uint8_t;
		using value_t = int64_t;

		static constexpr std::array DAYS_IN_MONTH = std::to_array<day_t>({ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 });
		static constexpr month_t MONTHS_IN_YEAR = DAYS_IN_MONTH.size();
		static_assert(MONTHS_IN_YEAR == 12);

		static constexpr value_t DAYS_IN_YEAR =
			std::accumulate(DAYS_IN_MONTH.begin(), DAYS_IN_MONTH.end(), value_t { 0 });
		static_assert(DAYS_IN_YEAR == 365);

		static constexpr std::array<value_t, MONTHS_IN_YEAR> DAYS_UP_TO_MONTH = [] {
			std::array<value_t, MONTHS_IN_YEAR> days_up_to_month {};
			value_t days = 0;
			for (month_t month = 0; month < MONTHS_IN_YEAR; month++) {
				days_up_to_month[month] = days;
				days += DAYS_IN_MONTH[month];
			}
			return days_up_to_month;
		}();
		static_assert(DAYS_UP_TO_MONTH[MONTHS_IN_YEAR - 1] == DAYS_IN_YEAR - DAYS_IN_MONTH[MONTHS_IN_YEAR - 1]);

		OV_ALWAYS_INLINE constexpr Timespan(value_t value = 0) : days { value } {}

		OV_ALWAYS_INLINE friend constexpr auto operator<=>(Timespan const&, Timespan const&) = default;
		OV_ALWAYS_INLINE friend constexpr bool operator==(Timespan const&, Timespan const&) = default;
		OV_SPEED_INLINE constexpr Timespan operator+(Timespan other) const {
			return days + other.days;
		}
		OV_SPEED_INLINE constexpr Timespan operator-(Timespan other) const {
			return days - other.days;
		}
		OV_SPEED_INLINE constexpr Timespan operator*(value_t factor) const {
			return days * factor;
		}
		OV_SPEED_INLINE constexpr Timespan operator/(value_t factor) const {
			return days / factor;
		}
		OV_SPEED_INLINE constexpr Timespan& operator+=(Timespan other) {
			days += other.days;
			return *this;
		}
		OV_SPEED_INLINE constexpr Timespan& operator-=(Timespan other) {
			days -= other.days;
			return *this;
		}
		OV_SPEED_INLINE constexpr Timespan& operator++() {
			days++;
			return *this;
		}
		OV_SPEED_INLINE constexpr Timespan operator++(int) {
			Timespan old = *this;
			++(*this);
			return old;
		}
		OV_SPEED_INLINE constexpr Timespan& operator--() {
			days--;
			return *this;
		}
		OV_SPEED_INLINE constexpr Timespan operator--(int) {
			Timespan old = *this;
			--(*this);
			return old;
		}
		OV_SPEED_INLINE constexpr Timespan operator-() const {
			Timespan ret = *this;
			ret.days = -ret.days;
			return ret;
		}

		OV_ALWAYS_INLINE constexpr value_t to_int() const {
			return days;
		}
		OV_ALWAYS_INLINE explicit constexpr operator value_t() const {
			return days;
		}

		inline std::to_chars_result to_chars(char* first, char* last) const {
			return std::to_chars(first, last, days);
		}

		memory::string to_string() const;
		explicit operator memory::string() const;

		OV_SPEED_INLINE static constexpr Timespan from_years(year_t num);
		OV_SPEED_INLINE static constexpr Timespan from_months(value_t num);
		OV_SPEED_INLINE static constexpr Timespan from_days(value_t num);

	private:
		value_t days;
	};

	OV_SPEED_INLINE constexpr Timespan Timespan::from_years(year_t num) {
		return static_cast<value_t>(num) * DAYS_IN_YEAR;
	}
	OV_SPEED_INLINE constexpr Timespan Timespan::from_months(value_t num) {
		return (num / MONTHS_IN_YEAR) * DAYS_IN_YEAR + DAYS_UP_TO_MONTH[num % MONTHS_IN_YEAR];
	}
	OV_SPEED_INLINE constexpr Timespan Timespan::from_days(value_t num) {
		return num;
	}

	std::ostream& operator<<(std::ostream& out, Timespan const& timespan);
}
