#pragma once

#include <fmt/base.h>

namespace ovfmt {
	template<typename T>
	struct validate_view {
		T const* ptr;
		fmt::string_view invalid_string;
	};

	template<typename T>
	validate_view<T> validate(T const* ptr, fmt::string_view invalid_string = "<NULL>") {
		return { ptr, invalid_string };
	}
}

template<typename T>
struct fmt::formatter<ovfmt::validate_view<T>> : fmt::formatter<T> {
	fmt::format_context::iterator format(ovfmt::validate_view<T> const& value, fmt::format_context& ctx) const {
		return value.ptr ? fmt::formatter<T>::format(*value.ptr, ctx)
						 : fmt::formatter<fmt::string_view> {}.format(value.invalid_string, ctx);
	}
};
