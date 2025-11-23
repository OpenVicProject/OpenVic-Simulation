#pragma once

#include <cstddef>
#include <cstdint>

#include <fmt/base.h>
#include <fmt/format.h>

#include <type_safe/strong_typedef.hpp>

#define TYPED_INDEX_CUSTOM(name, base_type) \
	namespace OpenVic { \
		struct name : type_safe::strong_typedef<name, base_type>, \
			type_safe::strong_typedef_op::equality_comparison<name>, \
			type_safe::strong_typedef_op::relational_comparison<name>, \
			type_safe::strong_typedef_op::integer_arithmetic<name> { \
			using strong_typedef::strong_typedef; \
		}; \
	} \
	template<> \
	struct fmt::formatter<OpenVic::name> : fmt::formatter<base_type> { \
		fmt::format_context::iterator format(OpenVic::name const& value, fmt::format_context& ctx) const { \
			return fmt::formatter<base_type>::format(type_safe::get(value), ctx); \
		} \
	};
#define TYPED_INDEX(name) TYPED_INDEX_CUSTOM(name, std::size_t)

TYPED_INDEX(bookmark_index_t)
TYPED_INDEX(building_instance_index_t)
TYPED_INDEX(building_type_index_t)
TYPED_INDEX(country_index_t)
TYPED_INDEX(crime_index_t)
TYPED_INDEX(good_index_t)
TYPED_INDEX(government_type_index_t)
TYPED_INDEX(ideology_index_t)
TYPED_INDEX(invention_index_t)
TYPED_INDEX(map_mode_index_t)
TYPED_INDEX(party_policy_index_t)
TYPED_INDEX(party_policy_group_index_t)
TYPED_INDEX(pop_type_index_t)
TYPED_INDEX(rebel_type_index_t)
TYPED_INDEX(reform_index_t)
TYPED_INDEX(reform_group_index_t)
TYPED_INDEX(regiment_type_index_t)
TYPED_INDEX(rule_index_t)
TYPED_INDEX(ship_type_index_t)
TYPED_INDEX(strata_index_t)
TYPED_INDEX(technology_index_t)
TYPED_INDEX(technology_folder_index_t)
TYPED_INDEX(terrain_type_index_t)

namespace OpenVic {
	struct province_index_t : type_safe::strong_typedef<province_index_t, std::uint16_t>,
		type_safe::strong_typedef_op::equality_comparison<province_index_t>,
		type_safe::strong_typedef_op::relational_comparison<province_index_t>,
		type_safe::strong_typedef_op::integer_arithmetic<province_index_t>,
		type_safe::strong_typedef_op::mixed_addition<province_index_t, std::uint16_t>,
		type_safe::strong_typedef_op::mixed_subtraction<province_index_t, std::uint16_t> {
		using strong_typedef::strong_typedef;
	};
}
template<>
struct fmt::formatter<OpenVic::province_index_t> : fmt::formatter<std::uint16_t> {
	fmt::format_context::iterator format(OpenVic::province_index_t const& value, fmt::format_context& ctx) const {
		return fmt::formatter<std::uint16_t>::format(type_safe::get(value), ctx);
	}
};

#undef TYPED_INDEX
#undef TYPED_INDEX_CUSTOM
