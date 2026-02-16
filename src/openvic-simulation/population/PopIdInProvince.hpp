#pragma once

#include <cstddef>

#include <fmt/base.h>
#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
	struct pop_id_in_province_t : type_safe::strong_typedef<pop_id_in_province_t, std::size_t>,
		type_safe::strong_typedef_op::equality_comparison<pop_id_in_province_t>,
		type_safe::strong_typedef_op::relational_comparison<pop_id_in_province_t>,
		type_safe::strong_typedef_op::integer_arithmetic<pop_id_in_province_t> {
		using strong_typedef::strong_typedef;

		constexpr bool is_null() const { return type_safe::get(*this) == 0; }
    	constexpr bool operator!() const { return is_null(); }
	};
}
namespace std {
	template <>
	struct hash<OpenVic::pop_id_in_province_t> : type_safe::hashable<OpenVic::pop_id_in_province_t> {};
}
template<>
struct fmt::formatter<OpenVic::pop_id_in_province_t> : fmt::formatter<std::size_t> {
	fmt::format_context::iterator format(OpenVic::pop_id_in_province_t const& value, fmt::format_context& ctx) const {
		return fmt::formatter<std::size_t>::format(type_safe::get(value), ctx);
	}
};