#pragma once

#include <cstdint>

namespace OpenVic {
	inline constexpr uint64_t sqrt(uint64_t n) {
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
}