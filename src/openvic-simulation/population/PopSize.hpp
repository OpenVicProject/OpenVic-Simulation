#pragma once

#include <cstdint>

#include <fmt/base.h>
#include <fmt/format.h>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
	struct pop_size_t : type_safe::strong_typedef<pop_size_t, std::int32_t>,
						type_safe::strong_typedef_op::equality_comparison<pop_size_t>,
						type_safe::strong_typedef_op::relational_comparison<pop_size_t>,
						type_safe::strong_typedef_op::integer_arithmetic<pop_size_t>,
						type_safe::strong_typedef_op::mixed_relational_comparison<pop_size_t, std::uint32_t>,
						type_safe::strong_typedef_op::mixed_equality_comparison<pop_size_t, fixed_point_t> {
		using strong_typedef::strong_typedef;
		constexpr pop_size_t(std::same_as<std::int32_t> auto value) : strong_typedef(value) {}

		using ov_return_by_value = void;

		friend inline constexpr fixed_point_t operator*(const pop_size_t lhs, const std::same_as<fixed_point_t> auto rhs) {
			return type_safe::get(lhs) * rhs;
		}

		friend inline constexpr fixed_point_t operator*(const std::same_as<fixed_point_t> auto lhs, const pop_size_t rhs) {
			return lhs * type_safe::get(rhs);
		}

		friend inline constexpr fixed_point_t& operator*=(std::same_as<fixed_point_t> auto lhs, const pop_size_t rhs) {
			lhs *= type_safe::get(rhs);
			return lhs;
		}

		friend inline constexpr fixed_point_t operator/(const pop_size_t lhs, const std::same_as<fixed_point_t> auto rhs) {
			return type_safe::get(lhs) / rhs;
		}

		friend inline constexpr fixed_point_t operator/(const std::same_as<fixed_point_t> auto lhs, const pop_size_t rhs) {
			return lhs / type_safe::get(rhs);
		}

		friend inline constexpr fixed_point_t& operator/=(std::same_as<fixed_point_t> auto lhs, const pop_size_t rhs) {
			lhs /= type_safe::get(rhs);
			return lhs;
		}

		friend inline constexpr bool operator==(const std::same_as<std::int32_t> auto lhs, const pop_size_t rhs) {
			return lhs == type_safe::get(rhs);
		}

		friend inline constexpr auto operator<=>(const std::same_as<std::int32_t> auto lhs, const pop_size_t rhs) {
			return lhs <=> type_safe::get(rhs);
		}

		friend inline constexpr auto operator<=>(const pop_size_t lhs, const pop_size_t rhs) {
			return type_safe::get(lhs) <=> type_safe::get(rhs);
		}
	};
}

template<>
struct fmt::formatter<OpenVic::pop_size_t> : fmt::formatter<std::int32_t> {
	fmt::format_context::iterator format(OpenVic::pop_size_t value, fmt::format_context& ctx) const {
		return fmt::formatter<std::int32_t>::format(type_safe::get(value), ctx);
	}
};
