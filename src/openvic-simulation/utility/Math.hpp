#pragma once

#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include "openvic-simulation/utility/Typedefs.hpp"

namespace OpenVic {
    template<typename T>
    [[nodiscard]] OV_SPEED_INLINE constexpr T abs(T num);

	template<typename T>
	requires std::integral<T> || std::floating_point<T>
	[[nodiscard]] OV_SPEED_INLINE constexpr T abs(T num) {
		if (std::is_constant_evaluated()) {
			return num < 0 ? -num : num;
		} else {
			return std::abs(num);
		}
	}

    template<typename T>
	requires (!(std::integral<T> || std::floating_point<T>))
	[[nodiscard]] OV_SPEED_INLINE constexpr T abs(T num);

	template<std::floating_point T>
	OV_SPEED_INLINE constexpr int64_t round_to_int64(T num) {
		return (num > T { 0 }) ? (num + (T { 1 } / 2)) : (num - (T { 1 } / 2));
	}

	OV_SPEED_INLINE constexpr uint64_t pow(uint64_t base, std::size_t exponent) {
		uint64_t ret = 1;
		for (;; base *= base) {
			if ((exponent & 1) != 0) {
				ret *= base;
			}
			exponent >>= 1;
			if (exponent == 0) {
				break;
			}
		}
		return ret;
	}

	OV_SPEED_INLINE constexpr int64_t pow(int64_t base, std::size_t exponent) {
		int64_t ret = 1;
		for (;; base *= base) {
			if ((exponent & 1) != 0) {
				ret *= base;
			}
			exponent >>= 1;
			if (exponent == 0) {
				break;
			}
		}
		return ret;
	}

	OV_SPEED_INLINE constexpr uint64_t sqrt(uint64_t n) {
		uint64_t x = n;
		uint64_t c = 0;
		uint64_t d = 1ull << 62;

		while (d > n) {
			d >>= 2;
		}

		for (; d != 0; d >>= 2) {
			if (x >= c + d) {
				x -= c + d;
				c = (c >> 1) + d;
			} else {
				c >>= 1;
			}
		}

		// round up
		if (x > 0) {
			c += 1;
		}

		return c;
	}

	OV_SPEED_INLINE constexpr bool is_power_of_two(uint64_t n) {
		return n > 0 && (n & (n - 1)) == 0;
	}
}
