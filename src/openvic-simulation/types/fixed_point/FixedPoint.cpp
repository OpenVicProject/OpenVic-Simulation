#include "FixedPoint.hpp"

#include <fmt/format.h>

using namespace OpenVic;

auto fmt::formatter<fixed_point_t>::format(fixed_point_t fp, format_context& ctx) const -> format_context::iterator {
	fixed_point_t::stack_string result = fp.to_array();
	if (OV_unlikely(result.empty())) {
		return formatter<string_view>::format(string_view {}, ctx);
	}

	return formatter<string_view>::format(string_view { result.data(), result.size() }, ctx);
}
