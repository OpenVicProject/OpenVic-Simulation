#pragma once

#include <cmath>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Utility.hpp"

namespace OpenVic::_detail::vecn_t {
	template<typename T>
	inline constexpr T abs(T num) {
		return OpenVic::utility::abs(num);
	}

	template<>
	inline constexpr fixed_point_t abs<fixed_point_t>(fixed_point_t num) {
		return num.abs();
	}
}

#define VEC_TYPE vec2_t
#define FOR_VEC_COMPONENTS(F, SEP) F(x) SEP F(y)
#define FOR_VEC_COMPONENTS_END(F, END_F) F(x) F(y) END_F(y)
#include "VectorN.inc"

#define VEC_TYPE vec3_t
#define FOR_VEC_COMPONENTS(F, SEP) F(x) SEP F(y) SEP F(z)
#define FOR_VEC_COMPONENTS_END(F, END_F) F(x) F(y) F(z) END_F(z)
#include "VectorN.inc"

#define VEC_TYPE vec4_t
#define FOR_VEC_COMPONENTS(F, SEP) F(x) SEP F(y) SEP F(z) SEP F(w)
#define FOR_VEC_COMPONENTS_END(F, END_F) F(x) F(y) F(z) F(w) END_F(w)
#include "VectorN.inc"

namespace OpenVic {

#define MAKE_VEC_ALIAS(prefix, type, size) \
	using prefix##vec##size##_t = vec##size##_t<type>; \
	static_assert( \
		sizeof(prefix##vec##size##_t) == size * sizeof(type), \
		#prefix "vec" #size "_t size does not equal the sum of its parts' sizes" \
	); \
	extern template struct vec##size##_t<type>;

#define MAKE_VEC_ALIASES(prefix, type) \
	MAKE_VEC_ALIAS(prefix, type, 2) \
	MAKE_VEC_ALIAS(prefix, type, 3) \
	MAKE_VEC_ALIAS(prefix, type, 4)

	MAKE_VEC_ALIASES(i, int32_t)
	MAKE_VEC_ALIASES(f, fixed_point_t)
	MAKE_VEC_ALIASES(d, double)

#undef MAKE_VEC_ALIASES
#undef MAKE_VEC_ALIAS

	// Check that everything is ordered correctly in the union
	static_assert(offsetof(fvec4_t, x) == offsetof(fvec4_t, components[0]));
	static_assert(offsetof(fvec4_t, y) == offsetof(fvec4_t, components[1]));
	static_assert(offsetof(fvec4_t, z) == offsetof(fvec4_t, components[2]));
	static_assert(offsetof(fvec4_t, w) == offsetof(fvec4_t, components[3]));
}
