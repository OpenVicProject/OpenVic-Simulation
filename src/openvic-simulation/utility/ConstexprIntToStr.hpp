#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace OpenVic::ConstexprIntToStr {
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
			return append_sequence(integer_to_string_sequence<next_value>(), std::integer_sequence<char, digits()[remainder]> {});
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