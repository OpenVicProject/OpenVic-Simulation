#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

	template<typename T>
	struct vec2_t {
		/* PROPERTY generated getter functions will return 2D vectors by value, rather than const reference. */
		using ov_return_by_value = void;

		using type = T;

		T x, y;

		constexpr vec2_t() = default;
		explicit constexpr vec2_t(T new_val) : x { new_val }, y { new_val } {}
		constexpr vec2_t(T new_x, T new_y) : x { new_x }, y { new_y } {}

		constexpr vec2_t(vec2_t const&) = default;
		constexpr vec2_t(vec2_t&&) = default;
		constexpr vec2_t& operator=(vec2_t const&) = default;
		constexpr vec2_t& operator=(vec2_t&&) = default;

		template<typename S>
		constexpr explicit operator vec2_t<S>() const {
			return { static_cast<S>(x), static_cast<S>(y) };
		}

		constexpr vec2_t abs() const {
			return { x >= 0 ? x : -x, y >= 0 ? y : -y };
		}
		constexpr T length_squared() const {
			return x * x + y * y;
		}

		constexpr bool nonnegative() const {
			return x >= 0 && y >= 0;
		}
		/* Checks that both coordinates are less than their corresponding values in the argument vector.
		 * This is intended for checking coordinates lie within certain bounds, it is not suitable for sorting vectors. */
		constexpr bool less_than(vec2_t const& vec) const {
			return x < vec.x && y < vec.y;
		}

		constexpr T* data() {
			return reinterpret_cast<T*>(this);
		}
		constexpr T const* data() const {
			return reinterpret_cast<T const*>(this);
		}

		constexpr T& operator[](size_t index) {
			return data()[index & 1];
		}
		constexpr T const& operator[](size_t index) const {
			return data()[index & 1];
		}

		constexpr friend vec2_t operator+(vec2_t const& left, vec2_t const& right) {
			return { left.x + right.x, left.y + right.y };
		}
		constexpr vec2_t& operator+=(vec2_t const& right) {
			x += right.x;
			y += right.y;
			return *this;
		}
		constexpr friend vec2_t operator+(vec2_t const& left, T const& right) {
			return { left.x + right, left.y + right };
		}
		constexpr friend vec2_t operator+(T const& left, vec2_t const& right) {
			return { left + right.x, left + right.y };
		}
		constexpr vec2_t& operator+=(T const& right) {
			x += right;
			y += right;
			return *this;
		}

		constexpr friend vec2_t operator-(vec2_t const& arg) {
			return { -arg.x, -arg.y };
		}

		constexpr friend vec2_t operator-(vec2_t const& left, vec2_t const& right) {
			return { left.x - right.x, left.y - right.y };
		}
		constexpr vec2_t& operator-=(vec2_t const& right) {
			x -= right.x;
			y -= right.y;
			return *this;
		}
		constexpr friend vec2_t operator-(vec2_t const& left, T const& right) {
			return { left.x - right, left.y - right };
		}
		constexpr friend vec2_t operator-(T const& left, vec2_t const& right) {
			return { left - right.x, left - right.y };
		}
		constexpr vec2_t& operator-=(T const& right) {
			x -= right;
			y -= right;
			return *this;
		}

		constexpr friend vec2_t operator*(vec2_t const& left, vec2_t const& right) {
			return { left.x * right.x, left.y * right.y };
		}
		constexpr vec2_t& operator*=(vec2_t const& right) {
			x *= right.x;
			y *= right.y;
			return *this;
		}
		constexpr friend vec2_t operator*(vec2_t const& left, T const& right) {
			return { left.x * right, left.y * right };
		}
		constexpr friend vec2_t operator*(T const& left, vec2_t const& right) {
			return { left * right.x, left * right.y };
		}
		constexpr vec2_t& operator*=(T const& right) {
			x *= right;
			y *= right;
			return *this;
		}

		constexpr friend vec2_t operator/(vec2_t const& left, vec2_t const& right) {
			return { left.x / right.x, left.y / right.y };
		}
		constexpr vec2_t& operator/=(vec2_t const& right) {
			x /= right.x;
			y /= right.y;
			return *this;
		}
		constexpr friend vec2_t operator/(vec2_t const& left, T const& right) {
			return { left.x / right, left.y / right };
		}
		constexpr friend vec2_t operator/(T const& left, vec2_t const& right) {
			return { left / right.x, left / right.y };
		}
		constexpr vec2_t& operator/=(T const& right) {
			x /= right;
			y /= right;
			return *this;
		}

		constexpr friend bool operator==(vec2_t const& left, vec2_t const& right) {
			return left.x == right.x && left.y == right.y;
		}
		constexpr friend bool operator!=(vec2_t const& left, vec2_t const& right) {
			return left.x != right.x || left.y != right.y;
		}

		constexpr friend std::ostream& operator<<(std::ostream& stream, vec2_t const& value) {
			return stream << "(" << value.x << ", " << value.y << ")";
		}
	};

	using ivec2_t = vec2_t<int32_t>;
	using fvec2_t = vec2_t<fixed_point_t>;

	static_assert(sizeof(ivec2_t) == 2 * sizeof(ivec2_t::type), "ivec2_t size does not equal the sum of its parts' sizes");
	static_assert(sizeof(fvec2_t) == 2 * sizeof(fvec2_t::type), "fvec2_t size does not equal the sum of its parts' sizes");
}
