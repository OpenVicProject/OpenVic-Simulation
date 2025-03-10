#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <range/v3/view/transform.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/range/conversion.hpp>
#include <tsl/ordered_map.h>

namespace OpenVic::StringUtils {
	template<typename T>
	[[nodiscard]] inline constexpr std::from_chars_result from_chars( //
		char const* const first, char const* const last, T& raw_value, const int base = 10
	) noexcept {
		constexpr auto digit_from_char = [](const char c) noexcept {
			// convert ['0', '9'] ['A', 'Z'] ['a', 'z'] to [0, 35], everything else to 255
			static constexpr unsigned char digit_from_byte[] = {
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 0,	 1,	  2,   3,	4,	 5,	  6,   7,	8,	 9,	  255, 255, 255, 255, 255, 255, 255, 10,
				11,	 12,  13,  14,	15,	 16,  17,  18,	19,	 20,  21,  22,	23,	 24,  25,  26,	27,	 28,  29,  30,	31,	 32,
				33,	 34,  35,  255, 255, 255, 255, 255, 255, 10,  11,  12,	13,	 14,  15,  16,	17,	 18,  19,  20,	21,	 22,
				23,	 24,  25,  26,	27,	 28,  29,  30,	31,	 32,  33,  34,	35,	 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
			};
			static_assert(std::size(digit_from_byte) == 256);

			return digit_from_byte[static_cast<unsigned char>(c)];
		};

		if (!std::is_constant_evaluated()) {
			return std::from_chars(first, last, raw_value, base);
		}

		bool minus_sign = false;

		const char* next = first;

		if constexpr (std::is_signed_v<T>) {
			if (next != last && *next == '-') {
				minus_sign = true;
				++next;
			}
		}

		using unsigned_t = std::make_unsigned_t<T>;

		[[maybe_unused]] constexpr unsigned_t uint_max = static_cast<unsigned_t>(-1);
		[[maybe_unused]] constexpr unsigned_t int_max = static_cast<unsigned_t>(uint_max >> 1);
		[[maybe_unused]] constexpr unsigned_t abs_int_min = static_cast<unsigned_t>(int_max + 1);

		unsigned_t risky_val;
		unsigned_t max_digit;

		if constexpr (std::is_signed_v<T>) {
			if (minus_sign) {
				risky_val = static_cast<unsigned_t>(abs_int_min / base);
				max_digit = static_cast<unsigned_t>(abs_int_min % base);
			} else {
				risky_val = static_cast<unsigned_t>(int_max / base);
				max_digit = static_cast<unsigned_t>(int_max % base);
			}
		} else {
			risky_val = static_cast<unsigned_t>(uint_max / base);
			max_digit = static_cast<unsigned_t>(uint_max % base);
		}

		unsigned_t value = 0;

		bool overflowed = false;

		for (; next != last; ++next) {
			const unsigned char digit = digit_from_char(*next);

			if (digit >= base) {
				break;
			}

			if (value < risky_val // never overflows
				|| (value == risky_val && digit <= max_digit)) { // overflows for certain digits
				value = static_cast<unsigned_t>(value * base + digit);
			} else { // value > risky_val always overflows
				overflowed = true; // keep going, next still needs to be updated, value is now irrelevant
			}
		}

		if (next - first == static_cast<std::ptrdiff_t>(minus_sign)) {
			return { first, std::errc::invalid_argument };
		}

		if (overflowed) {
			return { next, std::errc::result_out_of_range };
		}

		if constexpr (std::is_signed_v<T>) {
			if (minus_sign) {
				value = static_cast<unsigned_t>(0 - value);
			}
		}

		raw_value = static_cast<T>(value);

		return { next, std::errc {} };
	}

	/* The constexpr function 'string_to_uint64' will convert a string into a uint64_t integer value.
	 * The function takes four parameters: the input string (as a pair of pointers marking the start and
	 * end of the string), the value reference to assign, and the base for numerical conversion.
	 * The base parameter defaults to 10 (decimal), but it can be any value between 2 and 36. If the base
	 * given is 0, it will be set to 16 if the string starts with "0x" or "0X", otherwise 8 if the string
	 * still starts with "0", otherwise 10. The std::from_chars_result return value is used to report whether
	 * or not conversion was successful.
	 */
	 [[nodiscard]] inline constexpr std::from_chars_result string_to_uint64(char const* str, char const* const end, uint64_t& value, int base = 10) {
		// If base is zero, base is determined by the string prefix.
		if (base == 0) {
			if (str && *str == '0') {
				if (str + 1 != end && (str[1] == 'x' || str[1] == 'X')) {
					base = 16; // Hexadecimal.
					str += 2; // Skip '0x' or '0X'
				} else {
					base = 8; // Octal.
				}
			} else {
				base = 10; // Decimal.
			}
		} else if (base == 16) {
			// If base is 16 and string starts with '0x' or '0X', skip these characters.
			if (str && *str == '0' && str + 1 != end && (str[1] == 'x' || str[1] == 'X')) {
				str += 2;
			}
		}

		return from_chars(str, end, value, base);
	}

	[[nodiscard]] inline constexpr std::from_chars_result string_to_uint64(char const* const str, size_t length, uint64_t& value, int base = 10) {
		return string_to_uint64(str, str + length, value, base);
	}

	[[nodiscard]] inline constexpr std::from_chars_result string_to_uint64(std::string_view str, uint64_t& value, int base = 10) {
		return string_to_uint64(str.data(), str.length(), value, base);
	}

	/* The constexpr function 'string_to_int64' will convert a string into a int64_t integer value.
	 * The function takes four parameters: the input string (as a pair of pointers marking the start and
	 * end of the string), the value reference to assign, and the base for numerical conversion.
	 * The base parameter defaults to 10 (decimal), but it can be any value between 2 and 36. If the base
	 * given is 0, it will be set to 16 if the string starts with "0x" or "0X", otherwise 8 if the string
	 * still starts with "0", otherwise 10. The std::from_chars_result return value is used to report whether
	 * or not conversion was successful.
	 */
	[[nodiscard]] inline constexpr std::from_chars_result string_to_int64(char const* str, char const* const end, int64_t& value, int base = 10) {
		// If base is zero, base is determined by the string prefix.
		if (base == 0) {
			if (str && *str == '0') {
				if (str + 1 != end && (str[1] == 'x' || str[1] == 'X')) {
					base = 16; // Hexadecimal.
					str += 2; // Skip '0x' or '0X'
				} else {
					base = 8; // Octal.
				}
			} else {
				base = 10; // Decimal.
			}
		} else if (base == 16) {
			// If base is 16 and string starts with '0x' or '0X', skip these characters.
			if (str && *str == '0' && str + 1 != end && (str[1] == 'x' || str[1] == 'X')) {
				str += 2;
			}
		}

		return from_chars(str, end, value, base);
	}

	[[nodiscard]] inline constexpr std::from_chars_result string_to_int64(char const* str, size_t length, int64_t& value, int base = 10) {
		return string_to_int64(str, str + length, value, base);
	}

	[[nodiscard]] inline constexpr std::from_chars_result string_to_int64(std::string_view str, int64_t& value, int base = 10) {
		return string_to_int64(str.data(), str.length(), value, base);
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

	inline constexpr std::string string_tolower(std::string_view str) {
		std::string result { str };
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) -> unsigned char { return std::tolower(c); }
		);
		return result;
	}

	inline constexpr std::string string_toupper(std::string_view str) {
		std::string result { str };
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) -> unsigned char { return std::toupper(c); }
		);
		return result;
	}

	inline constexpr std::string_view bool_to_yes_no(bool b) {
		return b ? "yes" : "no";
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

	template<typename T, typename... Args>
	static std::string string_join(tsl::ordered_map<std::string, T, Args...> const& map, std::string_view delimiter = ", ") {
		if (map.empty()) {
			return "";
		}

		static auto transformer = [](std::pair<std::string_view, T> const& pair) -> std::string_view {
			return pair.first;
		};
		return map | ranges::views::transform(transformer) | ranges::views::join(delimiter) | ranges::to<std::string>();
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
