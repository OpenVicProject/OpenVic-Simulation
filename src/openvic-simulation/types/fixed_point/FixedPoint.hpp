#pragma once

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string_view>

#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/NumberUtils.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"

namespace OpenVic {
	struct fixed_point_t {
		/* PROPERTY generated getter functions will return fixed points by value, rather than const reference. */
		using ov_return_by_value = void;

		static constexpr size_t SIZE = 8;

		static constexpr int32_t PRECISION = 16;
		static constexpr int64_t ONE = 1 << PRECISION;
		static constexpr int64_t FRAC_MASK = ONE - 1;

		/* Fixed points represent any base 2 number with 48 bits above the point and 16 bits below it.
		 * - Any number expressible as n / 2^16 where n is an int64_t can be converted to a fixed point exactly.
		 * - If a number cannot be expressed as an integer divided by 2^16, it will be rounded down to the first
		 *   representable number below it. For example: 0.01 = 655.36 / 2^16 becomes 0.0099945068359375 = 655 / 2^16.
		 * - While numbers with an integer part greater than 2^32 can be represented, they do not work properly with
		 *   multiplication and division which involve an intermediate value that is 2^16 times larger than the result.
		 *   For numbers with integer part smaller than 2^32 the top 16 bits prevent this intermediate value from
		 *   overflowing. For larger values this will not work and the result will be missing its most significant bits. */

	private:
		int64_t PROPERTY_RW_CUSTOM_NAME(value, get_raw_value, set_raw_value);

		/* Sin lookup table */
		#include "openvic-simulation/types/fixed_point/FixedPointLUT_sin.hpp"

		static_assert(SIN_LUT_PRECISION == PRECISION);
		
		// Doesn't account for sign, so -n.abc -> 1 - 0.abc
		constexpr fixed_point_t get_frac() const {
			return value & FRAC_MASK;
		}

	public:
		constexpr fixed_point_t() : value { 0 } {}
		constexpr fixed_point_t(int64_t new_value) : value { new_value } {}
		constexpr fixed_point_t(int32_t new_value) : value { static_cast<int64_t>(new_value) << PRECISION } {}

		constexpr ~fixed_point_t() = default;

		static constexpr fixed_point_t max() {
			return std::numeric_limits<int64_t>::max();
		}

		static constexpr fixed_point_t min() {
			return std::numeric_limits<int64_t>::min();
		}

		static constexpr fixed_point_t usable_max() {
			return static_cast<int64_t>(2147483648LL);
		}

		static constexpr fixed_point_t usable_min() {
			return -usable_max();
		}

		static constexpr fixed_point_t _0() {
			return 0;
		}

		static constexpr fixed_point_t _1() {
			return 1;
		}

		static constexpr fixed_point_t _2() {
			return 2;
		}

		static constexpr fixed_point_t _4() {
			return 4;
		}

		static constexpr fixed_point_t _100() {
			return 100;
		}

		static constexpr fixed_point_t _0_20() {
			return _1() / 5;
		}

		static constexpr fixed_point_t _0_50() {
			return _1() / 2;
		}

		static constexpr fixed_point_t _1_50() {
			return _1() + _0_50();
		}

		static constexpr fixed_point_t minus_one() {
			return -1;
		}

		static constexpr fixed_point_t pi() {
			return static_cast<int64_t>(205887LL);
		}

		static constexpr fixed_point_t pi2() {
			return pi() * 2;
		}

		static constexpr fixed_point_t pi_quarter() {
			return pi() / 4;
		}

		static constexpr fixed_point_t pi_half() {
			return pi() / 2;
		}

		static constexpr fixed_point_t one_div_pi2() {
			return 1 / pi2();
		}

		static constexpr fixed_point_t deg2rad() {
			return static_cast<int64_t>(1143LL);
		}

		static constexpr fixed_point_t rad2deg() {
			return static_cast<int64_t>(3754936LL);
		}

		static constexpr fixed_point_t e() {
			return static_cast<int64_t>(178145LL);
		}

		constexpr fixed_point_t sin() const {
			int64_t num = (*this % pi2() * one_div_pi2()).get_raw_value();

			const bool negative = num < 0;
			if (negative) {
				num = -num;
			}

			const int64_t index = num >> SIN_LUT_SHIFT;
			const int64_t a = SIN_LUT[index];
			const int64_t b = SIN_LUT[index + 1];

			const int64_t fraction = (num - (index << SIN_LUT_SHIFT)) << SIN_LUT_COUNT_LOG2;
			const int64_t result = a + (((b - a) * fraction) >> SIN_LUT_PRECISION);
			return !negative ? result : -result;
		}

		constexpr fixed_point_t cos() const {
			return (*this + pi_half()).sin();
		}

		constexpr bool is_negative() const {
			return value < 0;
		}

		constexpr fixed_point_t abs() const {
			return !is_negative() ? value : -value;
		}

		constexpr fixed_point_t sqrt() const {
			return !is_negative()
				? static_cast<int64_t>(NumberUtils::sqrt(static_cast<uint64_t>(value) << PRECISION))
				: 0;
		}

		constexpr bool is_integer() const {
			return get_frac() == 0;
		}

		constexpr fixed_point_t floor() const {
			return value & ~FRAC_MASK;
		}

		constexpr fixed_point_t ceil() const {
			return floor() + !is_integer();
		}

		/* WARNING: the results of these rounding functions are affected by the accuracy of base 2 fixed point numbers,
		 * for example 1.0 rounded to a multiple of 0.01 is 0.99945068359375 for down and 1.0094451904296875 for up. */
		constexpr fixed_point_t round_down_to_multiple(fixed_point_t factor) const {
			const fixed_point_t remainder = *this % factor;
			return *this - remainder - (remainder < 0 ? factor : _0());
		}

		constexpr fixed_point_t round_up_to_multiple(fixed_point_t factor) const {
			const fixed_point_t remainder = *this % factor;
			return *this - remainder + (remainder > 0 ? factor : _0());
		}

		constexpr int64_t to_int64_t() const {
			return value >> PRECISION;
		}

		constexpr int32_t to_int32_t() const {
			return static_cast<int32_t>(to_int64_t());
		}

		constexpr float to_float() const {
			return value / static_cast<float>(ONE);
		}

		constexpr float to_float_rounded() const {
			return static_cast<float>(NumberUtils::round_to_int64((value / static_cast<float>(ONE)) * 100000.0f)) / 100000.0f;
		}

		constexpr double to_double() const {
			return value / static_cast<double>(ONE);
		}

		constexpr double to_double_rounded() const {
			return NumberUtils::round_to_int64((value / static_cast<double>(ONE)) * 100000.0) / 100000.0;
		}

		static std::ostream& print(std::ostream& stream, fixed_point_t val, size_t decimal_places = 0) {
			if (decimal_places > 0) {
				fixed_point_t err = _0_50();
				for (size_t i = decimal_places; i > 0; --i) {
					err /= 10;
				}
				val += err;
			}
			if (val.is_negative()) {
				stream << "-";
			}
			val = val.abs();
			stream << val.to_int64_t() << ".";
			val = val.get_frac();
			do {
				val *= 10;
				stream << static_cast<char>('0' + val.to_int64_t());
				val = val.get_frac();
			} while (val > 0 && --decimal_places > 0);
			return stream;
		}

		std::string to_string(size_t decimal_places = 0) const {
			std::stringstream str;
			print(str, *this, decimal_places);
			return str.str();
		}

		// Deterministic
		static constexpr fixed_point_t parse_raw(int64_t value) {
			return value;
		}

		// Deterministic
		static constexpr fixed_point_t parse(int64_t value) {
			return value << PRECISION;
		}

		// Deterministic
		static constexpr fixed_point_t parse(char const* str, char const* end, bool* successful = nullptr) {
			if (successful != nullptr) {
				*successful = false;
			}

			if (str == nullptr || str >= end) {
				return _0();
			}

			bool negative = false;

			if (*str == '-') {
				negative = true;
				++str;
				if (str == end) {
					return _0();
				}
			}

			{
				const char last_char = *(end - 1);
				if (last_char == 'f' || last_char == 'F') {
					--end;
					if (str == end) {
						return _0();
					}
				}
			}

			char const* dot_pointer = str;
			while (*dot_pointer != '.' && ++dot_pointer != end) {}

			if (dot_pointer == str && dot_pointer + 1 == end) {
				// Invalid: ".", "+." or "-."
				return _0();
			}

			fixed_point_t result = _0();
			if (successful != nullptr) {
				*successful = true;
			}

			if (dot_pointer != str) {
				// Non-empty integer part
				bool int_successful = false;
				result += parse_integer(str, dot_pointer, &int_successful);
				if (!int_successful && successful != nullptr) {
					*successful = false;
				}
			}

			if (dot_pointer + 1 < end) {
				// Non-empty fractional part
				bool frac_successful = false;
				result += parse_fraction(dot_pointer + 1, end, &frac_successful);
				if (!frac_successful && successful != nullptr) {
					*successful = false;
				}
			}

			return negative ? -result : result;
		}

		static constexpr fixed_point_t parse(char const* str, size_t length, bool* successful = nullptr) {
			return parse(str, str + length, successful);
		}

		static fixed_point_t parse(std::string_view str, bool* successful = nullptr) {
			return parse(str.data(), str.length(), successful);
		}

		// Not Deterministic
		static constexpr fixed_point_t parse_unsafe(float value) {
			return static_cast<int64_t>(value * ONE + 0.5f * (value < 0 ? -1 : 1));
		}

		// Not Deterministic
		static fixed_point_t parse_unsafe(char const* value) {
			char* endpointer;
			double double_value = std::strtod(value, &endpointer);

			if (*endpointer != '\0') {
				Logger::error("Unsafe fixed point parse failed to parse the end of a string: \"", endpointer, "\"");
			}

			int64_t integer_value = static_cast<long>(double_value * ONE + 0.5 * (double_value < 0 ? -1 : 1));

			return integer_value;
		}

		constexpr operator int32_t() const {
			return to_int32_t();
		}

		constexpr operator int64_t() const {
			return to_int64_t();
		}

		constexpr operator float() const {
			return to_float();
		}

		constexpr operator double() const {
			return to_double();
		}

		operator std::string() const {
			return to_string();
		}

		friend std::ostream& operator<<(std::ostream& stream, fixed_point_t const& obj) {
			return obj.print(stream, obj);
		}

		constexpr friend fixed_point_t operator-(fixed_point_t const& obj) {
			return -obj.value;
		}

		constexpr friend fixed_point_t operator+(fixed_point_t const& obj) {
			return +obj.value;
		}

		constexpr friend fixed_point_t operator+(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value + rhs.value;
		}

		constexpr friend fixed_point_t operator+(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value + (static_cast<int64_t>(rhs) << PRECISION);
		}

		constexpr friend fixed_point_t operator+(int32_t const& lhs, fixed_point_t const& rhs) {
			return (static_cast<int64_t>(lhs) << PRECISION) + rhs.value;
		}

		constexpr fixed_point_t operator+=(fixed_point_t const& obj) {
			value += obj.value;
			return *this;
		}

		constexpr fixed_point_t operator+=(int32_t const& obj) {
			value += (static_cast<int64_t>(obj) << PRECISION);
			return *this;
		}

		constexpr friend fixed_point_t operator-(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value - rhs.value;
		}

		constexpr friend fixed_point_t operator-(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value - (static_cast<int64_t>(rhs) << PRECISION);
		}

		constexpr friend fixed_point_t operator-(int32_t const& lhs, fixed_point_t const& rhs) {
			return (static_cast<int64_t>(lhs) << PRECISION) - rhs.value;
		}

		constexpr fixed_point_t operator-=(fixed_point_t const& obj) {
			value -= obj.value;
			return *this;
		}

		constexpr fixed_point_t operator-=(int32_t const& obj) {
			value -= (static_cast<int64_t>(obj) << PRECISION);
			return *this;
		}

		constexpr fixed_point_t& operator++() {
			value += ONE;
			return *this;
		}

		constexpr fixed_point_t operator++(int) {
			const fixed_point_t old = *this;
			value += ONE;
			return old;
		}

		constexpr fixed_point_t& operator--() {
			value -= ONE;
			return *this;
		}

		constexpr fixed_point_t operator--(int) {
			const fixed_point_t old = *this;
			value -= ONE;
			return old;
		}

		constexpr friend fixed_point_t operator*(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value * rhs.value >> PRECISION;
		}

		constexpr friend fixed_point_t operator*(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value * rhs;
		}

		constexpr friend fixed_point_t operator*(int32_t const& lhs, fixed_point_t const& rhs) {
			return lhs * rhs.value;
		}

		constexpr fixed_point_t operator*=(fixed_point_t const& obj) {
			value *= obj.value;
			value >>= PRECISION;
			return *this;
		}

		constexpr fixed_point_t operator*=(int32_t const& obj) {
			value *= obj;
			return *this;
		}

		constexpr friend fixed_point_t operator/(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return (lhs.value << PRECISION) / rhs.value;
		}

		constexpr friend fixed_point_t operator/(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value / rhs;
		}

		constexpr friend fixed_point_t operator/(int32_t const& lhs, fixed_point_t const& rhs) {
			return (static_cast<int64_t>(lhs) << (2 * PRECISION)) / rhs.value;
		}

		constexpr fixed_point_t operator/=(fixed_point_t const& obj) {
			value = (value << PRECISION) / obj.value;
			return *this;
		}

		constexpr fixed_point_t operator/=(int32_t const& obj) {
			value /= obj;
			return *this;
		}

		constexpr friend fixed_point_t operator%(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value % rhs.value;
		}

		constexpr friend fixed_point_t operator%(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value % (static_cast<int64_t>(rhs) << PRECISION);
		}

		constexpr friend fixed_point_t operator%(int32_t const& lhs, fixed_point_t const& rhs) {
			return (static_cast<int64_t>(lhs) << PRECISION) % rhs.value;
		}

		constexpr fixed_point_t operator%=(fixed_point_t const& obj) {
			value %= obj.value;
			return *this;
		}

		constexpr fixed_point_t operator%=(int32_t const& obj) {
			value %= (static_cast<int64_t>(obj) << PRECISION);
			return *this;
		}

		constexpr friend bool operator<(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value < rhs.value;
		}

		constexpr friend bool operator<(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value < static_cast<int64_t>(rhs) << PRECISION;
		}

		constexpr friend bool operator<(int32_t const& lhs, fixed_point_t const& rhs) {
			return static_cast<int64_t>(lhs) << PRECISION < rhs.value;
		}

		constexpr friend bool operator<=(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value <= rhs.value;
		}

		constexpr friend bool operator<=(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value <= static_cast<int64_t>(rhs) << PRECISION;
		}

		constexpr friend bool operator<=(int32_t const& lhs, fixed_point_t const& rhs) {
			return static_cast<int64_t>(lhs) << PRECISION <= rhs.value;
		}

		constexpr friend bool operator>(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value > rhs.value;
		}

		constexpr friend bool operator>(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value > static_cast<int64_t>(rhs) << PRECISION;
		}

		constexpr friend bool operator>(int32_t const& lhs, fixed_point_t const& rhs) {
			return static_cast<int64_t>(lhs) << PRECISION > rhs.value;
		}

		constexpr friend bool operator>=(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value >= rhs.value;
		}

		constexpr friend bool operator>=(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value >= static_cast<int64_t>(rhs) << PRECISION;
		}

		constexpr friend bool operator>=(int32_t const& lhs, fixed_point_t const& rhs) {
			return static_cast<int64_t>(lhs) << PRECISION >= rhs.value;
		}

		constexpr friend bool operator==(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value == rhs.value;
		}

		constexpr friend bool operator==(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value == static_cast<int64_t>(rhs) << PRECISION;
		}

		constexpr friend bool operator==(int32_t const& lhs, fixed_point_t const& rhs) {
			return static_cast<int64_t>(lhs) << PRECISION == rhs.value;
		}

		constexpr friend bool operator!=(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value != rhs.value;
		}

		constexpr friend bool operator!=(fixed_point_t const& lhs, int32_t const& rhs) {
			return lhs.value != static_cast<int64_t>(rhs) << PRECISION;
		}

		constexpr friend bool operator!=(int32_t const& lhs, fixed_point_t const& rhs) {
			return static_cast<int64_t>(lhs) << PRECISION != rhs.value;
		}

		constexpr friend std::strong_ordering operator<=>(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value <=> rhs.value;
		}

	private:
		static constexpr fixed_point_t parse_integer(char const* str, char const* const end, bool* successful) {
			int64_t parsed_value = StringUtils::string_to_int64(str, end, successful, 10);
			return parse(parsed_value);
		}

		static constexpr fixed_point_t parse_fraction(char const* str, char const* end, bool* successful) {
			char const* const read_end = str + PRECISION;
			if (read_end < end) {
				end = read_end;
			}
			uint64_t parsed_value = StringUtils::string_to_uint64(str, end, successful, 10);
			while (end++ < read_end) {
				parsed_value *= 10;
			}
			uint64_t decimal = NumberUtils::pow(static_cast<uint64_t>(10), PRECISION);
			int64_t ret = 0;
			for (int i = PRECISION - 1; i >= 0; --i) {
				decimal >>= 1;
				if (parsed_value >= decimal) {
					parsed_value -= decimal;
					ret |= 1 << i;
				}
			}
			return ret;
		}
	};

	static_assert(sizeof(fixed_point_t) == fixed_point_t::SIZE, "fixed_point_t is not 8 bytes");
}
