#pragma once

#include <cstdint>
#include <type_traits>

#include <fmt/base.h>
#include <fmt/format.h>

#include "openvic-simulation/population/PopSize.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {
	namespace detail {
		template<typename T, typename SelfT>
		struct basic_pop_sum_t : type_safe::strong_typedef<basic_pop_sum_t<T, SelfT>, T>,
								 type_safe::strong_typedef_op::equality_comparison<SelfT>,
								 type_safe::strong_typedef_op::relational_comparison<SelfT>,
								 type_safe::strong_typedef_op::integer_arithmetic<SelfT>,
								 type_safe::strong_typedef_op::mixed_relational_comparison<SelfT, std::make_unsigned_t<T>>,
								 type_safe::strong_typedef_op::mixed_equality_comparison<SelfT, fixed_point_t>,
								 type_safe::strong_typedef_op::mixed_equality_comparison<SelfT, pop_size_t> {
			using type_safe::strong_typedef<basic_pop_sum_t<T, SelfT>, T>::strong_typedef;

			constexpr basic_pop_sum_t(std::same_as<std::int32_t> auto value)
				: type_safe::strong_typedef<basic_pop_sum_t<T, SelfT>, T>(value) {}

			constexpr basic_pop_sum_t(pop_size_t value)
				: type_safe::strong_typedef<basic_pop_sum_t<T, SelfT>, T>(type_safe::get(value)) {}

			using ov_return_by_value = void;

			friend inline constexpr fixed_point_t operator*( //
				const SelfT lhs, const std::same_as<fixed_point_t> auto rhs
			) {
				return type_safe::get(lhs) * rhs;
			}

			friend inline constexpr fixed_point_t operator*( //
				const std::same_as<fixed_point_t> auto lhs, const SelfT rhs
			) {
				return lhs * type_safe::get(rhs);
			}

			friend inline constexpr fixed_point_t& operator*=(std::same_as<fixed_point_t> auto lhs, const SelfT rhs) {
				lhs *= type_safe::get(rhs);
				return lhs;
			}

			friend inline constexpr fixed_point_t operator/( //
				const SelfT lhs, const std::same_as<fixed_point_t> auto rhs
			) {
				return type_safe::get(lhs) / rhs;
			}

			friend inline constexpr fixed_point_t operator/( //
				const std::same_as<fixed_point_t> auto lhs, const SelfT rhs
			) {
				return lhs / type_safe::get(rhs);
			}

			friend inline constexpr fixed_point_t& operator/=(std::same_as<fixed_point_t> auto& lhs, const SelfT rhs) {
				lhs /= type_safe::get(rhs);
				return lhs;
			}

			friend inline constexpr bool operator==(const std::same_as<fixed_point_t> auto lhs, const SelfT rhs) {
				return lhs == type_safe::get(rhs);
			}

			friend inline constexpr bool operator==(const std::same_as<std::int32_t> auto lhs, const SelfT rhs) {
				return lhs == type_safe::get(rhs);
			}

			friend inline constexpr auto operator<=>(const std::same_as<std::int32_t> auto lhs, const SelfT rhs) {
				return lhs <=> type_safe::get(rhs);
			}

			friend inline constexpr auto operator<=>(const SelfT lhs, const SelfT rhs) {
				return type_safe::get(lhs) <=> type_safe::get(rhs);
			}

			friend inline constexpr auto operator<=>(const SelfT lhs, const pop_size_t rhs) {
				return type_safe::get(lhs) <=> type_safe::get(rhs);
			}
		};
	}

	struct pop_sum_t : detail::basic_pop_sum_t<std::int64_t, pop_sum_t> {
		using basic_pop_sum_t::basic_pop_sum_t;
	};

	struct upop_sum_t : detail::basic_pop_sum_t<std::uint64_t, upop_sum_t> {
		using basic_pop_sum_t::basic_pop_sum_t;
	};
}

template<>
struct fmt::formatter<OpenVic::pop_sum_t> : fmt::formatter<std::int64_t> {
	fmt::format_context::iterator format(OpenVic::pop_sum_t value, fmt::format_context& ctx) const {
		return fmt::formatter<std::int64_t>::format(type_safe::get(value), ctx);
	}
};

template<>
struct fmt::formatter<OpenVic::upop_sum_t> : fmt::formatter<std::uint64_t> {
	fmt::format_context::iterator format(OpenVic::upop_sum_t value, fmt::format_context& ctx) const {
		return fmt::formatter<std::uint64_t>::format(type_safe::get(value), ctx);
	}
};
