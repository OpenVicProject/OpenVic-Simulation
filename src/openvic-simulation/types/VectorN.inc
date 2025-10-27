
/* To generate a vector type this file must be included after defining:
 *  - VEC_TYPE: The name of the vector type template to generate. For example:
 *      #define VEC_TYPE vec3_t
 *  - FOR_VEC_COMPONENTS: A macro taking two arguments, a functor and a separator. The functor should be applied to the
 *    name of each of the vector's components and the separator should be placed between the functor calls. For example:
 *      #define FOR_VEC_COMPONENTS(F, SEP) F(x) SEP F(y) SEP F(z)
 */

#include <compare>
#include <ostream>

// Your IDE may show the warnings below being triggered as it doesn't have the context of where this file is included from.
// They are safe to ignore so long as they don't appear in the compiler log.

#ifndef VEC_TYPE
	#warning "VEC_TYPE not defined, defaulting to example: vec5_t"
	#define VEC_TYPE vec5_t
#endif

#ifndef FOR_VEC_COMPONENTS
	#warning "FOR_VEC_COMPONENTS not defined, defaulting to example: FOR_VEC_COMPONENTS(F, SEP) F(x) SEP F(y) SEP F(z) SEP F(w) SEP F(v)"
	#define FOR_VEC_COMPONENTS(F, SEP) F(x) SEP F(y) SEP F(z) SEP F(w) SEP F(v)
#endif

#define COMMA ,

namespace OpenVic {
	template<typename T>
	struct VEC_TYPE {
		/* PROPERTY generated getter functions will return vectors by value, rather than const reference. */
		using ov_return_by_value = void;

		using type = T;

		using iterator = basic_iterator<type*, VEC_TYPE>;
		using const_iterator = basic_iterator<type const*, VEC_TYPE>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

#define VEC_COMPONENT_ONE(v) 1
		static constexpr std::integral_constant<std::size_t, FOR_VEC_COMPONENTS(VEC_COMPONENT_ONE, +)> size = {};
#undef VEC_COMPONENT_ONE

		union {
			struct {
				T FOR_VEC_COMPONENTS(, COMMA);
			};

			T components[size];
		};

		constexpr iterator begin() {
			return iterator(components);
		}

		constexpr const_iterator begin() const {
			return const_iterator(components);
		}

		constexpr const_iterator cbegin() const {
			return begin();
		}

		constexpr iterator end() {
			return iterator(components + size());
		}

		constexpr const_iterator end() const {
			return const_iterator(components + size());
		}

		constexpr const_iterator cend() const {
			return end();
		}

		constexpr reverse_iterator rbegin() {
			return reverse_iterator(end());
		}

		constexpr const_reverse_iterator rbegin() const {
			return const_reverse_iterator(end());
		}

		constexpr const_reverse_iterator crbegin() const {
			return rbegin();
		}

		constexpr reverse_iterator rend() {
			return reverse_iterator(begin());
		}

		constexpr const_reverse_iterator rend() const {
			return const_reverse_iterator(begin());
		}

		constexpr const_reverse_iterator crend() const {
			return rend();
		}

#define VEC_CONSTRUCTOR_ARG(v) T new_##v
#define VEC_CONSTRUCTOR_INITIALISE(v) v { new_##v }
		constexpr VEC_TYPE(FOR_VEC_COMPONENTS(VEC_CONSTRUCTOR_ARG, COMMA))
			: FOR_VEC_COMPONENTS(VEC_CONSTRUCTOR_INITIALISE, COMMA) {}
#undef VEC_CONSTRUCTOR_ARG
#undef VEC_CONSTRUCTOR_INITIALISE

#define VEC_CONSTRUCTOR_SINGLE_ARG(v) new_val
		explicit constexpr VEC_TYPE(T VEC_CONSTRUCTOR_SINGLE_ARG())
			: VEC_TYPE { FOR_VEC_COMPONENTS(VEC_CONSTRUCTOR_SINGLE_ARG, COMMA) } {}
#undef VEC_CONSTRUCTOR_ARG_REPEAT

		constexpr VEC_TYPE() : VEC_TYPE { {} } {}

		constexpr VEC_TYPE(VEC_TYPE const&) = default;
		constexpr VEC_TYPE(VEC_TYPE&&) = default;
		constexpr VEC_TYPE& operator=(VEC_TYPE const&) = default;
		constexpr VEC_TYPE& operator=(VEC_TYPE&&) = default;

#define VEC_COMPONENT_CAST(v) static_cast<S>(v)
		template<typename S>
		constexpr explicit operator VEC_TYPE<S>() const {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_CAST, COMMA) };
		}
#undef VEC_COMPONENT_CAST

#define VEC_COMPONENT_ABS(v) OpenVic::abs(v)
		constexpr VEC_TYPE abs() const {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_ABS, COMMA) };
		}
#undef VEC_COMPONENT_ABS

#define VEC_COMPONENT_SQUARE(v) v * v
		constexpr T length_squared() const {
			return FOR_VEC_COMPONENTS(VEC_COMPONENT_SQUARE, +);
		}
#undef VEC_COMPONENT_SQUARE

#define VEC_COMPONENT_DIST_SQUARE(v) (v - vec.v) * (v - vec.v)
		template<typename OtherT>
		constexpr T distance_squared(VEC_TYPE<OtherT> const& vec) const {
			return FOR_VEC_COMPONENTS(VEC_COMPONENT_DIST_SQUARE, +);
		}
#undef VEC_COMPONENT_DIST_SQUARE

#define VEC_COMPONENT_DOT(v) v * vec.v
		template<typename S>
		constexpr T dot(VEC_TYPE<S> const& vec) const {
			return FOR_VEC_COMPONENTS(VEC_COMPONENT_DOT, +);
		}
#undef VEC_COMPONENT_DOT

#define VEC_COMPONENT_NONNEGATIVE(v) v >= 0
		constexpr bool nonnegative() const {
			return FOR_VEC_COMPONENTS(VEC_COMPONENT_NONNEGATIVE, &&);
		}
#undef VEC_COMPONENT_NONNEGATIVE

#define VEC_COMPONENT_COMPARE(v) if (left.v != right.v) return left.v <=> right.v;
#define VEC_COMPONENT_COMPARE_END(v) return left.v <=> right.v
		constexpr friend auto operator<=>(VEC_TYPE const& left, VEC_TYPE const& right) {
			FOR_VEC_COMPONENTS_END(VEC_COMPONENT_COMPARE, VEC_COMPONENT_COMPARE_END);
		}
#undef VEC_COMPONENT_COMPARE
#undef VEC_COMPONENT_COMPARE_END

#define VEC_COMPONENT_COMPARE(v) if (v != right.v) return v <=> right.v;
#define VEC_COMPONENT_COMPARE_END(v) return v <=> right.v
		template<typename S>
		constexpr auto operator<=>(VEC_TYPE<S> const& right) const {
			FOR_VEC_COMPONENTS_END(VEC_COMPONENT_COMPARE, VEC_COMPONENT_COMPARE_END);
		}
#undef VEC_COMPONENT_COMPARE
#undef VEC_COMPONENT_COMPARE_END

#define VEC_COMPONENT_BOUND(v) v < check.v
		/* Checks that all coordinates are less than their corresponding values in the argument vector.
		 * This is intended for checking coordinates lie within certain bounds, it is not suitable for sorting vectors. */
		template<typename S>
		constexpr bool is_within_bound(VEC_TYPE<S> const& check) const {
			return FOR_VEC_COMPONENTS(VEC_COMPONENT_BOUND, &&);
		}
#undef VEC_COMPONENT_BOUND

		constexpr T* data() {
			return components;
		}

		constexpr T const* data() const {
			return components;
		}

		constexpr T& operator[](size_t index) {
			return components[index % size];
		}

		constexpr T const& operator[](size_t index) const {
			return components[index % size];
		}

#define VEC_COMPONENT_ADD(v) left.v + right.v
		template<typename S>
		constexpr friend VEC_TYPE operator+(VEC_TYPE const& left, VEC_TYPE<S> const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_ADD, COMMA) };
		}
#undef VEC_COMPONENT_ADD

#define VEC_COMPONENT_ADDEQ(v) v += right.v;
		template<typename S>
		constexpr VEC_TYPE& operator+=(VEC_TYPE<S> const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_ADDEQ,)
			return *this;
		}
#undef VEC_COMPONENT_ADDEQ

#define VEC_COMPONENT_ADD_RIGHT(v) left.v + right
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator+(VEC_TYPE const& left, S const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_ADD_RIGHT, COMMA) };
		}
#undef VEC_COMPONENT_ADD_RIGHT

#define VEC_COMPONENT_ADD_LEFT(v) left + right.v
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator+(S const& left, VEC_TYPE const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_ADD_LEFT, COMMA) };
		}
#undef VEC_COMPONENT_ADD_LEFT

#define VEC_COMPONENT_ADDEQ_RIGHT(v) v += right;
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr VEC_TYPE& operator+=(S const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_ADDEQ_RIGHT,)
			return *this;
		}
#undef VEC_COMPONENT_ADDEQ_RIGHT

#define VEC_COMPONENT_NEGATE(v) -arg.v
		constexpr friend VEC_TYPE operator-(VEC_TYPE const& arg) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_NEGATE, COMMA) };
		}
#undef VEC_COMPONENT_NEGATE

#define VEC_COMPONENT_SUB(v) left.v - right.v
		template<typename S>
		constexpr friend VEC_TYPE operator-(VEC_TYPE const& left, VEC_TYPE<S> const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_SUB, COMMA) };
		}
#undef VEC_COMPONENT_SUB

#define VEC_COMPONENT_SUBEQ(v) v -= right.v;
		template<typename S>
		constexpr VEC_TYPE& operator-=(VEC_TYPE<S> const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_SUBEQ,)
			return *this;
		}
#undef VEC_COMPONENT_SUBEQ

#define VEC_COMPONENT_SUB_RIGHT(v) left.v - right
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator-(VEC_TYPE const& left, S const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_SUB_RIGHT, COMMA) };
		}
#undef VEC_COMPONENT_SUB_RIGHT

#define VEC_COMPONENT_SUB_LEFT(v) left - right.v
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator-(S const& left, VEC_TYPE const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_SUB_LEFT, COMMA) };
		}
#undef VEC_COMPONENT_SUB_LEFT

#define VEC_COMPONENT_SUBEQ_RIGHT(v) v -= right;
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr VEC_TYPE& operator-=(S const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_SUBEQ_RIGHT,)
			return *this;
		}
#undef VEC_COMPONENT_SUBEQ_RIGHT

#define VEC_COMPONENT_MUL(v) left.v * right.v
		template<typename S>
		constexpr friend VEC_TYPE operator*(VEC_TYPE const& left, VEC_TYPE<S> const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_MUL, COMMA) };
		}
#undef VEC_COMPONENT_MUL

#define VEC_COMPONENT_MULEQ(v) v *= right.v;
		template<typename S>
		constexpr VEC_TYPE& operator*=(VEC_TYPE<S> const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_MULEQ,)
			return *this;
		}
#undef VEC_COMPONENT_MULEQ

#define VEC_COMPONENT_MUL_RIGHT(v) left.v * right
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator*(VEC_TYPE const& left, S const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_MUL_RIGHT, COMMA) };
		}
#undef VEC_COMPONENT_MUL_RIGHT

#define VEC_COMPONENT_MUL_LEFT(v) left * right.v
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator*(S const& left, VEC_TYPE const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_MUL_LEFT, COMMA) };
		}
#undef VEC_COMPONENT_MUL_LEFT

#define VEC_COMPONENT_MULEQ_RIGHT(v) v *= right;
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr VEC_TYPE& operator*=(S const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_MULEQ_RIGHT,)
			return *this;
		}
#undef VEC_COMPONENT_MULEQ_RIGHT

#define VEC_COMPONENT_DIV(v) left.v / right.v
		template<typename S>
		constexpr friend VEC_TYPE operator/(VEC_TYPE const& left, VEC_TYPE<S> const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_DIV, COMMA) };
		}
#undef VEC_COMPONENT_DIV

#define VEC_COMPONENT_DIVEQ(v) v /= right.v;
		template<typename S>
		constexpr VEC_TYPE& operator/=(VEC_TYPE<S> const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_DIVEQ,)
			return *this;
		}
#undef VEC_COMPONENT_DIVEQ

#define VEC_COMPONENT_DIV_RIGHT(v) left.v / right
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator/(VEC_TYPE const& left, S const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_DIV_RIGHT, COMMA) };
		}
#undef VEC_COMPONENT_DIV_RIGHT

#define VEC_COMPONENT_DIV_LEFT(v) left / right.v
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr friend VEC_TYPE operator/(S const& left, VEC_TYPE const& right) {
			return { FOR_VEC_COMPONENTS(VEC_COMPONENT_DIV_LEFT, COMMA) };
		}
#undef VEC_COMPONENT_DIV_LEFT

#define VEC_COMPONENT_DIVEQ_RIGHT(v) v /= right;
		template<not_same_as<VEC_TYPE<T>> S>
		constexpr VEC_TYPE& operator/=(S const& right) {
			FOR_VEC_COMPONENTS(VEC_COMPONENT_DIVEQ_RIGHT,)
			return *this;
		}
#undef VEC_COMPONENT_DIVEQ_RIGHT

#define VEC_COMPONENT_EQ(v) left.v == right.v
		constexpr friend bool operator==(VEC_TYPE const& left, VEC_TYPE const& right) {
			return FOR_VEC_COMPONENTS(VEC_COMPONENT_EQ, &&);
		}

		template<typename S>
		constexpr friend bool operator==(VEC_TYPE const& left, VEC_TYPE<S> const& right) {
			return FOR_VEC_COMPONENTS(VEC_COMPONENT_EQ, &&);
		}
#undef VEC_COMPONENT_EQ

#define VEC_COMPONENT_PRINT(v) << value.v
		constexpr friend std::ostream& operator<<(std::ostream& stream, VEC_TYPE const& value) {
			return stream << "(" FOR_VEC_COMPONENTS(VEC_COMPONENT_PRINT, << ", ") << ")";
		}
#undef VEC_COMPONENT_PRINT
	};
}

#undef COMMA
