#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

	template<typename T>
	struct vec2_t {
		T x, y;

		constexpr vec2_t();
		constexpr vec2_t(T new_val);
		constexpr vec2_t(T new_x, T new_y);

		constexpr vec2_t abs() const;
		constexpr T length_squared() const;

		constexpr T* data();
		constexpr T const* data() const;

		constexpr T& operator[](size_t index);
		constexpr T const& operator[](size_t index) const;

		constexpr friend vec2_t operator+(vec2_t const& left, vec2_t const& right);
		constexpr vec2_t& operator+=(vec2_t const& right);

		constexpr friend vec2_t operator-(vec2_t const& arg);
		constexpr friend vec2_t operator-(vec2_t const& left, vec2_t const& right);
		constexpr vec2_t& operator-=(vec2_t const& right);

		constexpr friend std::ostream& operator<<(std::ostream& stream, vec2_t const& value);
	};

	using ivec2_t = vec2_t<int64_t>;
	using fvec2_t = vec2_t<fixed_point_t>;
}
