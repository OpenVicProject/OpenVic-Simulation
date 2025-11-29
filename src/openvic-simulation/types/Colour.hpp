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

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <range/v3/algorithm/rotate.hpp>

#include "openvic-simulation/types/StackString.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/core/string/CharConv.hpp"
#include "openvic-simulation/core/Typedefs.hpp"

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

		OV_SPEED_INLINE static constexpr integer_type make_rgb_integer(value_type red, value_type green, value_type blue) {
			return (red << red_shift) | (green << green_shift) | (blue << blue_shift);
		}
		OV_SPEED_INLINE static constexpr integer_type make_rgba_integer(value_type red, value_type green, value_type blue, value_type alpha) {
			return (make_rgb_integer(red, green, blue) << component_bit_size) | (alpha << rgba_alpha_shift);
		}
		OV_SPEED_INLINE static constexpr integer_type make_argb_integer(value_type red, value_type green, value_type blue, value_type alpha) {
			return make_rgb_integer(red, green, blue) | static_cast<integer_type>(alpha << argb_alpha_shift);
		}

		OV_SPEED_INLINE static constexpr value_type red_from_rgb(integer_type colour) {
			return (colour >> (red_shift + component_bit_size)) & component;
		}
		OV_SPEED_INLINE static constexpr value_type green_from_rgb(integer_type colour) {
			return (colour >> (green_shift + component_bit_size)) & component;
		}
		OV_SPEED_INLINE static constexpr value_type blue_from_rgb(integer_type colour) {
			return (colour >> (blue_shift + component_bit_size)) & component;
		}

		OV_SPEED_INLINE static constexpr value_type alpha_from_rgba(integer_type colour) {
			return (colour >> rgba_alpha_shift) & component;
		}

		OV_SPEED_INLINE static constexpr value_type red_from_argb(integer_type colour) {
			return (colour >> red_shift) & component;
		}
		OV_SPEED_INLINE static constexpr value_type green_from_argb(integer_type colour) {
			return (colour >> green_shift) & component;
		}
		OV_SPEED_INLINE static constexpr value_type blue_from_argb(integer_type colour) {
			return (colour >> blue_shift) & component;
		}

		OV_SPEED_INLINE static constexpr value_type alpha_from_argb(integer_type colour) {
			return (colour >> argb_alpha_shift) & component;
		}

		OV_SPEED_INLINE static constexpr float component_to_float(value_type value) {
			return static_cast<float>(value) / static_cast<float>(component);
		}

		OV_SPEED_INLINE static constexpr value_type component_from_float(float f, float min = 0.0f, float max = 1.0f) {
			constexpr auto floor = [](float f) {
				const std::int64_t i = static_cast<std::int64_t>(f);
				return f < i ? i - 1 : i;
			};

			return floor(std::clamp(min + f * (max - min), min, max) * static_cast<float>(component));
		}

		OV_SPEED_INLINE static constexpr value_type component_from_fraction(int n, int d, float min = 0.0f, float max = 1.0f) {
			return component_from_float(static_cast<float>(n) / static_cast<float>(d), min, max);
		}

		OV_SPEED_INLINE static constexpr float red_to_float(value_type red) {
			return component_to_float(red);
		}
		OV_SPEED_INLINE static constexpr float green_to_float(value_type green) {
			return component_to_float(green);
		}
		OV_SPEED_INLINE static constexpr float blue_to_float(value_type blue) {
			return component_to_float(blue);
		}
		OV_SPEED_INLINE static constexpr float alpha_to_float(value_type alpha) {
			return component_to_float(alpha);
		}

		OV_SPEED_INLINE static constexpr value_type red_from_float(float red) {
			return component_from_float(red);
		}
		OV_SPEED_INLINE static constexpr value_type green_from_float(float green) {
			return component_from_float(green);
		}
		OV_SPEED_INLINE static constexpr value_type blue_from_float(float blue) {
			return component_from_float(blue);
		}
		OV_SPEED_INLINE static constexpr value_type alpha_from_float(float alpha) {
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
			OV_ALWAYS_INLINE constexpr empty_value() {};
			OV_ALWAYS_INLINE constexpr empty_value(value_type) {}
			OV_ALWAYS_INLINE constexpr operator value_type() const {
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

		OV_SPEED_INLINE static constexpr basic_colour_t fill_as(value_type value) {
			if constexpr (colour_traits::has_alpha) {
				return { value, value, value, value };
			} else {
				return { value, value, value };
			}
		}

		OV_SPEED_INLINE static constexpr basic_colour_t null() {
			return fill_as(colour_traits::null);
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_integer(integer_type integer) {
			if constexpr (colour_traits::has_alpha) {
				return from_rgba(integer);
			} else {
				return from_rgb(integer);
			}
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_argb(integer_type integer)
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

		OV_SPEED_INLINE static constexpr basic_colour_t from_rgba(integer_type integer)
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

		OV_SPEED_INLINE static constexpr basic_colour_t from_rgb(integer_type integer)
		{
			return {
				colour_traits::red_from_argb(integer), colour_traits::green_from_argb(integer),
				colour_traits::blue_from_argb(integer)
			};
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_floats( //
			float r, float g, float b, float a = colour_traits::alpha_to_float(max_value)
		)
		requires(colour_traits::has_alpha)
		{
			return {
				colour_traits::red_from_float(r), colour_traits::green_from_float(g), colour_traits::blue_from_float(b),
				colour_traits::alpha_from_float(a)
			};
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_floats(float r, float g, float b)
		requires(!colour_traits::has_alpha)
		{
			return { colour_traits::red_from_float(r), colour_traits::green_from_float(g), colour_traits::blue_from_float(b) };
		}

	private:
		OV_SPEED_INLINE static constexpr std::from_chars_result parse_from_chars( //
			const char* first, const char* last, integer_type& value
		) {
			if (first < last && first[0] == '0' && (first + 1 < last) && (first[1] == 'x' || first[1] == 'X')) {
				first += 2;
			}

			std::from_chars_result result = OpenVic::from_chars(first, last, value, 16);
			return result;
		}

	public:
		OV_SPEED_INLINE constexpr std::from_chars_result from_chars(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_integer(value);
			return result;
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars(str.data(), str.data() + str.size());
			}
			return result;
		}

		OV_SPEED_INLINE constexpr std::from_chars_result from_chars_rgba(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_rgba(value);
			return result;
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_rgba_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars_rgba(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars_rgba(str.data(), str.data() + str.size());
			}
			return result;
		}

		OV_SPEED_INLINE constexpr std::from_chars_result from_chars_argb(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_argb(value);
			return result;
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_argb_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars_argb(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars_argb(str.data(), str.data() + str.size());
			}
			return result;
		}

		OV_SPEED_INLINE constexpr std::from_chars_result from_chars_rgb(const char* first, const char* last) {
			integer_type value;
			std::from_chars_result result = parse_from_chars(first, last, value);
			if (OV_unlikely(result.ec != std::errc {})) {
				return result;
			}
			*this = from_rgb(value);
			return result;
		}

		OV_SPEED_INLINE static constexpr basic_colour_t from_rgb_string(std::string_view str, std::from_chars_result* from_chars = nullptr) {
			basic_colour_t result {};
			if (from_chars == nullptr) {
				result.from_chars_rgb(str.data(), str.data() + str.size());
			} else {
				*from_chars = result.from_chars_rgb(str.data(), str.data() + str.size());
			}
			return result;
		}

		OV_SPEED_INLINE constexpr integer_type as_rgb() const {
			return colour_traits::make_rgb_integer(red, green, blue);
		}

		OV_SPEED_INLINE constexpr integer_type as_rgba() const {
			const integer_type ret = colour_traits::make_rgba_integer(red, green, blue, alpha);
			return ret;
		}

		OV_SPEED_INLINE constexpr integer_type as_argb() const {
			return colour_traits::make_argb_integer(red, green, blue, alpha);
		}

		OV_ALWAYS_INLINE constexpr basic_colour_t() : basic_colour_t { null() } {}

		OV_SPEED_INLINE constexpr basic_colour_t(value_type r, value_type g, value_type b, value_type a = max_value)
		requires(colour_traits::has_alpha)
			: red(r), green(g), blue(b), alpha(a) {}

		OV_SPEED_INLINE constexpr basic_colour_t(value_type r, value_type g, value_type b)
		requires(!colour_traits::has_alpha)
			: red(r), green(g), blue(b) {}

		template<typename _ColourTraits>
		requires(
			_ColourTraits::has_alpha && std::same_as<typename _ColourTraits::value_type, value_type> &&
			std::same_as<typename _ColourTraits::integer_type, integer_type>
		)
		OV_SPEED_INLINE explicit constexpr basic_colour_t(basic_colour_t<value_type, integer_type, _ColourTraits> const& colour)
		requires(colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue, colour.alpha } {}

		template<typename _ColourTraits>
		requires(
			!_ColourTraits::has_alpha && std::same_as<typename _ColourTraits::value_type, value_type> &&
			std::same_as<typename _ColourTraits::integer_type, integer_type>
		)
		OV_SPEED_INLINE explicit constexpr basic_colour_t(
			basic_colour_t<value_type, integer_type, _ColourTraits> const& colour, value_type a = max_value
		)
		requires(colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue, a } {}

		template<typename _ColourTraits>
		requires(
			std::same_as<typename _ColourTraits::value_type, value_type> &&
			std::same_as<typename _ColourTraits::integer_type, integer_type>
		)
		OV_SPEED_INLINE explicit constexpr basic_colour_t(basic_colour_t<value_type, integer_type, _ColourTraits> const& colour)
		requires(!colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue } {}

		OV_SPEED_INLINE constexpr explicit operator integer_type() const {
			if constexpr (colour_traits::has_alpha) {
				return as_rgba();
			} else {
				return as_rgb();
			}
		}

		OV_SPEED_INLINE constexpr basic_colour_t with_red(value_type r) const {
			if constexpr (colour_traits::has_alpha) {
				return { r, green, blue, alpha };
			} else {
				return { r, green, blue };
			}
		}

		OV_SPEED_INLINE constexpr basic_colour_t with_green(value_type g) const {
			if constexpr (colour_traits::has_alpha) {
				return { red, g, blue, alpha };
			} else {
				return { red, g, blue };
			}
		}

		OV_SPEED_INLINE constexpr basic_colour_t with_blue(value_type b) const {
			if constexpr (colour_traits::has_alpha) {
				return { red, green, b, alpha };
			} else {
				return { red, green, b };
			}
		}

		OV_SPEED_INLINE constexpr basic_colour_t with_alpha(value_type a) const
		requires(colour_traits::has_alpha)
		{
			return { red, green, blue, a };
		}

		OV_SPEED_INLINE constexpr float redf() const {
			return colour_traits::red_to_float(red);
		}

		OV_SPEED_INLINE constexpr float greenf() const {
			return colour_traits::green_to_float(green);
		}

		OV_SPEED_INLINE constexpr float bluef() const {
			return colour_traits::blue_to_float(blue);
		}

		OV_SPEED_INLINE constexpr float alphaf() const {
			return colour_traits::alpha_to_float(alpha);
		}

		OV_SPEED_INLINE constexpr std::to_chars_result to_hex_chars(char* first, char* last, bool alpha = colour_traits::has_alpha) const {
			constexpr size_t component_str_width = (std::bit_width(max_value) + 3) / 4;

			std::to_chars_result result = OpenVic::to_chars(first, last, alpha ? as_rgba() : as_rgb(), 16);
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

		struct stack_string final : StackString<colour_traits::component_bit_size / /*bits_per_digit*/ 4 * 4> {
		protected:
			using StackString<colour_traits::component_bit_size / /*bits_per_digit*/ 4 * 4>::StackString;
			friend OV_SPEED_INLINE constexpr stack_string basic_colour_t::to_hex_array(bool alpha) const;
			friend OV_SPEED_INLINE constexpr stack_string basic_colour_t::to_argb_hex_array() const;
		};

		OV_SPEED_INLINE constexpr stack_string to_hex_array(bool alpha = colour_traits::has_alpha) const {
			stack_string str {};
			std::to_chars_result result = to_hex_chars(str._array.data(), str._array.data() + str._array.size(), alpha);
			str._string_size = result.ptr - str.data();
			return str;
		}

		OV_SPEED_INLINE memory::string to_hex_string(bool alpha = colour_traits::has_alpha) const {
			stack_string result = to_hex_array(alpha);
			if (OV_unlikely(result.empty())) {
				return {};
			}

			return result;
		}

		OV_SPEED_INLINE constexpr std::to_chars_result to_argb_hex_chars(char* first, char* last) const {
			constexpr size_t component_str_width = (std::bit_width(max_value) + 3) / 4;

			std::to_chars_result result = to_hex_chars(first, last, true);
			if (result.ec != std::errc {}) {
				return result;
			}

			std::span hex_span { first, result.ptr };
			ranges::rotate(hex_span, hex_span.end() - component_str_width);

			return result;
		}

		OV_SPEED_INLINE constexpr stack_string to_argb_hex_array() const {
			stack_string str {};
			std::to_chars_result result = to_argb_hex_chars(str._array.data(), str._array.data() + str._array.size());
			str._string_size = result.ptr - str.data();
			return str;
		}

		OV_SPEED_INLINE memory::string to_argb_hex_string() const {
			stack_string result = to_argb_hex_array();
			if (OV_unlikely(result.empty())) {
				return {};
			}

			return result;
		}

		OV_SPEED_INLINE explicit operator memory::string() const {
			return to_hex_string();
		}

		friend std::ostream& operator<<(std::ostream& stream, basic_colour_t const& colour) {
			stack_string result = colour.to_hex_array();
			if (OV_unlikely(result.empty())) {
				return stream;
			}

			return stream << static_cast<std::string_view>(result);
		}

		OV_SPEED_INLINE constexpr bool is_null() const {
			return *this == null();
		}

		OV_SPEED_INLINE constexpr auto operator<=>(basic_colour_t const& rhs) const = default;
		OV_SPEED_INLINE constexpr auto operator<=>(integer_type rhs) const {
			return operator integer_type() <=> rhs;
		}

		OV_SPEED_INLINE constexpr value_type& operator[](std::size_t index) {
			return _array_access_helper<value_type>(*this, index);
		}

		OV_SPEED_INLINE constexpr value_type const& operator[](std::size_t index) const {
			return _array_access_helper<const value_type>(*this, index);
		}

		OV_SPEED_INLINE constexpr basic_colour_t invert() const {
			basic_colour_t new_colour = *this;
			new_colour.red = max_value - new_colour.red;
			new_colour.green = max_value - new_colour.green;
			new_colour.blue = max_value - new_colour.blue;
			return new_colour;
		}

		OV_SPEED_INLINE constexpr basic_colour_t full_invert() const requires(colour_traits::has_alpha) {
			basic_colour_t new_colour = *this;
			new_colour.red = max_value - new_colour.red;
			new_colour.green = max_value - new_colour.green;
			new_colour.blue = max_value - new_colour.blue;
			new_colour.alpha = max_value - new_colour.alpha;
			return new_colour;
		}

		OV_SPEED_INLINE constexpr basic_colour_t operator-() const {
			return invert();
		}

		// See https://stackoverflow.com/a/69869976
		// https://github.com/Myndex/max-contrast/
		OV_SPEED_INLINE basic_colour_t contrast() const {
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
		OV_SPEED_INLINE static constexpr V& _array_access_helper(T& t, std::size_t index) {
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
			OpenVic::unreachable();
		}

	public:
		template<std::size_t Index>
		OV_SPEED_INLINE auto&& get() & {
			return get_helper<Index>(*this);
		}

		template<std::size_t Index>
		OV_SPEED_INLINE auto&& get() && {
			return get_helper<Index>(*this);
		}

		template<std::size_t Index>
		OV_SPEED_INLINE auto&& get() const& {
			return get_helper<Index>(*this);
		}

		template<std::size_t Index>
		OV_SPEED_INLINE auto&& get() const&& {
			return get_helper<Index>(*this);
		}

	private:
		template<std::size_t Index, typename T>
		OV_SPEED_INLINE static auto&& get_helper(T&& t) {
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
	concept IsColour = specialization_of<T, basic_colour_t>;

	template<typename T>
	concept has_get_colour = requires(T const& t) {
		{ t.get_colour() } -> IsColour;
	};

	template<typename T>
	concept has_get_identifier_and_colour = has_get_identifier<T> && has_get_colour<T>;

	template<typename ValueT, typename IntT>
	using rgb_colour_traits = colour_traits<ValueT, IntT, false>;

	using colour_argb_t = basic_colour_t<std::uint8_t, std::uint32_t>;
	using colour_rgb_t = basic_colour_t<std::uint8_t, std::uint32_t, rgb_colour_traits<std::uint8_t, std::uint32_t>>;

	using colour_t = colour_rgb_t;

	extern template struct basic_colour_t<std::uint8_t, std::uint32_t>;
	extern template struct basic_colour_t<std::uint8_t, std::uint32_t, rgb_colour_traits<std::uint8_t, std::uint32_t>>;

	namespace colour_literals {
		OV_SPEED_INLINE constexpr colour_t operator""_rgb(unsigned long long value) {
			return colour_t::from_integer(value);
		}

		OV_SPEED_INLINE constexpr colour_argb_t operator""_rgba(unsigned long long value) {
			return colour_argb_t::from_rgba(value);
		}

		OV_SPEED_INLINE constexpr colour_argb_t operator""_argb(unsigned long long value) {
			return colour_argb_t::from_argb(value);
		}
	}
}

template<OpenVic::specialization_of<OpenVic::basic_colour_t> T>
class fmt::detail::is_tuple_like_<T> {
public:
	static constexpr const bool value = false;
};

template<OpenVic::specialization_of<OpenVic::basic_colour_t> T>
struct fmt::formatter<T> {
	constexpr void set_brackets(basic_string_view<char> open, basic_string_view<char> close) {
		_opening_bracket = open;
		_closing_bracket = close;
	}

	constexpr void set_separator(basic_string_view<char> sep) {
		_separator = sep;
	}

	constexpr format_parse_context::iterator parse(format_parse_context& ctx) {
		format_parse_context::iterator it = ctx.begin(), end = ctx.end();
		if (it == end || *it == '}') {
			return it;
		}

		it = detail::parse_align(it, end, _specs);
		if (it == end) {
			return it;
		}

		if (*it == '#') {
			_specs.set_alt();
			++it;
			if (it == end) {
				return it;
			}
		}

		char c = *it;
		if ((c >= '0' && c <= '9') || c == '{') {
			it = detail::parse_width(it, end, _specs, _specs.width_ref, ctx);
			if (it == end) {
				return it;
			}
		}

		switch (*it) {
		case 'X': _specs.set_upper(); [[fallthrough]];
		case 'x':
			_specs.set_type(presentation_type::hex);
			++it;
			break;
		}
		if (it == end) {
			return it;
		}

		switch (*it) {
		case 'n':
			set_brackets({}, {});
			_value_format_type = value_format_type::value;
			++it;
			break;
		}
		if (it == end) {
			return it;
		}

		switch (*it) {
		case 'v':
			_value_format_type = value_format_type::value;
			++it;
			break;
		case 'i':
			_value_format_type = value_format_type::integer;
			++it;
			break;
		case 'f':
			_value_format_type = value_format_type::floating;
			++it;
			break;
		}

		if (it != end) {
			switch (*it) {
			case 't':
				_alpha_handle_type = alpha_handle_type::follow_type;
				++it;
				break;
			case 's':
				_alpha_handle_type = alpha_handle_type::no_alpha;
				++it;
				break;
			case 'a':
				_alpha_handle_type = alpha_handle_type::argb;
				++it;
				break;
			case 'r':
				_alpha_handle_type = alpha_handle_type::rgba;
				++it;
				break;
			}
		}

		if (it != end && (*it == ':' || _value_format_type != value_format_type::none)) {
			if (_specs.align() != align::none) {
				report_error("invalid format specifier");
			}
			
			if (_specs.alt()) {
				report_error("invalid format specifier");
			}

			if (_specs.width != 0 || _specs.dynamic_width() != arg_id_kind::none) {
				report_error("invalid format specifier");
			}

			if (_specs.upper()) {
				report_error("invalid format specifier");
			}

			if (_specs.type() == presentation_type::hex) {
				report_error("invalid format specifier");
			}

			if (*it == ':') {
				++it;
			}
			ctx.advance_to(it);

			switch (_value_format_type) {
			case value_format_type::none:	  _value_format_type = value_format_type::value; [[fallthrough]];
			case value_format_type::value:	  return _colour_value.parse(ctx);
			case value_format_type::integer:  return _integer.parse(ctx);
			case value_format_type::floating: return _floating.parse(ctx);
			}
		}

		if (_value_format_type != value_format_type::none) {
			report_error("invalid format specifier");
		}

		return it;
	}

	format_context::iterator format(T const& colour, format_context& ctx) const {
		format_specs specs { _specs };
		if (_specs.dynamic()) {
			detail::handle_dynamic_spec(_specs.dynamic_width(), specs.width, _specs.width_ref, ctx);
		}

		format_context::iterator out = ctx.out();

		if (_value_format_type == value_format_type::none) {
			typename T::stack_string result = [&]() -> typename T::stack_string {
				switch (_alpha_handle_type) {
				default:
				case alpha_handle_type::follow_type: return colour.to_hex_array();
				case alpha_handle_type::no_alpha:	 return colour.to_hex_array(false);
				case alpha_handle_type::argb:		 return colour.to_argb_hex_array();
				case alpha_handle_type::rgba:		 return colour.to_hex_array(true);
				}
			}();

			if (result.empty()) {
				return out;
			}

			if (_specs.type() == presentation_type::hex) {
				bool upper = _specs.upper();
				if (_specs.alt()) {
					if (upper) {
						out = detail::write(out, "0X");
					} else {
						out = detail::write(out, "0x");
					}
				}
				
				if (!upper) {
					std::array<char, T::stack_string::array_length> lower;
					size_t i = 0;
					for (char& c : lower) {
						c = std::tolower(result[i]);
						if (c == '\0') {
							break;
						}
						++i;
					}
					return detail::write(out, string_view { lower.data(), i }, specs, ctx.locale());
				}
			} else if (_specs.alt()) {
				*out++ = '#'; 
			}

			return detail::write(out, string_view { result.data(), result.size() }, specs, ctx.locale());
		}

		auto write_value = [&](T::value_type const& comp) {
			ctx.advance_to(out);
			switch (_value_format_type) {
			case value_format_type::value:	  _colour_value.format(comp, ctx); break;
			case value_format_type::integer:  _integer.format(comp, ctx); break;
			case value_format_type::floating: _floating.format(double(comp) / T::max_value, ctx); break;
			}
		};

		out = detail::copy<char>(_opening_bracket, out);
		size_t size = colour.size();
		if (size == 4) {
			switch (_alpha_handle_type) {
			case alpha_handle_type::argb:
				write_value(colour[--size]);
				out = detail::copy<char>(_separator, out);
				break;
			case alpha_handle_type::no_alpha: --size; break;
			default:						  break;
			}
		} else if (_alpha_handle_type == alpha_handle_type::argb) {
			write_value(T::max_value);
			out = detail::copy<char>(_separator, out);
		}

		size_t i = 0;
		for (; i < size; ++i) {
			if (i > 0) {
				out = detail::copy<char>(_separator, out);
			}
			write_value(colour[i]);
		}

		if (i == 3 && _alpha_handle_type == alpha_handle_type::rgba) {
			out = detail::copy<char>(_separator, out);
			write_value(T::max_value);
		}

		return detail::copy<char>(_closing_bracket, out);
	}

private:
	fmt::detail::dynamic_format_specs<char> _specs;
	fmt::formatter<typename T::value_type> _colour_value;
	fmt::formatter<unsigned long long> _integer;
	fmt::formatter<double> _floating;

	enum value_format_type { none, value, integer, floating } _value_format_type = value_format_type::none;
	enum alpha_handle_type { follow_type, no_alpha, argb, rgba } _alpha_handle_type = alpha_handle_type::follow_type;

	basic_string_view<char> _separator = detail::string_literal<char, ',', ' '> {};
	basic_string_view<char> _opening_bracket = detail::string_literal<char, '('> {};
	basic_string_view<char> _closing_bracket = detail::string_literal<char, ')'> {};
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
