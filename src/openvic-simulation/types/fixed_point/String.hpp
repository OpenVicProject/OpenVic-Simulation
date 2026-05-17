#pragma once

#include <charconv>

#include "openvic-simulation/core/Math.hpp"
#include "openvic-simulation/core/string/CharConv.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/StackString.hpp"
#include "openvic-simulation/utility/Logger.hpp"

namespace OpenVic::fp {
	struct stack_string final : public StackString<25> {
	protected:
		using StackString::StackString;
	public:
		constexpr char* front() {
			return _array.data();
		}

		constexpr char* back() {
			return _array.data() + _array.size();
		}

		constexpr uint8_t& string_size() {
			return _string_size;
		}
	};

	OV_SPEED_INLINE static constexpr std::to_chars_result to_chars(
		const fixed_point_t v,
		char* first,
		char* last,
		size_t decimal_places = -1
	) {
		if (first == nullptr || first >= last) {
			return { last, std::errc::value_too_large };
		}

		if (v.is_negative()) {
			*first = '-';
			++first;
			if (last - first <= 1) {
				return { last, std::errc::value_too_large };
			}
		}

		std::to_chars_result result {};
		if (decimal_places == static_cast<size_t>(-1)) {
			result = OpenVic::to_chars(first, last, v.abs().unsafe_truncate<int64_t>());
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			if (OV_unlikely(last - result.ptr <= 1)) {
				return result;
			}

			fixed_point_t frac = v.abs().get_frac();
			if (frac != fixed_point_t::_0) {
				*result.ptr = '.';
				++result.ptr;
				do {
					if (OV_unlikely(last - result.ptr <= 1)) {
						return { last, std::errc::value_too_large };
					}
					frac *= 10;
					*result.ptr = static_cast<char>('0' + frac.unsafe_truncate<int64_t>());
					++result.ptr;
					frac = frac.get_frac();
				} while (frac != fixed_point_t::_0);
			}
			return result;
		}

		// Add the specified number of decimal places, potentially 0 (so no decimal point)
		fixed_point_t err = fixed_point_t::_0_50;
		for (size_t i = decimal_places; i > 0; --i) {
			err /= 10;
		}
		fixed_point_t val = v.abs() + err;


		result = OpenVic::to_chars(first, last, val.unsafe_truncate<int64_t>());
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
				*result.ptr = static_cast<char>('0' + val.unsafe_truncate<int64_t>());
				++result.ptr;
				val = val.get_frac();
			} while (--decimal_places > 0);
		}
		return result;
	}

	OV_SPEED_INLINE static constexpr stack_string to_array(
		const fixed_point_t v,
		size_t decimal_places = -1
	) {
		stack_string str {};
		std::to_chars_result result = OpenVic::fp::to_chars(
			v,
			str.front(),
			str.back(),
			decimal_places
		);
		str.string_size() = result.ptr - str.data();
		return str;
	}

	// Deterministic
	OV_SPEED_INLINE static constexpr std::from_chars_result from_chars(
		fixed_point_t& v,
		char const* begin,
		char const* end
	) {
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
			int64_t parsed_value = 0;
			from_chars = string_to_int64(begin, dot_pointer, parsed_value);
			if (from_chars.ec == std::errc{}) {
				if (parsed_value > std::numeric_limits<int32_t>::max()) {
					from_chars.ec = std::errc::value_too_large;
				} else {
					result = fixed_point_t(static_cast<int32_t>(parsed_value));
				}
			}
		}

		if (from_chars.ec != std::errc{}) {
			return from_chars;
		}

		if (dot_pointer + 1 < end) {
			// Non-empty fractional part, cannot be negative
			fixed_point_t adder;
			char const* fraction_begin = dot_pointer + 1;
			if (fraction_begin && *fraction_begin == '-') {
				from_chars = { fraction_begin, std::errc::invalid_argument };
			} else {
				end = end - fraction_begin > fixed_point_t::PRECISION ? fraction_begin + fixed_point_t::PRECISION : end;
				uint64_t parsed_value;
				from_chars = string_to_uint64(fraction_begin, end, parsed_value);
				if (from_chars.ec == std::errc{}) {
					for (ptrdiff_t remaining_shift = fixed_point_t::PRECISION - (end - fraction_begin); remaining_shift > 0; remaining_shift--) {
						parsed_value *= 10;
					}
					uint64_t decimal = OpenVic::pow(static_cast<uint64_t>(10), fixed_point_t::PRECISION);
					int64_t ret = 0;
					for (int i = fixed_point_t::PRECISION - 1; i >= 0; --i) {
						decimal >>= 1;
						if (parsed_value >= decimal) {
							parsed_value -= decimal;
							ret |= 1 << i;
						}
					}
					adder = fixed_point_t::parse_raw(ret);
				}
			}

			result += result.is_negative() || (*begin == '-' && result == fixed_point_t::_0) ? -adder : adder;
		}

		if (from_chars.ec != std::errc{}) {
			return { begin, from_chars.ec };
		}

		v = result;
		return from_chars;
	}

	// Deterministic
	OV_SPEED_INLINE static constexpr std::from_chars_result from_chars_with_plus(
		fixed_point_t& v,
		char const* begin,
		char const* end
	) {
		if (begin && *begin == '+') {
			begin++;
			if (begin < end && *begin == '-') {
				return std::from_chars_result { begin, std::errc::invalid_argument };
			}
		}

		return from_chars(v, begin, end);
	}

	// Not Deterministic
	OV_SPEED_INLINE static fixed_point_t parse_unsafe(char const* value) {
		char* endpointer;
		double double_value = std::strtod(value, &endpointer);

		if (*endpointer != '\0') {
			spdlog::error_s("Unsafe fixed point parse failed to parse the end of a string: \"{}\"", endpointer);
		}

		fixed_point_t::value_type integer_value = static_cast<long>(double_value * fixed_point_t::ONE + 0.5 * (double_value < 0 ? -1 : 1));

		return fixed_point_t::parse_raw(integer_value);
	}
}