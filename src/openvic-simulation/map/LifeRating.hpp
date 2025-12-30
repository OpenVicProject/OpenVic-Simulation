#pragma once

#include <cstdint>

#include <fmt/base.h>
#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
	struct life_rating_t : type_safe::strong_typedef<life_rating_t, std::int8_t>,
						   type_safe::strong_typedef_op::equality_comparison<life_rating_t>,
						   type_safe::strong_typedef_op::relational_comparison<life_rating_t>,
						   type_safe::strong_typedef_op::integer_arithmetic<life_rating_t>,
						   type_safe::strong_typedef_op::mixed_addition<life_rating_t, std::uint8_t>,
						   type_safe::strong_typedef_op::mixed_subtraction<life_rating_t, std::uint8_t> {
		using strong_typedef::strong_typedef;
	};
}

template<>
struct fmt::formatter<OpenVic::life_rating_t> : fmt::formatter<std::int8_t> {
	fmt::format_context::iterator format(OpenVic::life_rating_t const& value, fmt::format_context& ctx) const {
		return fmt::formatter<std::int8_t>::format(type_safe::get(value), ctx);
	}
};
