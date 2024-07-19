#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace OpenVic::StringUtils {
	/* The constexpr function 'string_to_uint64' will convert a string into a uint64_t integer value.
	 * The function takes four parameters: the input string (as a pair of pointers marking the start and
	 * end of the string), a bool pointer for reporting success, and the base for numerical conversion.
	 * The base parameter defaults to 10 (decimal), but it can be any value between 2 and 36. If the base
	 * given is 0, it will be set to 16 if the string starts with "0x" or "0X", otherwise 8 if the string
	 * still starts with "0", otherwise 10. The success bool pointer parameter is used to report whether
	 * or not conversion was successful. It can be nullptr if this information is not needed.
	 */
	constexpr uint64_t string_to_uint64(char const* str, char const* const end, bool* successful = nullptr, int base = 10) {
		if (successful != nullptr) {
			*successful = false;
		}

		// Base value should be between 2 and 36. If it's not, return 0 as an invalid case.
		if (str == nullptr || end <= str || base < 0 || base == 1 || base > 36) {
			return 0;
		}

		// The result of the conversion will be stored in this variable.
		uint64_t result = 0;

		// If base is zero, base is determined by the string prefix.
		if (base == 0) {
			if (*str == '0') {
				if (str + 1 != end && (str[1] == 'x' || str[1] == 'X')) {
					base = 16; // Hexadecimal.
					str += 2; // Skip '0x' or '0X'
					if (str == end) {
						return 0;
					}
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
				if (str == end) {
					return 0;
				}
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
		if (successful != nullptr && str == end) {
			*successful = true;
		}

		return result;
	}

	inline constexpr uint64_t string_to_uint64(char const* str, size_t length, bool* successful = nullptr, int base = 10) {
		return string_to_uint64(str, str + length, successful, base);
	}

	inline constexpr uint64_t string_to_uint64(std::string_view str, bool* successful = nullptr, int base = 10) {
		return string_to_uint64(str.data(), str.length(), successful, base);
	}

	constexpr int64_t string_to_int64(char const* str, char const* const end, bool* successful = nullptr, int base = 10) {
		if (successful != nullptr) {
			*successful = false;
		}

		if (str == nullptr || end <= str) {
			return 0;
		}

		// This flag will be set if the number is negative.
		bool is_negative = false;

		// Check if there is a sign character.
		if (*str == '+' || *str == '-') {
			if (*str == '-') {
				is_negative = true;
			}
			++str;
			if (str == end) {
				return 0;
			}
		}

		const uint64_t result = string_to_uint64(str, end, successful, base);
		if (!is_negative) {
			if (result >= std::numeric_limits<int64_t>::max()) {
				return std::numeric_limits<int64_t>::max();
			}
			return result;
		} else {
			if (result > std::numeric_limits<int64_t>::max()) {
				return std::numeric_limits<int64_t>::min();
			}
			return -result;
		}
	}

	inline constexpr int64_t string_to_int64(char const* str, size_t length, bool* successful = nullptr, int base = 10) {
		return string_to_int64(str, str + length, successful, base);
	}

	inline constexpr int64_t string_to_int64(std::string_view str, bool* successful = nullptr, int base = 10) {
		return string_to_int64(str.data(), str.length(), successful, base);
	}

	inline constexpr bool strings_equal_case_insensitive(std::string_view const& lhs, std::string_view const& rhs) {
		if (lhs.size() != rhs.size()) {
			return false;
		}
		constexpr auto ichar_equals = [](unsigned char l, unsigned char r) {
			return std::tolower(l) == std::tolower(r);
		};
		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), ichar_equals);
	}

	inline constexpr std::string_view get_filename(std::string_view path) {
		size_t pos = path.size();
		while (pos > 0 && path[pos - 1] != '/' && path[pos - 1] != '\\') {
			--pos;
		}
		path.remove_prefix(pos);
		return path;
	}

	inline constexpr char const* get_filename(char const* filepath, char const* default_path = nullptr) {
		const std::string_view filename { get_filename(std::string_view { filepath }) };
		if (!filename.empty()) {
			return filename.data();
		}
		return default_path;
	}

	inline std::string make_forward_slash_path(std::string_view path) {
		std::string ret { path };
		std::replace(ret.begin(), ret.end(), '\\', '/');
		for (char& c : ret) {
			if (c == '\\') {
				c = '/';
			}
		}
		return ret;
	}

	inline constexpr std::string_view remove_leading_slashes(std::string_view path) {
		size_t count = 0;
		while (count < path.size() && (path[count] == '/' || path[count] == '\\')) {
			++count;
		}
		path.remove_prefix(count);
		return path;
	}

	template<typename... Args>
	requires(std::is_same_v<std::string_view, Args> && ...)
	inline std::string _append_string_views(Args... args) {
		std::string ret;
		ret.reserve((args.size() + ...));
		(ret.append(args), ...);
		return ret;
	}

	template<typename... Args>
	requires(std::is_convertible_v<Args, std::string_view> && ...)
	inline std::string append_string_views(Args... args) {
		return _append_string_views(std::string_view { args }...);
	}

	inline constexpr size_t get_extension_pos(std::string_view const& path) {
		size_t pos = path.size();
		while (pos > 0 && path[--pos] != '.') {}
		return pos;
	}

	inline constexpr std::string_view get_extension(std::string_view path) {
		if (!path.empty()) {
			const size_t pos = get_extension_pos(path);
			if (path[pos] == '.') {
				path.remove_prefix(pos);
				return path;
			}
		}
		return {};
	}

	inline constexpr std::string_view remove_extension(std::string_view path) {
		if (!path.empty()) {
			const size_t pos = get_extension_pos(path);
			if (path[pos] == '.') {
				path.remove_suffix(path.size() - pos);
			}
		}
		return path;
	}
}
