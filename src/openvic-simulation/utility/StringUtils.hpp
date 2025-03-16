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

#include <tsl/ordered_map.h>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>

namespace OpenVic::StringUtils {
	template<typename T>
	[[nodiscard]] inline constexpr std::from_chars_result from_chars( //
		char const* const first, char const* const last, T& raw_value, const int base = 10
	) noexcept {
		if (!std::is_constant_evaluated()) {
			return std::from_chars(first, last, raw_value, base);
		}

		constexpr auto digit_from_char = [](const char c) constexpr noexcept {
			// convert ['0', '9'] ['A', 'Z'] ['a', 'z'] to [0, 35], everything else to 255
			constexpr unsigned char digit_from_byte[] = {
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

	template<typename T>
	[[nodiscard]] inline constexpr std::to_chars_result to_chars( //
		char* first, char* const last, const T raw_value, const int base = 10
	) noexcept {
		if (!std::is_constant_evaluated()) {
			return std::to_chars(first, last, raw_value, base);
		}

		constexpr char digits[] = //
			{ //
			  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
			  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
			};
		static_assert(std::size(digits) == 36);

		using _unsigned = std::make_unsigned_t<T>;

		_unsigned value = static_cast<_unsigned>(raw_value);

		if constexpr (std::is_signed_v<T>) {
			if (raw_value < 0) {
				if (first == last) {
					return { last, std::errc::value_too_large };
				}

				*first++ = '-';

				value = static_cast<_unsigned>(0 - value);
			}
		}

		constexpr std::size_t buffer_size = sizeof(_unsigned) * CHAR_BIT; // enough for base 2
		char buffer[buffer_size];
		char* const buffer_end = buffer + buffer_size;
		char* r_next = buffer_end;

		switch (base) {
		case 10: { // Derived from _UIntegral_to_buff()
			// Performance note: Ryu's digit table should be faster here.
			constexpr bool use_chunks = sizeof(_unsigned) > sizeof(size_t);

			if constexpr (use_chunks) { // For 64-bit numbers on 32-bit platforms, work in chunks to avoid 64-bit
										 // divisions.
				while (value > 0xFFFF'FFFFU) {
					// Performance note: Ryu's division workaround would be faster here.
					unsigned long chunk = static_cast<unsigned long>(value % 1'000'000'000);
					value = static_cast<_unsigned>(value / 1'000'000'000);

					for (int _Idx = 0; _Idx != 9; ++_Idx) {
						*--r_next = static_cast<char>('0' + chunk % 10);
						chunk /= 10;
					}
				}
			}

			using Truncated = std::conditional_t<use_chunks, unsigned long, _unsigned>;

			Truncated trunc = static_cast<Truncated>(value);

			do {
				*--r_next = static_cast<char>('0' + trunc % 10);
				trunc /= 10;
			} while (trunc != 0);
			break;
		}

		case 2:
			do {
				*--r_next = static_cast<char>('0' + (value & 0b1));
				value >>= 1;
			} while (value != 0);
			break;

		case 4:
			do {
				*--r_next = static_cast<char>('0' + (value & 0b11));
				value >>= 2;
			} while (value != 0);
			break;

		case 8:
			do {
				*--r_next = static_cast<char>('0' + (value & 0b111));
				value >>= 3;
			} while (value != 0);
			break;

		case 16:
			do {
				*--r_next = digits[value & 0b1111];
				value >>= 4;
			} while (value != 0);
			break;

		case 32:
			do {
				*--r_next = digits[value & 0b11111];
				value >>= 5;
			} while (value != 0);
			break;

		default:
			do {
				*--r_next = digits[value % base];
				value = static_cast<_unsigned>(value / base);
			} while (value != 0);
			break;
		}

		const std::ptrdiff_t digits_written = buffer_end - r_next;

		if (last - first < digits_written) {
			return { last, std::errc::value_too_large };
		}

		constexpr auto trivial_copy = [](char* dest, char* const src, size_t count) constexpr {
#if defined(__clang__)
			static_cast<void>(__builtin_memcpy(dest, src, count));
#else
			if (dest != nullptr && src != nullptr && count != 0) {
				count /= sizeof(char);
				for (std::size_t i = 0; i < count; ++i) {
					dest[i] = src[i];
				}
			}
#endif
		};
		//[neargye] constexpr copy chars. P1944 fix this?
		trivial_copy(first, r_next, static_cast<size_t>(digits_written));
		

		return { first + digits_written, std::errc {} };
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
