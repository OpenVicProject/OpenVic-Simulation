#pragma once

#include <cstdint>

#include <fmt/base.h>
#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
	struct combat_width_t : type_safe::strong_typedef<combat_width_t, std::int8_t>,
						   type_safe::strong_typedef_op::equality_comparison<combat_width_t>,
						   type_safe::strong_typedef_op::relational_comparison<combat_width_t>,
						   type_safe::strong_typedef_op::integer_arithmetic<combat_width_t>,
						   type_safe::strong_typedef_op::mixed_addition<combat_width_t, std::uint8_t>,
						   type_safe::strong_typedef_op::mixed_subtraction<combat_width_t, std::uint8_t> {
		using strong_typedef::strong_typedef;
	};
}

template<>
struct fmt::formatter<OpenVic::combat_width_t> : fmt::formatter<std::int32_t> {
	fmt::format_context::iterator format(OpenVic::combat_width_t const& value, fmt::format_context& ctx) const {
		return fmt::formatter<std::int32_t>::format(type_safe::get(value), ctx);
	}
};