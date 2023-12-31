#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "openvic-simulation/utility/Getters.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic {
	template<typename ValueT, typename IntT>
	struct colour_traits {
		using value_type = ValueT;
		using integer_type = IntT;
		static_assert(sizeof(value_type) * 4 <= sizeof(integer_type), "value_type must be 4x smaller then colour_integer_type");

		/* When colour_t is used as an identifier, NULL_COLOUR is disallowed
		 * and should be reserved as an error value.
		 * When colour_t is used in a purely graphical context, NULL_COLOUR
		 * should be allowed.
		 */
		static constexpr integer_type null = 0;
		static constexpr integer_type component = std::numeric_limits<ValueT>::max();
		static constexpr integer_type component_bit_size = sizeof(ValueT) * CHAR_BIT;
		static constexpr integer_type blue_shift = 0;
		static constexpr integer_type green_shift = component_bit_size;
		static constexpr integer_type red_shift = component_bit_size * 2;
		static constexpr integer_type alpha_shift = component_bit_size * 3;
		static constexpr bool has_alpha = true;

		static constexpr integer_type make_rgb_integer(value_type red, value_type green, value_type blue) {
			return (red << red_shift) | (green << green_shift) | (blue << blue_shift);
		}
		static constexpr integer_type make_argb_integer(value_type red, value_type green, value_type blue, value_type alpha) {
			return make_rgb_integer(red, green, blue) | (alpha << alpha_shift);
		}

		static constexpr value_type red_from_integer(integer_type colour) {
			return (colour >> red_shift) & component;
		}
		static constexpr value_type blue_from_integer(integer_type colour) {
			return (colour >> blue_shift) & component;
		}
		static constexpr value_type green_from_integer(integer_type colour) {
			return (colour >> green_shift) & component;
		}
		static constexpr value_type alpha_from_integer(integer_type colour) {
			return (colour >> alpha_shift) & component;
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
		static constexpr integer_type max_argb = make_argb_integer(component, component, component, component);
	};

	/* Colour represented by an unsigned integer, either 24-bit RGB or 32-bit ARGB. */
	template<typename ValueT, typename ColourIntT, typename ColourTraits = colour_traits<ValueT, ColourIntT>>
	struct basic_colour_t : ReturnByValueProperty {
		using colour_traits = ColourTraits;
		using value_type = typename colour_traits::value_type;
		using integer_type = typename colour_traits::integer_type;

		static_assert(std::same_as<ValueT, value_type> && std::same_as<ColourIntT, integer_type>);

		static constexpr auto max_value = colour_traits::component;

		struct empty_value {
			constexpr empty_value() {}
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
		[[no_unique_address]] _alpha_t alpha;

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
				return { colour_traits::red_from_integer(integer), colour_traits::green_from_integer(integer),
						 colour_traits::blue_from_integer(integer), colour_traits::alpha_from_integer(integer) };
			} else {
				assert(
					colour_traits::alpha_from_integer(integer) == colour_traits::null ||
					colour_traits::alpha_from_integer(integer) == max_value
				);
				return { colour_traits::red_from_integer(integer), colour_traits::green_from_integer(integer),
						 colour_traits::blue_from_integer(integer) };
			}
		}

		static constexpr basic_colour_t
		from_floats(float r, float g, float b, float a = colour_traits::alpha_to_float(max_value))
		requires(colour_traits::has_alpha)
		{
			return { colour_traits::red_from_float(r), colour_traits::green_from_float(g), colour_traits::blue_from_float(b),
					 colour_traits::alpha_from_float(a) };
		}

		static constexpr basic_colour_t from_floats(float r, float g, float b)
		requires(!colour_traits::has_alpha)
		{
			return { colour_traits::red_from_float(r), colour_traits::green_from_float(g), colour_traits::blue_from_float(b) };
		}

		constexpr integer_type as_rgb() const {
			return colour_traits::make_rgb_integer(red, green, blue);
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
		requires(_ColourTraits::has_alpha && std::same_as<typename _ColourTraits::value_type, value_type> && std::same_as<typename _ColourTraits::integer_type, integer_type>)
		explicit constexpr basic_colour_t(basic_colour_t<value_type, integer_type, _ColourTraits> const& colour)
		requires(colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue, colour.alpha } {}

		template<typename _ColourTraits>
		requires(!_ColourTraits::has_alpha && std::same_as<typename _ColourTraits::value_type, value_type> && std::same_as<typename _ColourTraits::integer_type, integer_type>)
		explicit constexpr basic_colour_t(
			basic_colour_t<value_type, integer_type, _ColourTraits> const& colour, value_type a = max_value
		)
		requires(colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue, a } {}

		template<typename _ColourTraits>
		requires(std::same_as<typename _ColourTraits::value_type, value_type> && std::same_as<typename _ColourTraits::integer_type, integer_type>)
		explicit constexpr basic_colour_t(basic_colour_t<value_type, integer_type, _ColourTraits> const& colour)
		requires(!colour_traits::has_alpha)
			: basic_colour_t { colour.red, colour.green, colour.blue } {}

		constexpr explicit operator integer_type() const {
			if constexpr (colour_traits::has_alpha) {
				return as_argb();
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

		inline std::string to_hex_string(bool alpha = false) const {
			using namespace std::string_view_literals;
			static constexpr std::string_view digits = "0123456789ABCDEF"sv;
			static constexpr std::size_t bits_per_digit = 4;
			static constexpr std::size_t digit_mask = (1 << bits_per_digit) - 1;
			static constexpr std::size_t argb_length = colour_traits::component_bit_size / bits_per_digit * 4;
			static constexpr std::size_t rgb_length = colour_traits::component_bit_size / bits_per_digit * 3;

			const std::size_t length = alpha ? argb_length : rgb_length;
			std::array<char, argb_length> address;
			const integer_type value = alpha ? as_argb() : as_rgb();

			for (std::size_t index = 0, j = (length - 1) * bits_per_digit; index < length; ++index, j -= bits_per_digit) {
				address[index] = digits[(value >> j) & digit_mask];
			}
			return { address.data(), length };
		}

		explicit operator std::string() const {
			return to_hex_string(colour_traits::has_alpha);
		}

		friend std::ostream& operator<<(std::ostream& stream, basic_colour_t const& colour) {
			return stream << colour.operator std::string();
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

		constexpr const value_type& operator[](std::size_t index) const {
			return _array_access_helper<const value_type>(*this, index);
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
	struct rgb_colour_traits : colour_traits<ValueT, IntT> {
		static constexpr bool has_alpha = false;
	};

	using colour_argb_t = basic_colour_t<std::uint8_t, std::uint32_t>;
	using colour_rgb_t = basic_colour_t<std::uint8_t, std::uint32_t, rgb_colour_traits<std::uint8_t, std::uint32_t>>;

	using colour_t = colour_rgb_t;

	namespace colour_literals {
		constexpr colour_t operator""_colour(unsigned long long value) {
			return colour_t::from_integer(value);
		}

		constexpr colour_argb_t operator""_argb(unsigned long long value) {
			return colour_argb_t::from_integer(value);
		}
	}
}

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
