#include "FixedPoint.hpp"

#include <fmt/format.h>

using namespace OpenVic;

fixed_point_t::stack_string fixed_point_t::to_array(size_t decimal_places) const {
	stack_string str {};
	std::to_chars_result result = to_chars(str.array.data(), str.array.data() + str.array.size(), decimal_places);
	str.string_size = result.ptr - str.data();
	return str;
}

auto fmt::formatter<fixed_point_t>::format(fixed_point_t fp, format_context& ctx) const -> format_context::iterator {
	fixed_point_t::stack_string result = fp.to_array();
	if (OV_unlikely(result.empty())) {
		return formatter<string_view>::format(string_view {}, ctx);
	}

	return formatter<string_view>::format(string_view { result.data(), result.size() }, ctx);
}
