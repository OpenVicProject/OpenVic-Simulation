#include "openvic-simulation/types/TypedIndices.hpp"

#include <cstdint>

#include <type_safe/strong_typedef.hpp>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

static_assert(sizeof(bookmark_index_t) == 4);
static_assert(sizeof(building_type_index_t) == 4);
static_assert(sizeof(province_building_index_t) == 4);
static_assert(sizeof(country_index_t) == 4);
static_assert(sizeof(crime_index_t) == 4);
static_assert(sizeof(good_index_t) == 4);
static_assert(sizeof(good_category_index_t) == 4);
static_assert(sizeof(government_type_index_t) == 4);
static_assert(sizeof(graphical_culture_index_t) == 4);
static_assert(sizeof(ideology_index_t) == 4);
static_assert(sizeof(invention_index_t) == 4);
static_assert(sizeof(map_mode_index_t) == 4);
static_assert(sizeof(party_policy_index_t) == 4);
static_assert(sizeof(party_policy_group_index_t) == 4);
static_assert(sizeof(pop_type_index_t) == 4);
static_assert(sizeof(rebel_type_index_t) == 4);
static_assert(sizeof(reform_index_t) == 4);
static_assert(sizeof(reform_group_index_t) == 4);
static_assert(sizeof(regiment_type_index_t) == 4);
static_assert(sizeof(ship_type_index_t) == 4);
static_assert(sizeof(strata_index_t) == 4);
static_assert(sizeof(technology_index_t) == 4);
static_assert(sizeof(technology_folder_index_t) == 4);
static_assert(sizeof(terrain_type_index_t) == 4);
static_assert(sizeof(unit_type_index_t) == 4);
static_assert(sizeof(province_index_t) == 2);

TEST_CASE("TypedIndices index_from_count round trip", "[TypedIndices]") {
	CHECK(type_safe::get(index_from_count<good_index_t>(0)) == 0);
	CHECK(type_safe::get(index_from_count<good_index_t>(42)) == 42);
	CHECK(type_safe::get(index_from_count<country_index_t>(65535)) == 65535);
	CHECK(type_safe::get(index_from_count<country_index_t>(100000)) == 100000);
	CHECK(index_from_count<ideology_index_t>(7) == ideology_index_t { static_cast<std::uint32_t>(7) });
}
