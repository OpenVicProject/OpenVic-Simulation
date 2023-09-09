#include <cstdint>
#include <limits>
#include <string_view>

namespace OpenVic::StringUtils {
	/* The constexpr function 'string_to_uint64' will convert a string into a uint64_t integer value.
	 * The function takes four parameters: the input string (as a pair of pointers marking the start and
	 * end of the string), a bool pointer for reporting success, and the base for numerical conversion.
	 * The base parameter defaults to 10 (decimal), but it can be any value between 2 and 36. If the base
	 * given is 0, it will be set to 16 if the string starts with "0x" or "0X", otherwise 8 if the string
	 * still starts with "0", otherwise 10. The success bool pointer parameter is used to report whether
	 * or not conversion was successful. It can be nullptr if this information is not needed.
	 */
	constexpr uint64_t string_to_uint64(char const* str, const char* const end, bool* successful = nullptr, int base = 10) {
		if (successful != nullptr) *successful = false;

		// Base value should be between 2 and 36. If it's not, return 0 as an invalid case.
		if (str == nullptr || end <= str || base < 0 || base == 1 || base > 36)
			return 0;

		// The result of the conversion will be stored in this variable.
		uint64_t result = 0;

		// If base is zero, base is determined by the string prefix.
		if (base == 0) {
			if (*str == '0') {
				if (str + 1 != end && (str[1] == 'x' || str[1] == 'X')) {
					base = 16; // Hexadecimal.
					str += 2;  // Skip '0x' or '0X'
					if (str == end) return 0;
				} else {
					base = 8; // Octal.
				}
			} else {
				base = 10; // Decimal.
			}
		} else if (base == 16) {
			// If base is 16 and string starts with '0x' or '0X', skip these characters.
			if (*str == '0' && str + 1 != end && (str[1] == 'x' || str[1] == 'X')) {
				str += 2;
				if (str == end) return 0;
			}
		}

		// Convert the number in the string.
		for (; str != end; ++str) {
			int digit;
			if (*str >= '0' && *str <= '9') {
				digit = *str - '0'; // Calculate digit value for '0'-'9'.
			} else if (*str >= 'a' && *str <= 'z') {
				digit = *str - 'a' + 10; // Calculate digit value for 'a'-'z'.
			} else if (*str >= 'A' && *str <= 'Z') {
				digit = *str - 'A' + 10; // Calculate digit value for 'A'-'Z'.
			} else {
				break; // Stop conversion if current character is not a digit.
			}

			if (digit >= base) {
				break; // Stop conversion if current digit is greater than or equal to the base.
			}

			// Check for overflow on multiplication
			if (result > std::numeric_limits<uint64_t>::max() / base) {
				return std::numeric_limits<uint64_t>::max();
			}

			result *= base;

			// Check for overflow on addition
			if (result > std::numeric_limits<uint64_t>::max() - digit) {
				return std::numeric_limits<uint64_t>::max();
			}

			result += digit;
		}

		// If successful is not null and the entire string was parsed,
		// set *successful to true (if not it is already false).
		if (successful != nullptr && str == end) *successful = true;

		return result;
	}

	constexpr uint64_t string_to_uint64(char const* str, size_t length, bool* successful = nullptr, int base = 10) {
		return string_to_uint64(str, str + length, successful, base);
	}

	inline uint64_t string_to_uint64(const std::string_view str, bool* successful = nullptr, int base = 10) {
		return string_to_uint64(str.data(), str.length(), successful, base);
	}

	constexpr int64_t string_to_int64(char const* str, const char* const end, bool* successful = nullptr, int base = 10) {
		if (successful != nullptr) *successful = false;

		if (str == nullptr || end <= str) return 0;

		// This flag will be set if the number is negative.
		bool is_negative = false;

		// Check if there is a sign character.
		if (*str == '+' || *str == '-') {
			if (*str == '-')
				is_negative = true;
			++str;
			if (str == end) return 0;
		}

		const uint64_t result = string_to_uint64(str, end, successful, base);
		if (!is_negative) {
			if (result >= std::numeric_limits<int64_t>::max())
				return std::numeric_limits<int64_t>::max();
			return result;
		} else {
			if (result > std::numeric_limits<int64_t>::max())
				return std::numeric_limits<int64_t>::min();
			return -result;
		}
	}

	constexpr int64_t string_to_int64(char const* str, size_t length, bool* successful = nullptr, int base = 10) {
		return string_to_int64(str, str + length, successful, base);
	}

	inline int64_t string_to_int64(const std::string_view str, bool* successful = nullptr, int base = 10) {
		return string_to_int64(str.data(), str.length(), successful, base);
	}
}
