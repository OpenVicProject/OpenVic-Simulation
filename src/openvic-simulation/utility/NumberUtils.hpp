#include <cstdint>

namespace OpenVic::NumberUtils {
	constexpr int64_t round_to_int64(float num) {
		return (num > 0.0f) ? (num + 0.5f) : (num - 0.5f);
	}

	constexpr int64_t round_to_int64(double num) {
		return (num > 0.0) ? (num + 0.5) : (num - 0.5);
	}

	constexpr uint64_t pow(uint64_t base, size_t exponent) {
		uint64_t ret = 1;
		while (exponent-- > 0) {
			ret *= base;
		}
		return ret;
	}

	constexpr int64_t pow(int64_t base, size_t exponent) {
		int64_t ret = 1;
		while (exponent-- > 0) {
			ret *= base;
		}
		return ret;
	}
}
