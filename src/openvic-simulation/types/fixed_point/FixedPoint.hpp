#pragma once

#include <cctype>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <system_error>

#include <fmt/base.h>
#include <fmt/format.h>

#include <zpp_bits.h>

#include "openvic-simulation/types/StackString.hpp"
#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Logger.hpp"
#include "openvic-simulation/utility/NumberUtils.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"
#include "openvic-simulation/utility/Utility.hpp"
#include "openvic-simulation/utility/Containers.hpp"

/* Sin lookup table */
#include "openvic-simulation/types/fixed_point/FixedPointLUT_sin.hpp"

/* Base e exponential lookup table */
#include "openvic-simulation/types/fixed_point/FixedPointLUT_2_16_EXP_e.hpp"

/* Base 2001 exponential lookup table */
#include "openvic-simulation/types/fixed_point/FixedPointLUT_2_16_EXP_2001.hpp"

namespace OpenVic {
	struct fixed_point_t {
		/* PROPERTY generated getter functions will return fixed points by value, rather than const reference. */
		using ov_return_by_value = void;
		using value_type = int64_t;

		friend zpp::bits::access;
		using serialize = zpp::bits::members<1>;

		static constexpr size_t SIZE = 8;

		static constexpr int32_t PRECISION = 16;
		static constexpr value_type ONE = 1 << PRECISION;
		static constexpr value_type FRAC_MASK = ONE - 1;

		/* Fixed points represent any base 2 number with 48 bits above the point and 16 bits below it.
		 * - Any number expressible as n / 2^16 where n is an int64_t can be converted to a fixed point exactly.
		 * - If a number cannot be expressed as an integer divided by 2^16, it will be rounded down to the first
		 *   representable number below it. For example: 0.01 = 655.36 / 2^16 becomes 0.0099945068359375 = 655 / 2^16.
		 * - While numbers with an integer part greater than 2^32 can be represented, they do not work properly with
		 *   multiplication and division which involve an intermediate value that is 2^16 times larger than the result.
		 *   For numbers with integer part smaller than 2^32 the top 16 bits prevent this intermediate value from
		 *   overflowing. For larger values this will not work and the result will be missing its most significant bits. */

	private:
		value_type PROPERTY_RW_CUSTOM_NAME(value, get_raw_value, set_raw_value);

		static_assert(_detail::LUT::SIN_PRECISION == PRECISION);
		static_assert(_detail::LUT::_2_16_EXP_e_DIVISOR == 1 << PRECISION);
		static_assert(_detail::LUT::_2_16_EXP_2001_DIVISOR == 1 << PRECISION);

		// Doesn't account for sign, so -n.abc -> 1 - 0.abc
		constexpr fixed_point_t get_frac() const {
			return parse_raw(value & FRAC_MASK);
		}

	public:
		constexpr fixed_point_t() : value { 0 } {}
		explicit constexpr fixed_point_t(value_type new_value) : value { new_value } {}
		constexpr fixed_point_t(int32_t new_value) : value { static_cast<value_type>(new_value) << PRECISION } {}

		static const fixed_point_t max;
		static const fixed_point_t min;
		static const fixed_point_t usable_max;
		static const fixed_point_t usable_min;
		static const fixed_point_t epsilon;
		static const fixed_point_t _0;
		static const fixed_point_t _1;
		static const fixed_point_t _2;
		static const fixed_point_t _4;
		static const fixed_point_t _10;
		static const fixed_point_t _100;
		static const fixed_point_t _0_01;
		static const fixed_point_t _0_10;
		static const fixed_point_t _0_20;
		static const fixed_point_t _0_25;
		static const fixed_point_t _0_50;
		static const fixed_point_t _1_50;
		static const fixed_point_t minus_one;
		static const fixed_point_t pi;
		static const fixed_point_t pi2;
		static const fixed_point_t pi_quarter;
		static const fixed_point_t pi_half;
		static const fixed_point_t one_div_pi2;
		static const fixed_point_t deg2rad;
		static const fixed_point_t rad2deg;
		static const fixed_point_t e;

		// Standard for constexpr requires this here
		template<std::integral T>
		constexpr friend fixed_point_t operator/(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value / rhs);
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator/(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << (2 * PRECISION)) / rhs.value);
		}

		template<std::integral T>
		constexpr fixed_point_t& operator/=(T const& obj) {
			value /= obj;
			return *this;
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator*(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value * rhs);
		}

		template<std::integral T>
		constexpr fixed_point_t& operator*=(T const& obj) {
			value *= obj;
			return *this;
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator+(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value + (static_cast<value_type>(rhs) << PRECISION));
		}

		template<std::integral T>
		constexpr std::strong_ordering operator<=>(T const& rhs) const {
			return value <=> static_cast<value_type>(rhs) << PRECISION;
		}

		constexpr fixed_point_t sin() const {
			using namespace _detail::LUT;

			value_type num = (*this % pi2 * one_div_pi2).get_raw_value();

			const bool negative = num < 0;
			if (negative) {
				num = -num;
			}

			const value_type index = num >> SIN_SHIFT;
			const value_type a = SIN[index];
			const value_type b = SIN[index + 1];

			const value_type fraction = (num - (index << SIN_SHIFT)) << SIN_COUNT_LOG2;
			const value_type result = a + (((b - a) * fraction) >> SIN_PRECISION);
			return !negative ? parse_raw(result) : parse_raw(-result);
		}

		constexpr fixed_point_t cos() const {
			return (*this + pi_half).sin();
		}

		constexpr bool is_negative() const {
			return value < 0;
		}

		constexpr fixed_point_t abs() const {
			return !is_negative() ? parse_raw(value) : parse_raw(-value);
		}

		constexpr fixed_point_t sqrt() const {
			return !is_negative()
				? parse_raw(NumberUtils::sqrt(static_cast<uint64_t>(value) << PRECISION))
				: _0;
		}

		constexpr bool is_integer() const {
			return get_frac() == 0;
		}

		constexpr fixed_point_t floor() const {
			return parse_raw(value & ~FRAC_MASK);
		}

		constexpr fixed_point_t ceil() const {
			return floor() + !is_integer();
		}

		/* WARNING: the results of these rounding functions are affected by the accuracy of base 2 fixed point numbers,
		 * for example 1.0 rounded to a multiple of 0.01 is 0.99945068359375 for down and 1.0094451904296875 for up. */
		constexpr fixed_point_t round_down_to_multiple(fixed_point_t factor) const {
			const fixed_point_t remainder = *this % factor;
			return *this - remainder - (remainder < 0 ? factor : _0);
		}

		constexpr fixed_point_t round_up_to_multiple(fixed_point_t factor) const {
			const fixed_point_t remainder = *this % factor;
			return *this - remainder + (remainder > 0 ? factor : _0);
		}

		constexpr int64_t to_int64_t() const {
			return value >> PRECISION;
		}

		constexpr int32_t to_int32_t() const {
			return static_cast<int32_t>(to_int64_t());
		}

		//away from zero
		constexpr int64_t to_int64_t_rounded() const {
			const fixed_point_t fp = *this;
			if (fp >= 0) {
				return (fp + _0_50).value >> PRECISION;
			} else {
				return (fp - _0_50).value >> PRECISION;
			}
		}
		constexpr int32_t to_int32_t_rounded() const {
			return static_cast<int32_t>(to_int64_t_rounded());
		}

		template<std::integral T>
		constexpr T to() const {
			return static_cast<T>(to_int64_t());
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

		inline constexpr std::to_chars_result to_chars(char* first, char* last, size_t decimal_places = -1) const {
			if (first == nullptr || first >= last) {
				return { last, std::errc::value_too_large };
			}

			if (is_negative()) {
				*first = '-';
				++first;
				if (last - first <= 1) {
					return { last, std::errc::value_too_large };
				}
			}

			std::to_chars_result result {};
			if (decimal_places == static_cast<size_t>(-1)) {
				result = StringUtils::to_chars(first, last, abs().to_int64_t());
				if (OV_unlikely(result.ec != std::errc {})) {
					return result;
				}
				if (OV_unlikely(last - result.ptr <= 1)) {
					return result;
				}

				fixed_point_t frac = abs().get_frac();
				if (frac != _0) {
					*result.ptr = '.';
					++result.ptr;
					do {
						if (OV_unlikely(last - result.ptr <= 1)) {
							return { last, std::errc::value_too_large };
						}
						frac *= 10;
						*result.ptr = static_cast<char>('0' + frac.to_int64_t());
						++result.ptr;
						frac = frac.get_frac();
					} while (frac != _0);
				}
				return result;
			}

			// Add the specified number of decimal places, potentially 0 (so no decimal point)
			fixed_point_t err = _0_50;
			for (size_t i = decimal_places; i > 0; --i) {
				err /= 10;
			}
			fixed_point_t val = this->abs() + err;


			result = StringUtils::to_chars(first, last, val.to_int64_t());
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}

			if (decimal_places > 0) {
				if (OV_unlikely(last - result.ptr <= 1)) {
					return result;
				}
				val = val.get_frac();
				*result.ptr = '.';
				result.ptr++;
				do {
					if (OV_unlikely(last - result.ptr <= 1)) {
						return result;
					}
					val *= 10;
					*result.ptr = static_cast<char>('0' + val.to_int64_t());
					++result.ptr;
					val = val.get_frac();
				} while (--decimal_places > 0);
			}
			return result;
		}

		struct stack_string;
		inline constexpr stack_string to_array(size_t decimal_places = -1) const;

		struct stack_string final : StackString<25> {
		protected:
			using StackString::StackString;
			friend inline constexpr stack_string fixed_point_t::to_array(size_t decimal_places) const;
		};

		memory::string to_string(size_t decimal_places = -1) const {
			stack_string result = to_array(decimal_places);
			if (OV_unlikely(result.empty())) {
				return {};
			}

			return result;
		}

		// Deterministic
		inline static constexpr fixed_point_t parse_raw(value_type value) {
			return fixed_point_t { value };
		}

		// Deterministic
		static constexpr fixed_point_t parse(int64_t value) {
			return parse_raw(value << PRECISION);
		}

		// Deterministic
		constexpr std::from_chars_result from_chars(char const* begin, char const* end) {
			if (begin == nullptr || begin >= end) {
				return { begin, std::errc::invalid_argument };
			}

			if (char const& c = *(end - 1); c == 'f' || c == 'F') {
				--end;
				if (begin == end) {
					return { begin, std::errc::invalid_argument };
				}
			}

			char const* dot_pointer = begin;
			while (*dot_pointer != '.' && ++dot_pointer != end) {}
			// "."
			if (dot_pointer == begin && dot_pointer + 1 == end) {
				return { begin, std::errc::invalid_argument };
			}

			fixed_point_t result = 0;
			std::from_chars_result from_chars = {};
			if (dot_pointer != begin) {
				// Non-empty integer part, may be negative
				from_chars = from_chars_integer(begin, dot_pointer, result);
			}

			if (from_chars.ec != std::errc{}) {
				return from_chars;
			}

			if (dot_pointer + 1 < end) {
				// Non-empty fractional part, cannot be negative
				fixed_point_t adder;
				from_chars = from_chars_fraction(dot_pointer + 1, end, adder);
				result += result.is_negative() || (*begin == '-' && result == _0) ? -adder : adder;
			}

			if (from_chars.ec != std::errc{}) {
				return { begin, from_chars.ec };
			}

			value = result.value;
			return from_chars;
		}

		constexpr std::from_chars_result from_chars_with_plus(char const* begin, char const* end) {
			if (begin && *begin == '+') {
				begin++;
				if (begin < end && *begin == '-') {
					return std::from_chars_result { begin, std::errc::invalid_argument };
				}
			}

			return from_chars(begin, end);
		}

		// Deterministic
		static constexpr fixed_point_t parse(char const* str, char const* end, bool* successful = nullptr) {
			fixed_point_t value = 0;
			std::from_chars_result result = value.from_chars_with_plus(str, end);
			if (successful) {
				*successful = result.ec == std::errc {};
			}
			return value;
		}

		static constexpr fixed_point_t parse(char const* str, size_t length, bool* successful = nullptr) {
			return parse(str, str + length, successful);
		}

		static constexpr fixed_point_t parse(std::string_view str, bool* successful = nullptr) {
			return parse(str.data(), str.length(), successful);
		}

		// Not Deterministic
		static constexpr fixed_point_t parse_unsafe(float value) {
			return parse_raw(value * ONE + 0.5f * (value < 0 ? -1 : 1));
		}

		// Not Deterministic
		static fixed_point_t parse_unsafe(char const* value) {
			char* endpointer;
			double double_value = std::strtod(value, &endpointer);

			if (*endpointer != '\0') {
				Logger::error("Unsafe fixed point parse failed to parse the end of a string: \"", endpointer, "\"");
			}

			value_type integer_value = static_cast<long>(double_value * ONE + 0.5 * (double_value < 0 ? -1 : 1));

			return parse_raw(integer_value);
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

		operator memory::string() const {
			return to_string();
		}

		friend std::ostream& operator<<(std::ostream& stream, fixed_point_t const& obj) {
			stack_string result = obj.to_array();
			if (OV_unlikely(result.empty())) {
				return stream;
			}

			return stream << static_cast<std::string_view>(result);
		}

		constexpr friend fixed_point_t operator-(fixed_point_t const& obj) {
			return parse_raw(-obj.value);
		}

		constexpr friend fixed_point_t operator+(fixed_point_t const& obj) {
			return parse_raw(+obj.value);
		}

		constexpr friend fixed_point_t operator+(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value + rhs.value);
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator+(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << PRECISION) + rhs.value);
		}

		constexpr fixed_point_t& operator+=(fixed_point_t const& obj) {
			value += obj.value;
			return *this;
		}

		template<std::integral T>
		constexpr fixed_point_t& operator+=(T const& obj) {
			value += (static_cast<value_type>(obj) << PRECISION);
			return *this;
		}

		constexpr friend fixed_point_t operator-(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value - rhs.value);
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator-(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value - (static_cast<value_type>(rhs) << PRECISION));
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator-(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << PRECISION) - rhs.value);
		}

		constexpr fixed_point_t& operator-=(fixed_point_t const& obj) {
			value -= obj.value;
			return *this;
		}

		template<std::integral T>
		constexpr fixed_point_t& operator-=(T const& obj) {
			value -= (static_cast<value_type>(obj) << PRECISION);
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

		template<std::integral T>
		constexpr friend fixed_point_t operator<<(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value << rhs);
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator>>(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value >> rhs);
		}

		constexpr friend fixed_point_t operator*(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value * rhs.value >> PRECISION);
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator*(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs * rhs.value);
		}

		constexpr fixed_point_t& operator*=(fixed_point_t const& obj) {
			value *= obj.value;
			value >>= PRECISION;
			return *this;
		}

		constexpr friend fixed_point_t operator/(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw((lhs.value << PRECISION) / rhs.value);
		}

		constexpr fixed_point_t& operator/=(fixed_point_t const& obj) {
			value = (value << PRECISION) / obj.value;
			return *this;
		}

		//Preserves accuracy. Performing a normal multiplication of small values results in 0.
		constexpr static fixed_point_t mul_div(fixed_point_t const& a, fixed_point_t const& b, fixed_point_t const& denominator) {
			return parse_raw(a.value * b.value / denominator.value);
		}

		constexpr friend fixed_point_t operator%(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value % rhs.value);
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator%(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value % (static_cast<value_type>(rhs) << PRECISION));
		}

		template<std::integral T>
		constexpr friend fixed_point_t operator%(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << PRECISION) % rhs.value);
		}

		constexpr fixed_point_t& operator%=(fixed_point_t const& obj) {
			value %= obj.value;
			return *this;
		}

		template<std::integral T>
		constexpr fixed_point_t& operator%=(T const& obj) {
			value %= (static_cast<value_type>(obj) << PRECISION);
			return *this;
		}

		friend constexpr bool operator==(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value == rhs.value;
		}

		template<std::integral T>
		constexpr bool operator==(T const& rhs) const {
			return value == static_cast<value_type>(rhs) << PRECISION;
		}

		template<std::floating_point T>
		constexpr bool operator==(T const& rhs) const {
			if constexpr (std::same_as<T, float>) {
				return to_float() == rhs;
			} else {
				return to_double() == rhs;
			}
		}

		friend constexpr std::strong_ordering operator<=>(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value <=> rhs.value;
		}

		template<std::floating_point T>
		constexpr std::partial_ordering operator<=>(T const& rhs) const {
			if constexpr (std::same_as<T, float>) {
				return to_float() <=> rhs;
			} else {
				return to_double() <=> rhs;
			}
		}

	private:
		// Deterministic
		// Can produce negative values
		static constexpr std::from_chars_result from_chars_integer(char const* str, char const* const end, fixed_point_t& value) {
			int64_t parsed_value = 0;
			std::from_chars_result result = StringUtils::string_to_int64(str, end, parsed_value);
			if (result.ec == std::errc{}) {
				value = parse(parsed_value);
			}
			return result;
		}

		// Deterministic
		// Cannot produce negative values
		static constexpr std::from_chars_result from_chars_fraction(char const* begin, char const* end, fixed_point_t& value) {
			if (begin && *begin == '-') {
				return { begin, std::errc::invalid_argument };
			}

			end = end - begin > PRECISION ? begin + PRECISION : end;
			uint64_t parsed_value;
			std::from_chars_result result = StringUtils::string_to_uint64(begin, end, parsed_value);
			if (result.ec != std::errc{}) {
				return result;
			}

			for (ptrdiff_t remaining_shift = PRECISION - (end - begin); remaining_shift > 0; remaining_shift--) {
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
			value = parse_raw(ret);
			return result;
		}

		template<size_t N, std::array<int64_t, N> EXP_LUT>
		static constexpr fixed_point_t _exp_internal(fixed_point_t const& x) {
			const bool negative = x.is_negative();
			value_type bits = negative ? -x.value : x.value;
			fixed_point_t result = _1;

			for (size_t index = 0; bits != 0 && index < EXP_LUT.size(); ++index, bits >>= 1) {
				if (bits & 1LL) {
					result *= parse_raw(EXP_LUT[index]);
				}
			}

			if (bits != 0) {
				Logger::error("Fixed point exponential overflow!");
			}

			if (negative) {
				return _1 / result;
			} else {
				return result;
			}
		}

	public:
		static inline constexpr fixed_point_t exp(fixed_point_t const& x) {
			return _exp_internal<_detail::LUT::_2_16_EXP_e.size(), _detail::LUT::_2_16_EXP_e>(x);
		}

		static inline constexpr fixed_point_t exp_2001(fixed_point_t const& x) {
			return _exp_internal<_detail::LUT::_2_16_EXP_2001.size(), _detail::LUT::_2_16_EXP_2001>(x);
		}
	};

	static_assert(sizeof(fixed_point_t) == fixed_point_t::SIZE, "fixed_point_t is not 8 bytes");

	inline constexpr fixed_point_t::stack_string fixed_point_t::to_array(size_t decimal_places) const {
		stack_string str {};
		std::to_chars_result result = to_chars(str._array.data(), str._array.data() + str._array.size(), decimal_places);
		str._string_size = result.ptr - str.data();
		return str;
	}

	inline constexpr fixed_point_t fixed_point_t::max = parse_raw(std::numeric_limits<value_type>::max());
	inline constexpr fixed_point_t fixed_point_t::min = parse_raw(std::numeric_limits<value_type>::min());
	inline constexpr fixed_point_t fixed_point_t::usable_max = parse_raw(2147483648LL);
	inline constexpr fixed_point_t fixed_point_t::usable_min = -usable_max;
	inline constexpr fixed_point_t fixed_point_t::epsilon = parse_raw(1);
	inline constexpr fixed_point_t fixed_point_t::_0 = 0;
	inline constexpr fixed_point_t fixed_point_t::_1 = 1;
	inline constexpr fixed_point_t fixed_point_t::_2 = 2;
	inline constexpr fixed_point_t fixed_point_t::_4 = 4;
	inline constexpr fixed_point_t fixed_point_t::_10 = 10;
	inline constexpr fixed_point_t fixed_point_t::_100 = 100;
	inline constexpr fixed_point_t fixed_point_t::_0_01 = _1 / 100;
	inline constexpr fixed_point_t fixed_point_t::_0_10 = _1 / 10;
	inline constexpr fixed_point_t fixed_point_t::_0_20 = _1 / 5;
	inline constexpr fixed_point_t fixed_point_t::_0_25 = _1 / 4;
	inline constexpr fixed_point_t fixed_point_t::_0_50 = _1 / 2;
	inline constexpr fixed_point_t fixed_point_t::_1_50 = _1 + _0_50;
	inline constexpr fixed_point_t fixed_point_t::minus_one = -1;
	inline constexpr fixed_point_t fixed_point_t::pi = parse_raw(205887LL);
	inline constexpr fixed_point_t fixed_point_t::pi2 = pi * 2;
	inline constexpr fixed_point_t fixed_point_t::pi_quarter = pi / 4;
	inline constexpr fixed_point_t fixed_point_t::pi_half = pi / 2;
	inline constexpr fixed_point_t fixed_point_t::one_div_pi2 = 1 / pi2;
	inline constexpr fixed_point_t fixed_point_t::deg2rad = parse_raw(1143LL);
	inline constexpr fixed_point_t fixed_point_t::rad2deg = parse_raw(3754936LL);
	inline constexpr fixed_point_t fixed_point_t::e = parse_raw(178145LL);
}

template<>
struct fmt::formatter<OpenVic::fixed_point_t> {
	constexpr format_parse_context::iterator parse(format_parse_context& ctx) {
		if (ctx.begin() == ctx.end() || *ctx.begin() == '}') {
			return ctx.begin();
		}

		format_parse_context::iterator end = parse_format_specs(ctx.begin(), ctx.end(), _specs, ctx, detail::type::double_type);

		if (_specs.type() == presentation_type::general) {
			report_error("OpenVic::fixed_point_t does not support 'g' or 'G' specifiers");
		} else if (_specs.type() == presentation_type::exp) {
			report_error("OpenVic::fixed_point_t does not support 'e' or 'E' specifiers");
		} else if (_specs.type() == presentation_type::fixed) {
			report_error("OpenVic::fixed_point_t does not support 'f' or 'F' specifiers");
		} else if (_specs.type() == presentation_type::hexfloat) {
			report_error("OpenVic::fixed_point_t does not support 'a' or 'A' specifiers");
		}

		return end;
	}

	format_context::iterator format(OpenVic::fixed_point_t fp, format_context& ctx) const;

private:
	fmt::detail::dynamic_format_specs<char> _specs;
};
