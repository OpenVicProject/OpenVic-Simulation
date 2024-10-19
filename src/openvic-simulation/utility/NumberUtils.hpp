#include <cstdint>

namespace OpenVic::NumberUtils {
	constexpr int64_t round_to_int64(float num) {
		return (num > 0.0f) ? (num + 0.5f) : (num - 0.5f);
	}

	constexpr int64_t round_to_int64(double num) {
		return (num > 0.0) ? (num + 0.5) : (num - 0.5);
	}

	constexpr uint64_t pow(uint64_t base, std::size_t exponent) {
		uint64_t ret = 1;
		while (exponent-- > 0) {
			ret *= base;
		}
		return ret;
	}

	constexpr int64_t pow(int64_t base, std::size_t exponent) {
		int64_t ret = 1;
		while (exponent-- > 0) {
			ret *= base;
		}
		return ret;
	}

	constexpr uint64_t sqrt(uint64_t n) {
		uint64_t x = n;
		uint64_t c = 0;
		uint64_t d = 1ull << 62;

		while (d > n)
			d >>= 2;

		for (; d != 0; d >>= 2) {
			if (x >= c + d) {
				x -= c + d;
				c = (c >> 1) + d;
			} else {
				c >>= 1;
			}
		}

		//round up
		if (x > 0) {
			c += 1;
		}

		return c;
	}

	constexpr bool is_power_of_two(uint64_t n) {
		return n > 0 && (n & (n - 1)) == 0;
	}
}
