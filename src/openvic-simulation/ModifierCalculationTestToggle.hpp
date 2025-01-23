#pragma once

#include <cstdint>

// #include "openvic-simulation/types/EnumBitfield.hpp"

#define OV_MODIFIER_CALCULATION_TEST 1

/*
	What can we change about the update process?

	- provinces then countries vs countries then provinces (second should be faster as there's no extra loop over owned provinces)

	- do we update owners/add owner contributions to provinces or laeve that to= the point of looking something up?

	- map_instance.update_modifier_sums_owners(); vs country_instance_manager.update_modifier_sums_owners();
*/

namespace OpenVic {
	// enum struct update_modifier_sum_rule_t : uint8_t {
	enum update_modifier_sum_rule_t : uint8_t {
		COUNTRIES_THEN_PROVINCES = 0,
		PROVINCES_THEN_COUNTRIES = 1,

		DONT_ADD_OWNER_CONTRIBUTIONS = 0,
		ADD_OWNER_CONTRIBUTIONS = 2,

		ADD_OWNER_VIA_COUNTRIES = 0,
		ADD_OWNER_VIA_PROVINCES = 4
	};

	// using enum update_modifier_sum_rule_t;

	// template<> struct enable_bitfield<update_modifier_sum_rule_t> : std::true_type {};

	// constexpr operator bool(update_modifier_sum_rule_t UPDATE_MODIFIER_SUM_RULE) {
	// 	return static_cast<uint8_t>(UPDATE_MODIFIER_SUM_RULE) != 0;
	// }
}
