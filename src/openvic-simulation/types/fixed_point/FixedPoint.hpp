#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <fmt/base.h>
#include <fmt/format.h>

#include "openvic-simulation/core/Typedefs.hpp"

namespace OpenVic {
	enum class midpoint_rounding { AWAY_ZERO, TO_ZERO };
	
	template<typename T>
	concept integral_max_size_4 = std::integral<T> && sizeof(T) <= 4;

	struct fixed_point_t {
		/* PROPERTY generated getter functions will return fixed points by value, rather than const reference. */
		using ov_return_by_value = void;
		using value_type = int64_t;


		static constexpr size_t SIZE = 8;

		static constexpr int32_t PRECISION = 16;
		static constexpr value_type ONE = value_type { 1 } << PRECISION;
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
		value_type value;
	public:
		[[nodiscard]] constexpr value_type const& get_raw_value() const {
			return value;
		}
		// Doesn't account for sign, so -n.abc -> 1 - 0.abc
		OV_ALWAYS_INLINE constexpr fixed_point_t get_frac() const {
			return parse_raw(value & FRAC_MASK);
		}
	private:
		struct raw_value_t {
			explicit raw_value_t() = default;
		};
		inline static constexpr raw_value_t raw_value {};

		OV_ALWAYS_INLINE constexpr fixed_point_t(raw_value_t, value_type new_value) : value { new_value } {}

	public:
		OV_ALWAYS_INLINE constexpr fixed_point_t() : value { 0 } {}
		template<integral_max_size_4 T>
		OV_ALWAYS_INLINE constexpr fixed_point_t(T new_value) : value { static_cast<value_type>(new_value) << PRECISION } {}

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
		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator/(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value / rhs);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator/(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << (2 * PRECISION)) / rhs.value);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr fixed_point_t& operator/=(T const& obj) {
			value /= obj;
			return *this;
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator*(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value * rhs);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr fixed_point_t& operator*=(T const& obj) {
			value *= obj;
			return *this;
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator+(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value + (static_cast<value_type>(rhs) << PRECISION));
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr std::strong_ordering operator<=>(T const& rhs) const {
			return value <=> static_cast<value_type>(rhs) << PRECISION;
		}

		OV_SPEED_INLINE constexpr bool is_negative() const {
			return value < 0;
		}

		OV_SPEED_INLINE constexpr fixed_point_t abs() const {
			return !is_negative() ? parse_raw(value) : parse_raw(-value);
		}

		OV_SPEED_INLINE constexpr bool is_integer() const {
			return get_frac() == 0;
		}

		template<std::integral T>
		OV_SPEED_INLINE constexpr T unsafe_truncate() const {
			if constexpr(std::unsigned_integral<T>) {
				assert(OV_likely(!is_negative()));
			}
			return value >> PRECISION;
		}

		OV_SPEED_INLINE void warn_if_truncated() const;

		template<std::integral T>
		OV_SPEED_INLINE explicit constexpr operator T() const {
			#ifdef DEV_ENABLED
			if (!std::is_constant_evaluated()) {
				warn_if_truncated();
			}
			#endif
			return unsafe_truncate<T>();
		}

		OV_SPEED_INLINE constexpr fixed_point_t truncate() const {
			return parse_raw(value & ~FRAC_MASK);
		}

		template<std::integral T>
		OV_SPEED_INLINE constexpr T truncate() const {
			return static_cast<T>(*this);
		}

		template<std::floating_point T>
		OV_SPEED_INLINE explicit constexpr operator T() const {
			return value / static_cast<T>(ONE);
		}

		template<std::integral T>
		OV_SPEED_INLINE constexpr T floor() const {
			if (!is_negative()) {
				return unsafe_truncate<T>();
			}

			return parse_raw(value + FRAC_MASK).unsafe_truncate<T>() - !is_integer();
		}

		OV_SPEED_INLINE constexpr fixed_point_t floor() const {
			if (!is_negative()) {
				return truncate();
			}

			return floor<int32_t>();
		}

		template<std::integral T>
		OV_SPEED_INLINE constexpr T ceil() const {
			if (is_negative()) {
				return unsafe_truncate<T>();
			}

			return unsafe_truncate<T>() + !is_integer();
		}

		OV_SPEED_INLINE constexpr fixed_point_t ceil() const {
			if (is_negative()) {
				return truncate();
			}

			return ceil<int32_t>();
		}

		OV_SPEED_INLINE constexpr fixed_point_t round() const {
			if (is_negative()) {
				return (*this - _0_50).truncate();
			}

			return (*this + _0_50).truncate();
		}

		template<typename T>
		OV_SPEED_INLINE constexpr T round() const;

		/* WARNING: the results of these rounding functions are affected by the accuracy of base 2 fixed point numbers,
		 * for example 1.0 rounded to a multiple of 0.01 is 0.99945068359375 for down and 1.0094451904296875 for up. */
		template<midpoint_rounding Preference>
		OV_SPEED_INLINE constexpr fixed_point_t round(fixed_point_t factor) const {
			if constexpr (Preference == midpoint_rounding::AWAY_ZERO) {
				const fixed_point_t remainder = *this % factor;
				return *this - remainder - (remainder < 0 ? factor : _0);
			} else {
				const fixed_point_t remainder = *this % factor;
				return *this - remainder + (remainder > 0 ? factor : _0);
			}
		}

		/* WARNING: the results of these rounding functions are affected by the accuracy of base 2 fixed point numbers,
		 * for example 1.0 rounded to a multiple of 0.01 is 0.99945068359375 for down and 1.0094451904296875 for up. */
		OV_SPEED_INLINE constexpr fixed_point_t round(fixed_point_t factor, midpoint_rounding preference) const {
			switch (preference) {
			case midpoint_rounding::AWAY_ZERO: return round<midpoint_rounding::AWAY_ZERO>(factor);
			case midpoint_rounding::TO_ZERO:   return round<midpoint_rounding::TO_ZERO>(factor);
			}
		}

		// Rounds away from zero on tie
		template<std::integral T>
		OV_SPEED_INLINE constexpr T round() const {
			if (is_negative()) {
				return (*this - _0_50).unsafe_truncate<T>();
			}

			return (*this + _0_50).unsafe_truncate<T>();
		}

		template<std::floating_point T>
		OV_SPEED_INLINE constexpr T round() const {
			return static_cast<T>(round_to_int64<T>((value / static_cast<T>(ONE)) * T { 100000 })) / T { 100000 };
		}

		// Deterministic
		OV_ALWAYS_INLINE static constexpr fixed_point_t parse_raw(value_type value) {
			return fixed_point_t { raw_value, value };
		}

		OV_SPEED_INLINE static constexpr fixed_point_t parse_capped(const int32_t value) { return fixed_point_t(value); }

		template<std::integral T>
		requires (sizeof(T) < 4)
		OV_SPEED_INLINE static constexpr  fixed_point_t parse_capped(const T value) { return fixed_point_t(static_cast<int32_t>(value)); }

		static fixed_point_t parse_capped(const int64_t value);
		static fixed_point_t parse_capped(const uint64_t value);
		
		template<std::integral T>
		requires (sizeof(T) >= 4)
		static fixed_point_t parse_capped(const T value) {
			return std::is_signed_v<T>
				? parse_capped(static_cast<int64_t>(value))
				: parse_capped(static_cast<uint64_t>(value));
		}

		// Not Deterministic
		OV_SPEED_INLINE static constexpr fixed_point_t parse_unsafe(float value) {
			return parse_raw(value * ONE + 0.5f * (value < 0 ? -1 : 1));
		}

		friend std::ostream& operator<<(std::ostream& stream, fixed_point_t const& obj);

		OV_SPEED_INLINE constexpr friend fixed_point_t operator-(fixed_point_t const& obj) {
			return parse_raw(-obj.value);
		}

		OV_SPEED_INLINE constexpr friend fixed_point_t operator+(fixed_point_t const& obj) {
			return parse_raw(+obj.value);
		}

		OV_SPEED_INLINE constexpr friend fixed_point_t operator+(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value + rhs.value);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator+(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << PRECISION) + rhs.value);
		}

		OV_SPEED_INLINE constexpr fixed_point_t& operator+=(fixed_point_t const& obj) {
			value += obj.value;
			return *this;
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr fixed_point_t& operator+=(T const& obj) {
			value += (static_cast<value_type>(obj) << PRECISION);
			return *this;
		}

		OV_SPEED_INLINE constexpr friend fixed_point_t operator-(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value - rhs.value);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator-(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value - (static_cast<value_type>(rhs) << PRECISION));
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator-(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << PRECISION) - rhs.value);
		}

		OV_SPEED_INLINE constexpr fixed_point_t& operator-=(fixed_point_t const& obj) {
			value -= obj.value;
			return *this;
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr fixed_point_t& operator-=(T const& obj) {
			value -= (static_cast<value_type>(obj) << PRECISION);
			return *this;
		}

		OV_SPEED_INLINE constexpr fixed_point_t& operator++() {
			value += ONE;
			return *this;
		}

		OV_SPEED_INLINE constexpr fixed_point_t operator++(int) {
			const fixed_point_t old = *this;
			value += ONE;
			return old;
		}

		OV_SPEED_INLINE constexpr fixed_point_t& operator--() {
			value -= ONE;
			return *this;
		}

		OV_SPEED_INLINE constexpr fixed_point_t operator--(int) {
			const fixed_point_t old = *this;
			value -= ONE;
			return old;
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator<<(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value << rhs);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator>>(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value >> rhs);
		}

		OV_SPEED_INLINE constexpr friend fixed_point_t operator*(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value * rhs.value >> PRECISION);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator*(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs * rhs.value);
		}

		OV_SPEED_INLINE constexpr fixed_point_t& operator*=(fixed_point_t const& obj) {
			value *= obj.value;
			value >>= PRECISION;
			return *this;
		}

		OV_SPEED_INLINE constexpr friend fixed_point_t operator/(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw((lhs.value << PRECISION) / rhs.value);
		}

		OV_SPEED_INLINE constexpr fixed_point_t& operator/=(fixed_point_t const& obj) {
			value = (value << PRECISION) / obj.value;
			return *this;
		}

		OV_SPEED_INLINE constexpr friend fixed_point_t operator%(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return parse_raw(lhs.value % rhs.value);
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator%(fixed_point_t const& lhs, T const& rhs) {
			return parse_raw(lhs.value % (static_cast<value_type>(rhs) << PRECISION));
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr friend fixed_point_t operator%(T const& lhs, fixed_point_t const& rhs) {
			return parse_raw((static_cast<value_type>(lhs) << PRECISION) % rhs.value);
		}

		OV_SPEED_INLINE constexpr fixed_point_t& operator%=(fixed_point_t const& obj) {
			value %= obj.value;
			return *this;
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr fixed_point_t& operator%=(T const& obj) {
			value %= (static_cast<value_type>(obj) << PRECISION);
			return *this;
		}

		OV_SPEED_INLINE friend constexpr bool operator==(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value == rhs.value;
		}

		template<integral_max_size_4 T>
		OV_SPEED_INLINE constexpr bool operator==(T const& rhs) const {
			return value == static_cast<value_type>(rhs) << PRECISION;
		}

		template<std::floating_point T>
		OV_SPEED_INLINE constexpr bool operator==(T const& rhs) const {
			return static_cast<T>(*this) == rhs;
		}

		OV_SPEED_INLINE friend constexpr std::strong_ordering operator<=>(fixed_point_t const& lhs, fixed_point_t const& rhs) {
			return lhs.value <=> rhs.value;
		}

		template<std::floating_point T>
		OV_SPEED_INLINE constexpr std::partial_ordering operator<=>(T const& rhs) const {
			return static_cast<T>(*this) <=> rhs;
		}

	public:
		static fixed_point_t exp(fixed_point_t const& x);
		static fixed_point_t exp_2001(fixed_point_t const& x);
	};

	static_assert(sizeof(fixed_point_t) == fixed_point_t::SIZE, "fixed_point_t is not 8 bytes");

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
