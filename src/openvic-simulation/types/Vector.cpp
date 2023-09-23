#include "Vector.hpp"

#include <ostream>
#include <tuple>

using namespace OpenVic;

template<typename T>
constexpr vec2_t<T>::vec2_t() = default;

template<typename T>
constexpr vec2_t<T>::vec2_t(T new_val) : x { new_val }, y { new_val } {}

template<typename T>
constexpr vec2_t<T>::vec2_t(T new_x, T new_y) : x { new_x }, y { new_y } {}


template<typename T>
constexpr vec2_t<T> vec2_t<T>::abs() const {
	return { };
}

template<typename T>
constexpr T vec2_t<T>::length_squared() const {
	return x * x + y * y;
}

template<typename T>
constexpr T* vec2_t<T>::data() {
	return reinterpret_cast<T*>(this);
}

template<typename T>
constexpr T const* vec2_t<T>::data() const {
	return reinterpret_cast<T const*>(this);
}

template<typename T>
constexpr T& vec2_t<T>::operator[](size_t index) {
	return data()[index & 1];
}

template<typename T>
constexpr T const& vec2_t<T>::operator[](size_t index) const {
	return data()[index & 1];
}

template<typename T>
constexpr vec2_t<T> operator+(vec2_t<T> const& left, vec2_t<T> const& right) {
	return { left.x + right.x, left.y + right.y };
}

template<typename T>
constexpr vec2_t<T>& vec2_t<T>::operator+=(vec2_t const& right) {
	x += right.x;
	y += right.y;
	return *this;
}

template<typename T>
constexpr vec2_t<T> operator-(vec2_t<T> const& arg) {
	return { -arg.x, -arg.y };
}

template<typename T>
constexpr vec2_t<T> operator-(vec2_t<T> const& left, vec2_t<T> const& right) {
	return { left.x - right.x, left.y - right.y };
}

template<typename T>
constexpr vec2_t<T>& vec2_t<T>::operator-=(vec2_t const& right) {
	x -= right.x;
	y -= right.y;
	return *this;
}

template<typename T>
constexpr std::ostream& operator<<(std::ostream& stream, vec2_t<T> const& value) {
	return stream << "(" << value.x << ", " << value.y << ")";
}

template struct OpenVic::vec2_t<int64_t>;
template struct OpenVic::vec2_t<fixed_point_t>;

static_assert(sizeof(ivec2_t) == 2 * sizeof(int64_t), "ivec2_t size does not equal the sum of its parts' sizes");
static_assert(sizeof(fvec2_t) == 2 * sizeof(fixed_point_t), "fvec2_t size does not equal the sum of its parts' sizes");
