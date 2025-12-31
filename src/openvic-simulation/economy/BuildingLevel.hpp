#pragma once

#include <cstdint>

#include <fmt/base.h>
#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
	struct building_level_t : type_safe::strong_typedef<building_level_t, std::int16_t>,
							  type_safe::strong_typedef_op::equality_comparison<building_level_t>,
							  type_safe::strong_typedef_op::relational_comparison<building_level_t>,
							  type_safe::strong_typedef_op::integer_arithmetic<building_level_t>,
							  type_safe::strong_typedef_op::mixed_addition<building_level_t, std::uint16_t>,
							  type_safe::strong_typedef_op::mixed_subtraction<building_level_t, std::uint16_t> {
		using strong_typedef::strong_typedef;
	};
}

template<>
struct fmt::formatter<OpenVic::building_level_t> : fmt::formatter<std::int16_t> {
	fmt::format_context::iterator format(OpenVic::building_level_t const& value, fmt::format_context& ctx) const {
		return fmt::formatter<std::int16_t>::format(type_safe::get(value), ctx);
	}
};
