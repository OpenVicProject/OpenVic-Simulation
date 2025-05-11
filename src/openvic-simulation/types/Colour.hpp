#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cctype>
#include <charconv>
#include <climits>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

#include <fmt/core.h>

#include <range/v3/algorithm/rotate.hpp>

#include "openvic-simulation/utility/Utility.hpp"
#include "openvic-simulation/utility/StringUtils.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	template<typename ValueT, typename IntT, bool HasAlpha = true>
	struct colour_traits {
		using value_type = ValueT;
		using integer_type = IntT;
		static_assert(sizeof(value_type) * 4 <= sizeof(integer_type), "value_type must be 4x smaller then colour_integer_type");

		/* When colour_t is used as an identifier, NULL_COLOUR is disallowed
		 * and should be reserved as an error value.
		 * When colour_t is used in a purely graphical context, NULL_COLOUR
		 * should be allowed.
		 */
		static constexpr bool has_alpha = HasAlpha;
		static constexpr integer_type null = 0;
		static constexpr integer_type component = std::numeric_limits<ValueT>::max();
		static constexpr integer_type component_bit_size = sizeof(ValueT) * CHAR_BIT;

		static constexpr integer_type blue_shift = component_bit_size * 0;
		static constexpr integer_type green_shift = component_bit_size * 1;
		static constexpr integer_type red_shift = component_bit_size * 2;

		static constexpr integer_type rgba_alpha_shift = component_bit_size * 0;
		static constexpr integer_type argb_alpha_shift = component_bit_size * 3;

		static constexpr integer_type make_rgb_integer(value_type red, value_type green, value_type blue) {
			return (red << red_shift) | (green << green_shift) | (blue << blue_shift);
		}
		static constexpr integer_type make_rgba_integer(value_type red, value_type green, value_type blue, value_type alpha) {
			return (make_rgb_integer(red, green, blue) << component_bit_size) | (alpha << rgba_alpha_shift);
		}
		static constexpr integer_type make_argb_integer(value_type red, value_type green, value_type blue, value_type alpha) {
			return make_rgb_integer(red, green, blue) | static_cast<integer_type>(alpha << argb_alpha_shift);
		}

		static constexpr value_type red_from_rgb(integer_type colour) {
			return (colour >> (red_shift + component_bit_size)) & component;
		}
		static constexpr value_type green_from_rgb(integer_type colour) {
			return (colour >> (green_shift + component_bit_size)) & component;
		}
		static constexpr value_type blue_from_rgb(integer_type colour) {
			return (colour >> (blue_shift + component_bit_size)) & component;
		}

		static constexpr value_type alpha_from_rgba(integer_type colour) {
			return (colour >> rgba_alpha_shift) & component;
		}

		static constexpr value_type red_from_argb(integer_type colour) {
			return (colour >> red_shift) & component;
		}
		static constexpr value_type green_from_argb(integer_type colour) {
			return (colour >> green_shift) & component;
		}
		static constexpr value_type blue_from_argb(integer_type colour) {
			return (colour >> blue_shift) & component;
		}

		static constexpr value_type alpha_from_argb(integer_type colour) {
			return (colour >> argb_alpha_shift) & component;
		}

		static constexpr float component_to_float(value_type value) {
			return static_cast<float>(value) / static_cast<float>(component);
		}

		static constexpr value_type component_from_float(float f, float min = 0.0f, float max = 1.0f) {
			constexpr auto floor = [](float f) {
				const std::int64_t i = static_cast<std::int64_t>(f);
				return f < i ? i - 1 : i;
			};

			return floor(std::clamp(min + f * (max - min), min, max) * static_cast<float>(component));
		}

		static constexpr value_type component_from_fraction(int n, int d, float min = 0.0f, float max = 1.0f) {
			return component_from_float(static_cast<float>(n) / static_cast<float>(d), min, max);
		}

		static constexpr float red_to_float(value_type red) {
			return component_to_float(red);
		}
		static constexpr float green_to_float(value_type green) {
			return component_to_float(green);
		}
		static constexpr float blue_to_float(value_type blue) {
			return component_to_float(blue);
		}
		static constexpr float alpha_to_float(value_type alpha) {
			return component_to_float(alpha);
		}

		static constexpr value_type red_from_float(float red) {
			return component_from_float(red);
		}
		static constexpr value_type green_from_float(float green) {
			return component_from_float(green);
		}
		static constexpr value_type blue_from_float(float blue) {
			return component_from_float(blue);
		}
		static constexpr value_type alpha_from_float(float alpha) {
			return component_from_float(alpha);
		}

		static constexpr integer_type max_rgb = make_rgb_integer(component, component, component);
		static constexpr integer_type max_rgba = make_rgba_integer(component, component, component, component);
	};

	/* Colour represented by an unsigned integer, either 24-bit RGB or 32-bit ARGB. */
	template<typename ValueT, typename ColourIntT, typename ColourTraits = colour_traits<ValueT, ColourIntT>>
	struct basic_colour_t {
		/* PROPERTY generated getter functions will return colours by value, rather than const reference. */
		using ov_return_by_value = void;

		using colour_traits = ColourTraits;
		using value_type = typename colour_traits::value_type;
		using integer_type = typename colour_traits::integer_type;

		static_assert(std::same_as<ValueT, value_type> && std::same_as<ColourIntT, integer_type>);

		static constexpr auto max_value = colour_traits::component;

		struct empty_value {
			constexpr empty_value() = default;
			constexpr empty_value(value_type) {}
			constexpr operator value_type() const {
				return max_value;
			}
		};

	private:
		using _alpha_t = std::conditional_t<colour_traits::has_alpha, value_type, empty_value>;

	public:
		value_type red;
		value_type green;
		value_type blue;
		OV_NO_UNIQUE_ADDRESS _alpha_t alpha;

		static constexpr std::integral_constant<std::size_t, std::is_same_v<decltype(alpha), value_type> ? 4 : 3> size = {};

		static constexpr basic_colour_t fill_as(value_type value) {
			if constexpr (colour_traits::has_alpha) {
				return { value, value, value, value };
			} else {
				return { value, value, value };
			}
		}

		static constexpr basic_colour_t null() {
			return fill_as(colour_traits::null);
		}

		static constexpr basic_colour_t from_integer(integer_type integer) {
			if constexpr (colour_traits::has_alpha) {
				return from_rgba(integer);
			} else {
				return from_rgb(integer);
			}
		}

		static constexpr basic_colour_t from_argb(integer_type integer)
		{
			if constexpr (colour_traits::has_alpha) {
				return {
					colour_traits::red_from_argb(integer), colour_traits::green_from_argb(integer),
					colour_traits::blue_from_argb(integer), colour_traits::alpha_from_argb(integer)
				};
			} else {
				return from_rgb(integer);
			}
		}

		static constexpr basic_colour_t from_rgba(integer_type integer)
		{
			if constexpr (colour_traits::has_alpha) {
				return {
					colour_traits::red_from_rgb(integer), colour_traits::green_from_rgb(integer),
					colour_traits::blue_from_rgb(integer), colour_traits::alpha_from_rgba(integer)
				};
			} else {
				return {
					colour_traits::red_from_rgb(integer), colour_traits::green_from_rgb(integer),
					colour_traits::blue_from_rgb(integer)
				};
			}
		}

		static constexpr basic_colour_t from_rgb(integer_type integer)
		{
			return {
				colour_traits::red_from_argb(integer), colour_traits::green_from_argb(integer),
				colour_traits::blue_from_argb(integer)
			};
		}

		static constexpr basic_colour_t from_floats( //
			float r, float g, float b, float a = colour_traits::alpha_to_float(max_value)
		)
		requires(colour_traits::has_alpha)
		{
			return {
				colour_traits::red_from_float(r), colour_traits::green_from_float(g), colour_traits::blue_from_float(b),
				colour_traits::alpha_from_float(a)
			};
		}

		static constexpr basic_colour_t from_floats(float r, float g, float b)
		requires(!colour_traits::has_alpha)
		{
			return { colour_traits::red_from_float(r), colour_traits::green_from_float(g), colour_traits::blue_from_float(b) };
		}

	private:
		inline static constexpr std::from_chars_result parse_from_chars( //
			const char* first, const char* last, integer_type& value
		) {
			if (first < last && first[0] == '0' && (first + 1 < last) && (first[1] == 'x' || first[1] == 'X')) {
				first += 2;
			}

			std::from_chars_result result = StringUtils::from_chars(first, last, value, 16);
			return result;
		}

	public:
		constexpr std::from_chars_result from_chars(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_integer(value);
			return result;
		}

		static constexpr basic_colour_t from_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars(str.data(), str.data() + str.size());
			}
			return result;
		}

		constexpr std::from_chars_result from_chars_rgba(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_rgba(value);
			return result;
		}

		static constexpr basic_colour_t from_rgba_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars_rgba(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars_rgba(str.data(), str.data() + str.size());
			}
			return result;
		}

		constexpr std::from_chars_result from_chars_argb(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_argb(value);
			return result;
		}

		static constexpr basic_colour_t from_argb_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars_argb(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars_argb(str.data(), str.data() + str.size());
			}
			return result;
		}

		constexpr std::from_chars_result from_chars_rgb(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_rgb(value);
			return result;
		}

		static constexpr basic_colour_t from_rgb_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars_rgb(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars_rgb(str.data(), str.data() + str.size());
			}
			return result;
		}

		constexpr integer_type as_rgb() const {
			return colour_traits::make_rgb_integer(red, green, blue);
		}

		constexpr integer_type as_rgba() const {
			const integer_type ret = colour_traits::make_rgba_integer(red, green, blue, alpha);
			return ret;
		}

		constexpr integer_type as_argb() const {
			return colour_traits::make_argb_integer(red, green, blue, alpha);
		}

		constexpr basic_colour_t() : basic_colour_t { null() } {}

		constexpr basic_colour_t(value_type r, value_type g, value_type b, value_type a = max_value)
		requires(colour_traits::has_alpha)
			: red(r), green(g), blue(b), alpha(a) {}

		constexpr basic_colour_t(value_type r, value_type g, value_type b)
		requires(!colour_traits::has_alpha)
			: red(r), green(g), blue(b) {}

		template<typename _ColourTraits>
		requires(
			_ColourTraits::has_alpha && std::same_as<typename _ColourTraits::value_type, value_type> &&
			std::same_as<typename _ColourTraits::integer_type, integer_type>
		)
		explicit constexpr basic_colour_t(basic_colour_t<value_type, integer_type, _ColourTraits> const& colour)
		requires(colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue, colour.alpha } {}

		template<typename _ColourTraits>
		requires(
			!_ColourTraits::has_alpha && std::same_as<typename _ColourTraits::value_type, value_type> &&
			std::same_as<typename _ColourTraits::integer_type, integer_type>
		)
		explicit constexpr basic_colour_t(
			basic_colour_t<value_type, integer_type, _ColourTraits> const& colour, value_type a = max_value
		)
		requires(colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue, a } {}

		template<typename _ColourTraits>
		requires(
			std::same_as<typename _ColourTraits::value_type, value_type> &&
			std::same_as<typename _ColourTraits::integer_type, integer_type>
		)
		explicit constexpr basic_colour_t(basic_colour_t<value_type, integer_type, _ColourTraits> const& colour)
		requires(!colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue } {}

		constexpr explicit operator integer_type() const {
			if constexpr (colour_traits::has_alpha) {
				return as_rgba();
			} else {
				return as_rgb();
			}
		}

		constexpr basic_colour_t with_red(value_type r) const {
			if constexpr (colour_traits::has_alpha) {
				return { r, green, blue, alpha };
			} else {
				return { r, green, blue };
			}
		}

		constexpr basic_colour_t with_green(value_type g) const {
			if constexpr (colour_traits::has_alpha) {
				return { red, g, blue, alpha };
			} else {
				return { red, g, blue };
			}
		}

		constexpr basic_colour_t with_blue(value_type b) const {
			if constexpr (colour_traits::has_alpha) {
				return { red, green, b, alpha };
			} else {
				return { red, green, b };
			}
		}

		constexpr basic_colour_t with_alpha(value_type a) const
		requires(colour_traits::has_alpha)
		{
			return { red, green, blue, a };
		}

		constexpr float redf() const {
			return colour_traits::red_to_float(red);
		}

		constexpr float greenf() const {
			return colour_traits::green_to_float(green);
		}

		constexpr float bluef() const {
			return colour_traits::blue_to_float(blue);
		}

		constexpr float alphaf() const {
			return colour_traits::alpha_to_float(alpha);
		}

		inline constexpr std::to_chars_result to_hex_chars(char* first, char* last, bool alpha = colour_traits::has_alpha) const {
			constexpr size_t component_str_width = (std::bit_width(max_value) + 3) / 4;

			std::to_chars_result result = StringUtils::to_chars(first, last, alpha ? as_rgba() : as_rgb(), 16);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}

			const size_t required_size = component_str_width * (3 + alpha);
			if (OV_unlikely(last - first < required_size)) {
				result.ec = std::errc::value_too_large;
				result.ptr = last;
				return result;
			}

			std::span span { first, result.ptr };
			for (char& c : span) {
				c = c >= 'a' && c <= 'z' ? (c - 32) : c;
			}

			if (span.size() < required_size) {
				size_t rotate_count = 0;
				for (; result.ptr - first < required_size; result.ptr++, rotate_count++) {
					*result.ptr = '0';
				}
				span = { first, result.ptr };
				ranges::rotate(span, span.end() - rotate_count);
			}

			return result;
		}

		struct stack_string {
			static constexpr size_t bits_per_digit = 4;
			static constexpr size_t array_length = colour_traits::component_bit_size / bits_per_digit * 4;

		private:
			std::array<char, array_length> array {};
			uint8_t string_size = 0;

			constexpr stack_string() = default;

			friend inline constexpr stack_string basic_colour_t::to_hex_array(bool alpha) const;
			friend inline constexpr stack_string basic_colour_t::to_argb_hex_array() const;

		public:
			constexpr const char* data() const {
				return array.data();
			}

			constexpr size_t size() const {
				return string_size;
			}

			constexpr size_t length() const {
				return string_size;
			}

			constexpr decltype(array)::const_iterator begin() const {
				return array.begin();
			}

			constexpr decltype(array)::const_iterator end() const {
				return begin() + size();
			}

			constexpr decltype(array)::const_reference operator[](size_t index) const {
				return array[index];
			}

			constexpr bool empty() const {
				return size() == 0;
			}

			constexpr operator std::string_view() const {
				return std::string_view { data(), data() + size() };
			}

			operator memory::string() const {
				return memory::string { data(), size() };
			}
		};

		inline constexpr stack_string to_hex_array(bool alpha = colour_traits::has_alpha) const {
			stack_string str {};
			std::to_chars_result result = to_hex_chars(str.array.data(), str.array.data() + str.array.size(), alpha);
			str.string_size = result.ptr - str.data();
			return str;
		}

		inline memory::string to_hex_string(bool alpha = colour_traits::has_alpha) const {
			stack_string result = to_hex_array(alpha);
			if (OV_unlikely(result.empty())) {
				return {};
			}

			return result;
		}

		inline constexpr std::to_chars_result to_argb_hex_chars(char* first, char* last) const {
			constexpr size_t component_str_width = (std::bit_width(max_value) + 3) / 4;

			std::to_chars_result result = to_hex_chars(first, last, true);
			if (result.ec != std::errc {}) {
				return result;
			}

			std::span hex_span { first, result.ptr };
			ranges::rotate(hex_span, hex_span.end() - component_str_width);

			return result;
		}

		inline constexpr stack_string to_argb_hex_array() const {
			stack_string str {};
			std::to_chars_result result = to_argb_hex_chars(str.array.data(), str.array.data() + str.array.size());
			str.string_size = result.ptr - str.data();
			return str;
		}

		inline memory::string to_argb_hex_string() const {
			stack_string result = to_argb_hex_array();
			if (OV_unlikely(result.empty())) {
				return {};
			}

			return result;
		}

		explicit operator memory::string() const {
			return to_hex_string();
		}

		friend std::ostream& operator<<(std::ostream& stream, basic_colour_t const& colour) {
			stack_string result = colour.to_hex_array();
			if (OV_unlikely(result.empty())) {
				return stream;
			}

			return stream << static_cast<std::string_view>(result);
		}

		constexpr bool is_null() const {
			return *this == null();
		}

		constexpr auto operator<=>(basic_colour_t const& rhs) const = default;
		constexpr auto operator<=>(integer_type rhs) const {
			return operator integer_type() <=> rhs;
		}

		constexpr value_type& operator[](std::size_t index) {
			return _array_access_helper<value_type>(*this, index);
		}

		constexpr value_type const& operator[](std::size_t index) const {
			return _array_access_helper<const value_type>(*this, index);
		}

		constexpr basic_colour_t invert() const {
			basic_colour_t new_colour = *this;
			new_colour.red = max_value - new_colour.red;
			new_colour.green = max_value - new_colour.green;
			new_colour.blue = max_value - new_colour.blue;
			return new_colour;
		}

		constexpr basic_colour_t full_invert() const requires(colour_traits::has_alpha) {
			basic_colour_t new_colour = *this;
			new_colour.red = max_value - new_colour.red;
			new_colour.green = max_value - new_colour.green;
			new_colour.blue = max_value - new_colour.blue;
			new_colour.alpha = max_value - new_colour.alpha;
			return new_colour;
		}

		constexpr basic_colour_t operator-() const {
			return invert();
		}

		// See https://stackoverflow.com/a/69869976
		// https://github.com/Myndex/max-contrast/
		basic_colour_t contrast() const {
			constexpr double LUMINANCE_FLIP = 0.342;
			constexpr double POWER = 2.4;
			constexpr double RED_MULTIPLIER = 0.2126729;
			constexpr double GREEN_MULTIPLIER = 0.7151522;
			constexpr double BLUE_MULTIPLIER = 0.0721750;

			double luminance = std::pow(red / max_value, POWER) * RED_MULTIPLIER +
				std::pow(green / max_value, POWER) * GREEN_MULTIPLIER + std::pow(blue, POWER) * BLUE_MULTIPLIER;
			return luminance >= LUMINANCE_FLIP ? basic_colour_t::from_floats(0, 0, 0) : basic_colour_t::from_floats(1, 1, 1);
		}

	private:
		template<typename V, typename T>
		static constexpr V& _array_access_helper(T& t, std::size_t index) {
			switch (index) {
			case 0: return t.red;
			case 1: return t.green;
			case 2: return t.blue;
			}
			if constexpr (size() == 4) {
				if (index == 3) {
					return t.alpha;
				}
				assert(index < 4);
			} else {
				assert(index < 3);
			}
			/* Segfault if index is out of bounds and NDEBUG is defined! */
			OpenVic::utility::unreachable();
		}

	public:
		template<std::size_t Index>
		auto&& get() & {
			return get_helper<Index>(*this);
		}

		template<std::size_t Index>
		auto&& get() && {
			return get_helper<Index>(*this);
		}

		template<std::size_t Index>
		auto&& get() const& {
			return get_helper<Index>(*this);
		}

		template<std::size_t Index>
		auto&& get() const&& {
			return get_helper<Index>(*this);
		}

	private:
		template<std::size_t Index, typename T>
		static auto&& get_helper(T&& t) {
			static_assert(Index < size(), "Index out of bounds for OpenVic::basic_colour_t");
			if constexpr (Index == 0) {
				return std::forward<T>(t).red;
			}
			if constexpr (Index == 1) {
				return std::forward<T>(t).green;
			}
			if constexpr (Index == 2) {
				return std::forward<T>(t).blue;
			}
			if constexpr (Index == 3) {
				return std::forward<T>(t).alpha;
			}
		}
	};

	template<typename T>
	concept IsColour = OpenVic::utility::is_specialization_of_v<T, basic_colour_t>;

	template<typename ValueT, typename IntT>
	using rgb_colour_traits = colour_traits<ValueT, IntT, false>;

	using colour_argb_t = basic_colour_t<std::uint8_t, std::uint32_t>;
	using colour_rgb_t = basic_colour_t<std::uint8_t, std::uint32_t, rgb_colour_traits<std::uint8_t, std::uint32_t>>;

	using colour_t = colour_rgb_t;

	extern template struct basic_colour_t<std::uint8_t, std::uint32_t>;
	extern template struct basic_colour_t<std::uint8_t, std::uint32_t, rgb_colour_traits<std::uint8_t, std::uint32_t>>;

	namespace colour_literals {
		constexpr colour_t operator""_rgb(unsigned long long value) {
			return colour_t::from_integer(value);
		}

		constexpr colour_argb_t operator""_rgba(unsigned long long value) {
			return colour_argb_t::from_rgba(value);
		}

		constexpr colour_argb_t operator""_argb(unsigned long long value) {
			return colour_argb_t::from_argb(value);
		}
	}
}

template<typename T>
struct fmt::formatter<T, std::enable_if_t<OpenVic::utility::specialization_of<T, OpenVic::basic_colour_t>, char>>
	: formatter<string_view> {
	auto format(T const& c, format_context& ctx) const {
		typename T::stack_string result = c.to_hex_array();
		if (OV_unlikely(result.empty())) {
			return formatter<string_view>::format(string_view {}, ctx);
		}

		return formatter<string_view>::format(string_view { result.data(), result.size() }, ctx);
	}
};

namespace std {
	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	struct tuple_size<::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>>
		: integral_constant<size_t, ::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>::size()> {};

	template<size_t Index, typename ValueT, typename ColourIntT, typename ColourTraits>
	struct tuple_element<Index, ::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>> {
		using type = decltype(::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>::red);
	};
	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	struct tuple_element<1, ::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>> {
		using type = decltype(::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>::green);
	};
	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	struct tuple_element<2, ::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>> {
		using type = decltype(::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>::blue);
	};
	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	struct tuple_element<3, ::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>> {
		using type = decltype(::OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>::alpha);
	};

	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	struct hash<OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits>> {
		size_t operator()(OpenVic::basic_colour_t<ValueT, ColourIntT, ColourTraits> const& s) const noexcept {
			return hash<decltype(s.as_argb())> {}(s.as_argb());
		}
	};
}
