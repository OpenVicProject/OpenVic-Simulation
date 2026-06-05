#include "openvic-simulation/ecs/SystemAccess.hpp"

#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

TEST_CASE("access_overlaps detects W/W and W/R", "[ecs][SystemAccess]") {
	std::vector<ComponentAccess> a = {
		ComponentAccess { 100, AccessMode::Write },
		ComponentAccess { 200, AccessMode::Read }
	};
	std::vector<ComponentAccess> b_ww = {
		ComponentAccess { 100, AccessMode::Write }
	};
	std::vector<ComponentAccess> b_rw = {
		ComponentAccess { 200, AccessMode::Write }
	};
	std::vector<ComponentAccess> b_rr = {
		ComponentAccess { 200, AccessMode::Read }
	};
	std::vector<ComponentAccess> b_disjoint = {
		ComponentAccess { 999, AccessMode::Write }
	};

	CHECK(access_overlaps(a, b_ww));
	CHECK(access_overlaps(a, b_rw));
	CHECK_FALSE(access_overlaps(a, b_rr));
	CHECK_FALSE(access_overlaps(a, b_disjoint));
}

TEST_CASE("access_conflict_components reports overlapping ids", "[ecs][SystemAccess]") {
	std::vector<ComponentAccess> a = {
		ComponentAccess { 100, AccessMode::Write },
		ComponentAccess { 200, AccessMode::Read }
	};
	std::vector<ComponentAccess> b = {
		ComponentAccess { 100, AccessMode::Read },
		ComponentAccess { 200, AccessMode::Write },
		ComponentAccess { 300, AccessMode::Read }
	};
	std::vector<component_type_id_t> conflict = access_conflict_components(a, b);
	REQUIRE(conflict.size() == 2u);
	CHECK(conflict[0] == 100);
	CHECK(conflict[1] == 200);
}

TEST_CASE("merge_extra_reads adds Read entries; W coalesces over R", "[ecs][SystemAccess]") {
	std::vector<ComponentAccess> set = {
		ComponentAccess { 100, AccessMode::Write }
	};
	std::vector<component_type_id_t> extras = { 100, 200 };
	merge_extra_reads(set, extras);
	canonicalise_access_set(set);

	REQUIRE(set.size() == 2u);
	// 100 stays as Write; 200 gets a Read.
	CHECK(set[0].component_id == 100);
	CHECK(set[0].mode == AccessMode::Write);
	CHECK(set[1].component_id == 200);
	CHECK(set[1].mode == AccessMode::Read);
}

TEST_CASE("canonicalise_access_set sorts and dedupes; W beats R", "[ecs][SystemAccess]") {
	std::vector<ComponentAccess> set = {
		ComponentAccess { 200, AccessMode::Read },
		ComponentAccess { 100, AccessMode::Read },
		ComponentAccess { 100, AccessMode::Write },
		ComponentAccess { 200, AccessMode::Read }
	};
	canonicalise_access_set(set);
	REQUIRE(set.size() == 2u);
	CHECK(set[0].component_id == 100);
	CHECK(set[0].mode == AccessMode::Write);
	CHECK(set[1].component_id == 200);
	CHECK(set[1].mode == AccessMode::Read);
}
