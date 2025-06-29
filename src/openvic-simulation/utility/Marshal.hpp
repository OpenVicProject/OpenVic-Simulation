#pragma once

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/ErrorMacros.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic::utility {
	OV_ALWAYS_INLINE uint32_t halfbits_to_floatbits(uint16_t p_half) {
		uint16_t h_exp, h_sig;
		uint32_t f_sgn, f_exp, f_sig;

		h_exp = (p_half & 0x7c00u);
		f_sgn = ((uint32_t)p_half & 0x8000u) << 16;
		switch (h_exp) {
		case 0x0000u: /* 0 or subnormal */
			h_sig = (p_half & 0x03ffu);
			/* Signed zero */
			if (h_sig == 0) {
				return f_sgn;
			}
			/* Subnormal */
			h_sig <<= 1;
			while ((h_sig & 0x0400u) == 0) {
				h_sig <<= 1;
				h_exp++;
			}
			f_exp = ((uint32_t)(127 - 15 - h_exp)) << 23;
			f_sig = ((uint32_t)(h_sig & 0x03ffu)) << 13;
			return f_sgn + f_exp + f_sig;
		case 0x7c00u: /* inf or NaN */
			/* All-ones exponent and a copy of the significand */
			return f_sgn + 0x7f800000u + (((uint32_t)(p_half & 0x03ffu)) << 13);
		default: /* normalized */
			/* Just need to adjust the exponent and shift */
			return f_sgn + (((uint32_t)(p_half & 0x7fffu) + 0x1c000u) << 13);
		}
	}

	OV_ALWAYS_INLINE float halfptr_to_float(const uint16_t* p_half) {
		return std::bit_cast<float>(halfbits_to_floatbits(*p_half));
	}

	OV_ALWAYS_INLINE float half_to_float(const uint16_t p_half) {
		return halfptr_to_float(&p_half);
	}

	OV_ALWAYS_INLINE uint16_t make_half_float(float p_value) {
		uint32_t x = std::bit_cast<uint32_t>(p_value);
		uint32_t sign = (unsigned short)(x >> 31);
		uint32_t mantissa;
		uint32_t exponent;
		uint16_t hf;

		// get mantissa
		mantissa = x & ((1 << 23) - 1);
		// get exponent bits
		exponent = x & (0xFF << 23);
		if (exponent >= 0x47800000) {
			// check if the original single precision float number is a NaN
			if (mantissa && (exponent == (0xFF << 23))) {
				// we have a single precision NaN
				mantissa = (1 << 23) - 1;
			} else {
				// 16-bit half-float representation stores number as Inf
				mantissa = 0;
			}
			hf = (((uint16_t)sign) << 15) | (uint16_t)((0x1F << 10)) | (uint16_t)(mantissa >> 13);
		}
		// check if exponent is <= -15
		else if (exponent <= 0x38000000) {
			/*
			// store a denorm half-float value or zero
			exponent = (0x38000000 - exponent) >> 23;
			mantissa >>= (14 + exponent);

			hf = (((uint16_t)sign) << 15) | (uint16_t)(mantissa);
			*/
			hf = 0; // denormals do not work for 3D, convert to zero
		} else {
			hf = (((uint16_t)sign) << 15) | (uint16_t)((exponent - 0x38000000) >> 13) | (uint16_t)(mantissa >> 13);
		}

		return hf;
	}

	struct half_float {
		float value;
	};

	static_assert(std::endian::native != std::endian::big || std::endian::native != std::endian::little, "HOW?!?!?!");

	template<typename T, std::endian Endian = std::endian::native>
	static inline size_t encode(T value, std::span<uint8_t> span = {}) {
		if constexpr (std::is_const_v<T>) {
			return encode<std::remove_const_t<T>, Endian>(value, span);
		} else if constexpr (std::integral<T>) {
			using unsigned_type = std::make_unsigned_t<T>;

			if (!span.empty()) {
				if constexpr (sizeof(T) == 1) {
					*span.data() = *std::bit_cast<unsigned_type*>(&value);
				} else {
					if constexpr (std::endian::native != Endian) {
						value = std::bit_cast<T>(utility::byteswap(std::bit_cast<unsigned_type>(value)));
					}
					decltype(span)::pointer ptr = span.data();
					for (size_t i = 0; i < sizeof(T); i++) {
						*ptr = *std::bit_cast<unsigned_type*>(&value) & 0xFF;
						ptr++;
						*std::bit_cast<unsigned_type*>(&value) >>= 8;
					}
				}
				return span.size() >= sizeof(T) ? sizeof(T) : 0;
			}

			return sizeof(T);
		} else if constexpr (std::floating_point<T>) {
			using int_type = std::conditional_t<std::same_as<T, float>, uint32_t, uint64_t>;

			return encode<int_type, Endian>(std::bit_cast<int_type>(value), span);
		} else if constexpr (std::convertible_to<T, std::string_view>) {
			const std::string_view str = static_cast<std::string_view>(value);
			const size_t str_size = str.size();
			const size_t length_size = encode<uint32_t, Endian>(str_size);
			const size_t size = str_size + length_size;

			if (span.empty()) {
				return size;
			} else if (span.size() < size) {
				return 0;
			}

			uint8_t* ptr = span.data() + encode<uint32_t, Endian>(str_size, span);
			std::copy_n(reinterpret_cast<uint8_t const*>(str.data()), str_size, ptr);

			return size;
		} else if constexpr (std::same_as<T, half_float>) {
			return encode<uint16_t, Endian>(make_half_float(value.value), span);
		} else if constexpr (std::same_as<T, Timespan>) {
			return encode<int64_t, Endian>(value.to_int(), span);
		} else if constexpr (std::same_as<T, Date>) {
			return encode<Timespan, Endian>(value.get_timespan(), span);
		} else if constexpr (IsColour<T>) {
			return encode<typename T::integer_type, Endian>(static_cast<typename T::integer_type>(value), span);
		} else if constexpr (std::same_as<T, fixed_point_t>) {
			return encode<typename T::value_type, Endian>(value.get_raw_value(), span);
		} else if constexpr (std::convertible_to<T, std::span<const uint8_t>>) {
			const std::span<const uint8_t> inner_span = static_cast<std::span<const uint8_t>>(value);
			const size_t span_size = inner_span.size();
			const size_t length_size = encode<uint32_t, Endian>(span_size);
			const size_t size = span_size + length_size;

			if (span.empty()) {
				return size;
			} else if (span.size() < size) {
				return 0;
			}

			uint8_t* ptr = span.data() + encode<uint32_t, Endian>(span_size, span);
			std::copy_n(reinterpret_cast<uint8_t const*>(inner_span.data()), span_size, ptr);

			return size;
		} else {
			static_assert(!std::same_as<T, T>);
		}
	}

	template<typename T, std::endian Endian = std::endian::native>
	static inline T decode(std::span<uint8_t> span, size_t& r_decode_count) {
		using result_type = std::remove_cv_t<T>;

		if constexpr (std::integral<T>) {
			using unsigned_type = std::make_unsigned_t<result_type>;

			r_decode_count = sizeof(T);
			if (OV_unlikely(span.size() < r_decode_count)) {
				r_decode_count = 0;
				OV_ERR_FAIL_V({});
			}

			if constexpr (sizeof(result_type) == 1) {
				return std::bit_cast<result_type>(span[0]);
			} else {
				decltype(span)::pointer ptr = span.data();
				result_type u = 0;
				for (int i = 0; i < r_decode_count; i++) {
					result_type b = *ptr;
					b <<= (i * 8);
					u |= b;
					ptr++;
				}
				if constexpr (std::endian::native != Endian) {
					u = utility::byteswap(u);
				}
				return u;
			}
		} else if constexpr (std::floating_point<T>) {
			using int_type = std::conditional_t<std::same_as<result_type, float>, uint32_t, uint64_t>;

			return std::bit_cast<result_type>(decode<int_type, Endian>(span, r_decode_count));
		} else if constexpr (std::convertible_to<std::string_view, T>) {
			uint32_t size = decode<uint32_t, Endian>(span, r_decode_count);
			OV_ERR_FAIL_INDEX_V(size, span.size(), {});
			std::string_view view { reinterpret_cast<std::string_view::value_type*>(span.data() + r_decode_count), size };
			r_decode_count += size;
			return static_cast<result_type>(view);
		} else if constexpr (std::is_constructible_v<T, std::string_view>) {
			return result_type { decode<std::string_view, Endian>(span, r_decode_count) };
		} else if constexpr (std::same_as<T, half_float>) {
			return { half_to_float(decode<uint16_t, Endian>(span, r_decode_count)) };
		} else if constexpr (std::same_as<T, Timespan>) {
			return result_type { decode<int64_t, Endian>(span, r_decode_count) };
		} else if constexpr (std::same_as<T, Date>) {
			return result_type { decode<Timespan, Endian>(span, r_decode_count) };
		} else if constexpr (IsColour<T>) {
			return result_type::from_integer(decode<typename result_type::integer_type, Endian>(span, r_decode_count));
		} else if constexpr (std::same_as<T, fixed_point_t>) {
			return result_type::parse_raw(decode<typename result_type::value_type, Endian>(span, r_decode_count));
		} else if constexpr (std::is_constructible_v<T, uint8_t*, uint8_t*>) {
			uint32_t size = decode<uint32_t, Endian>(span, r_decode_count);
			OV_ERR_FAIL_INDEX_V(size, span.size(), {});
			result_type value //
				{ //
				  reinterpret_cast<uint8_t*>(span.data() + r_decode_count), //
				  reinterpret_cast<uint8_t*>(span.data() + r_decode_count + size)
				};
			r_decode_count += size;
			return value;
		} else {
			static_assert(!std::same_as<T, T>);
		}
	}
}
