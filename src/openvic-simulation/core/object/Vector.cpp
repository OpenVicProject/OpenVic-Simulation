#include "Vector.hpp" // IWYU pragma: keep

namespace OpenVic {
#define MAKE_VEC_ALIAS(prefix, type, size) template struct vec##size##_t<type>;

#define MAKE_VEC_ALIASES(prefix, type) \
	MAKE_VEC_ALIAS(prefix, type, 2) \
	MAKE_VEC_ALIAS(prefix, type, 3) \
	MAKE_VEC_ALIAS(prefix, type, 4)

	MAKE_VEC_ALIASES(i, int32_t)
	MAKE_VEC_ALIASES(f, fixed_point_t)
	MAKE_VEC_ALIASES(d, double)

#undef MAKE_VEC_ALIASES
#undef MAKE_VEC_ALIAS
}
