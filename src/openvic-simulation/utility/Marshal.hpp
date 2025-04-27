#pragma once

#include <cstdint>
#include <span>
#include <string_view>

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

	template<typename T>
	static inline size_t encode(T p_int, std::span<uint8_t> span);

	template<std::integral T>
	static inline size_t encode(T p_int, std::span<uint8_t> span) {
		using unsigned_type = std::make_unsigned_t<T>;

		if constexpr (sizeof(T) == 1) {
			*span.data() = *std::bit_cast<unsigned_type*>(&p_int);
		} else {
			decltype(span)::pointer ptr = span.data();
			for (size_t i = 0; i < sizeof(T); i++) {
				*ptr = *std::bit_cast<unsigned_type*>(&p_int) & 0xFF;
				ptr++;
				*std::bit_cast<unsigned_type*>(&p_int) >>= 8;
			}
		}

		return sizeof(T);
	}

	template<std::floating_point T>
	static inline size_t encode(T p_float, std::span<uint8_t> span) {
		using int_type = std::conditional_t<std::same_as<T, float>, uint32_t, uint64_t>;

		return encode(std::bit_cast<int_type>(p_float), span);
	}

	template<>
	inline size_t encode(std::string_view str, std::span<uint8_t> span) {
		size_t len = 0;

		decltype(span)::pointer ptr = span.data();
		decltype(str)::const_pointer str_ptr = str.data();
		while (*str_ptr) {
			if (ptr) {
				*ptr = (uint8_t)*str_ptr;
				ptr++;
			}
			str_ptr++;
			len++;
		}

		if (ptr) {
			*ptr = 0;
		}
		return len + 1;
	}

	struct half_float {
		float value;
	};

	template<>
	inline size_t encode(half_float p_float, std::span<uint8_t> span) {
		return encode<uint16_t>(make_half_float(p_float.value), span);
	}

	template<typename T>
	static inline T decode(std::span<uint8_t> span);

	template<std::integral T>
	static inline T decode(std::span<uint8_t> span) {
		using unsigned_type = std::make_unsigned_t<T>;

		if constexpr (sizeof(T) == 1) {
			return std::bit_cast<T>(span[0]);
		} else {
			decltype(span)::pointer ptr = span.data();
			T u = 0;
			for (int i = 0; i < span.size(); i++) {
				T b = *ptr;
				b <<= (i * 8);
				u |= b;
				ptr++;
			}
			return u;
		}
	}

	template<std::floating_point T>
	static inline T decode(std::span<uint8_t> span) {
		using int_type = std::conditional_t<std::same_as<T, float>, uint32_t, uint64_t>;

		return std::bit_cast<T>(decode<int_type>(span));
	}

	template<>
	inline half_float decode<half_float>(std::span<uint8_t> span) {
		return { half_to_float(decode<uint16_t>(span)) };
	}

}
