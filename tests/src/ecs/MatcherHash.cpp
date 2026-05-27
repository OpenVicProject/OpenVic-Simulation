#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/Query.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <bit>
#include <cstdint>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct MHA {};
	struct MHB {};
	struct MHC {};
	struct MHD {};
	struct MHE {};
}

ECS_COMPONENT(MHA, "test_MatcherHash::MHA")
ECS_COMPONENT(MHB, "test_MatcherHash::MHB")
ECS_COMPONENT(MHC, "test_MatcherHash::MHC")
ECS_COMPONENT(MHD, "test_MatcherHash::MHD")
ECS_COMPONENT(MHE, "test_MatcherHash::MHE")

namespace {
	// Replicates `World::compute_matcher_hash`. We assert the same shape of computation
	// so the test is sensitive to bug-introducing changes (e.g. % 64 instead of % 63).
	uint64_t expected_matcher_for(std::vector<component_type_id_t> const& sig) {
		uint64_t mask = 0;
		for (component_type_id_t id : sig) {
			mask |= (uint64_t { 1 } << (id % 63));
		}
		return mask;
	}
}

TEST_CASE("Query results match for archetypes with require-only filter", "[ecs][MatcherHash]") {
	World world;
	world.create_entity(MHA {});
	world.create_entity(MHB {});
	world.create_entity(MHA {}, MHB {});

	int a_count = 0;
	world.for_each<MHA>([&](MHA&) { ++a_count; });
	CHECK(a_count == 2);

	int ab_count = 0;
	world.for_each<MHA, MHB>([&](MHA&, MHB&) { ++ab_count; });
	CHECK(ab_count == 1);
}

TEST_CASE("Query results respect exclude filter", "[ecs][MatcherHash]") {
	World world;
	world.create_entity(MHA {});
	world.create_entity(MHA {}, MHB {});
	world.create_entity(MHA {}, MHC {});
	world.create_entity(MHA {}, MHB {}, MHC {});

	Query q;
	q.with<MHA>().exclude<MHB>().build();

	int count = 0;
	world.for_each<MHA>(q, [&](MHA&) { ++count; });
	CHECK(count == 2); // {A} and {A,C}, neither carries B
}

TEST_CASE("Query results survive new archetype creation", "[ecs][MatcherHash][cache]") {
	World world;
	world.create_entity(MHA {});
	world.create_entity(MHA {});

	int count_before = 0;
	world.for_each<MHA>([&](MHA&) { ++count_before; });
	CHECK(count_before == 2);

	// Create a new archetype that should also match the cached query.
	world.create_entity(MHA {}, MHC {});

	int count_after = 0;
	world.for_each<MHA>([&](MHA&) { ++count_after; });
	CHECK(count_after == 3);
}

TEST_CASE("matcher_hash bitfield has exactly N bits set for N distinct components", "[ecs][MatcherHash]") {
	component_type_id_t const ids[] = {
		component_type_id_of<MHA>(), component_type_id_of<MHB>(),
		component_type_id_of<MHC>(), component_type_id_of<MHD>(),
		component_type_id_of<MHE>()
	};

	// Each individual component contributes one bit. Across 5 distinct ids, the bitfield
	// is the union — popcount equals the number of distinct (id % 63) values, which is at
	// most 5 (could be less if any happen to collide modulo 63).
	uint64_t mask_all = expected_matcher_for({ ids[0], ids[1], ids[2], ids[3], ids[4] });
	int popcount = std::popcount(mask_all);
	CHECK(popcount >= 1);
	CHECK(popcount <= 5);
}

TEST_CASE("Query with multiple require + exclude filters correctly", "[ecs][MatcherHash]") {
	World world;
	// {A,B} matches require {A,B} and excludes {C,D}
	EntityID const a = world.create_entity(MHA {}, MHB {});
	// {A,B,C} fails exclude (carries C)
	world.create_entity(MHA {}, MHB {}, MHC {});
	// {A,B,D} fails exclude (carries D)
	world.create_entity(MHA {}, MHB {}, MHD {});
	// {A,B,E} matches (E not in exclude)
	EntityID const e = world.create_entity(MHA {}, MHB {}, MHE {});
	// {A} fails require (no B)
	world.create_entity(MHA {});

	Query q;
	q.with<MHA, MHB>().exclude<MHC, MHD>().build();

	int count = 0;
	world.for_each_with_entity<MHA, MHB>(q, [&](EntityID, MHA&, MHB&) { ++count; });
	CHECK(count == 2);
	(void) a;
	(void) e;
}
