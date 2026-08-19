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

		using day_t = int64_t;

		static constexpr std::array DAYS_IN_MONTH = std::to_array<day_t>({ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 });
		static constexpr day_t MONTHS_IN_YEAR = DAYS_IN_MONTH.size();

		static constexpr day_t DAYS_IN_YEAR =
			std::accumulate(DAYS_IN_MONTH.begin(), DAYS_IN_MONTH.end(), Timespan::day_t { 0 });
		static_assert(DAYS_IN_YEAR == 365);

		static constexpr std::array<Timespan::day_t, MONTHS_IN_YEAR> DAYS_UP_TO_MONTH = [] {
			std::array<Timespan::day_t, MONTHS_IN_YEAR> days_up_to_month {};
			Timespan::day_t days = 0;
			for (Timespan::day_t month = 0; month < MONTHS_IN_YEAR; month++) {
				days_up_to_month[month] = days;
				days += DAYS_IN_MONTH[month];
			}
			return days_up_to_month;
		}();

		OV_ALWAYS_INLINE constexpr Timespan(day_t value = 0) : days { value } {}

		OV_ALWAYS_INLINE friend constexpr auto operator<=>(Timespan const&, Timespan const&) = default;
		OV_ALWAYS_INLINE friend constexpr bool operator==(Timespan const&, Timespan const&) = default;
		OV_SPEED_INLINE constexpr Timespan operator+(Timespan other) const {
			return days + other.days;
		}
		OV_SPEED_INLINE constexpr Timespan operator-(Timespan other) const {
			return days - other.days;
		}
		OV_SPEED_INLINE constexpr Timespan operator*(day_t factor) const {
			return days * factor;
		}
		OV_SPEED_INLINE constexpr Timespan operator/(day_t factor) const {
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

		OV_ALWAYS_INLINE constexpr day_t to_int() const {
			return days;
		}
		OV_ALWAYS_INLINE explicit constexpr operator day_t() const {
			return days;
		}

		inline std::to_chars_result to_chars(char* first, char* last) const {
			return std::to_chars(first, last, days);
		}

		memory::string to_string() const;
		explicit operator memory::string() const;

		OV_SPEED_INLINE static constexpr Timespan from_years(day_t num);
		OV_SPEED_INLINE static constexpr Timespan from_months(day_t num);
		OV_SPEED_INLINE static constexpr Timespan from_days(day_t num);

	private:
		day_t days;
	};

	OV_SPEED_INLINE constexpr Timespan Timespan::from_years(day_t num) {
		return num * DAYS_IN_YEAR;
	}
	OV_SPEED_INLINE constexpr Timespan Timespan::from_months(day_t num) {
		return (num / MONTHS_IN_YEAR) * DAYS_IN_YEAR + DAYS_UP_TO_MONTH[num % MONTHS_IN_YEAR];
	}
	OV_SPEED_INLINE constexpr Timespan Timespan::from_days(day_t num) {
		return num;
	}

	std::ostream& operator<<(std::ostream& out, Timespan const& timespan);
}
