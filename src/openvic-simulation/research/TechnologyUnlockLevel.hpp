#pragma once

#include <cstdint>

#include <fmt/base.h>
#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
	struct technology_unlock_level_t
		: type_safe::strong_typedef<technology_unlock_level_t, std::int8_t>,
		  type_safe::strong_typedef_op::equality_comparison<technology_unlock_level_t>,
		  type_safe::strong_typedef_op::relational_comparison<technology_unlock_level_t>,
		  type_safe::strong_typedef_op::integer_arithmetic<technology_unlock_level_t>,
		  type_safe::strong_typedef_op::mixed_addition<technology_unlock_level_t, std::uint8_t>,
		  type_safe::strong_typedef_op::mixed_subtraction<technology_unlock_level_t, std::uint8_t>,
		  type_safe::strong_typedef_op::mixed_relational_comparison<technology_unlock_level_t, std::int32_t>,
		  type_safe::strong_typedef_op::mixed_equality_comparison<technology_unlock_level_t, std::int32_t> {
		using strong_typedef::strong_typedef;
	};
}

template<>
struct fmt::formatter<OpenVic::technology_unlock_level_t> : fmt::formatter<std::int32_t> {
	fmt::format_context::iterator format(OpenVic::technology_unlock_level_t const& value, fmt::format_context& ctx) const {
		return fmt::formatter<std::int32_t>::format(type_safe::get(value), ctx);
	}
};
