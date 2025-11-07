#pragma once

#include <algorithm>
#include <cctype>
#include <string_view>

#include <tsl/ordered_map.h>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>

#include "openvic-simulation/core/memory/String.hpp"

namespace OpenVic {
	inline constexpr bool strings_equal_case_insensitive(std::string_view const& lhs, std::string_view const& rhs) {
		if (lhs.size() != rhs.size()) {
			return false;
		}
		constexpr auto ichar_equals = [](unsigned char l, unsigned char r) {
			return std::tolower(l) == std::tolower(r);
		};
		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), ichar_equals);
	}

	inline memory::string string_tolower(std::string_view str) {
		memory::string result { str };
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) -> unsigned char {
			return std::tolower(c);
		});
		return result;
	}

	inline memory::string string_toupper(std::string_view str) {
		memory::string result { str };
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) -> unsigned char {
			return std::toupper(c);
		});
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

	inline memory::string make_forward_slash_path(std::string_view path) {
		memory::string ret { path };
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
	inline memory::string _append_string_views(Args... args) {
		memory::string ret;
		ret.reserve((args.size() + ...));
		(ret.append(args), ...);
		return ret;
	}

	template<typename... Args>
	requires(std::is_convertible_v<Args, std::string_view> && ...)
	inline memory::string append_string_views(Args... args) {
		return _append_string_views(std::string_view { args }...);
	}

	template<typename T, typename... Args>
	static memory::string
	string_join(tsl::ordered_map<memory::string, T, Args...> const& map, std::string_view delimiter = ", ") {
		if (map.empty()) {
			return "";
		}

		static auto transformer = [](std::pair<std::string_view, T> const& pair) -> std::string_view {
			return pair.first;
		};
		return map | ranges::views::transform(transformer) | ranges::views::join(delimiter) | ranges::to<memory::string>();
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

	constexpr bool valid_basic_identifier_char(char c) {
		return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '_';
	}
	constexpr bool valid_basic_identifier(std::string_view identifier) {
		return std::all_of(identifier.begin(), identifier.end(), valid_basic_identifier_char);
	}
	constexpr std::string_view extract_basic_identifier_prefix(std::string_view identifier) {
		size_t len = 0;
		while (len < identifier.size() && valid_basic_identifier_char(identifier[len])) {
			++len;
		}
		return { identifier.data(), len };
	}

	template<class T, T... values1, T... values2>
	constexpr decltype(auto) append_sequence(std::integer_sequence<T, values1...>, std::integer_sequence<T, values2...>) {
		return std::integer_sequence<T, values1..., values2...> {};
	}

	template<class sequence_t>
	struct string_sequence_to_view;

	template<char... chars>
	struct string_sequence_to_view<std::integer_sequence<char, chars...>> {
		static constexpr decltype(auto) get() {
			return std::string_view { c_str };
		}

		static constexpr const char c_str[] { chars..., char {} };
	};

	template<std::size_t value>
	constexpr decltype(auto) integer_to_string_sequence() {
		constexpr auto digits = []() {
			return "0123456789abcdefghijklmnopqrstuvwxyz";
		};

		constexpr std::size_t remainder = value % 10;
		constexpr std::size_t next_value = value / 10;

		if constexpr (next_value != 0) {
			return append_sequence(
				integer_to_string_sequence<next_value>(), std::integer_sequence<char, digits()[remainder]> {}
			);
		} else {
			return std::integer_sequence<char, digits()[remainder]> {};
		}
	}
	template<std::size_t i>
	constexpr std::string_view make_string() {
		return string_sequence_to_view<decltype(integer_to_string_sequence<i>())> {}.c_str;
	}

	template<std::size_t... ManyIntegers>
	constexpr auto generate_itosv_array(std::integer_sequence<std::size_t, ManyIntegers...>) {
		return std::array<std::string_view, sizeof...(ManyIntegers)> { make_string<ManyIntegers>()... };
	}

	// Make array of N string views, countings up from 0 to N - 1
	template<std::size_t N>
	constexpr auto make_itosv_array() {
		return generate_itosv_array(std::make_integer_sequence<std::size_t, N>());
	}
}
