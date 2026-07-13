#include "openvic-simulation/ecs/CachedRef.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct VA {
		int v = 0;
	};
	struct VB {
		int w = 0;
	};
	struct VTag {};
}

ECS_COMPONENT(VA, "test_CachedRef::VA")
ECS_COMPONENT(VB, "test_CachedRef::VB")
ECS_COMPONENT(VTag, "test_CachedRef::VTag")

TEST_CASE("component_version_in returns 0 for dead entity", "[ecs][World][version]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	world.destroy_entity(eid);
	CHECK(world.component_version_in<VA>(eid) == 0u);
}

TEST_CASE("component_version_in returns 0 for missing component", "[ecs][World][version]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	CHECK(world.component_version_in<VB>(eid) == 0u);
}

TEST_CASE("component_version_in returns 0 for invalid EntityID", "[ecs][World][version]") {
	World world;
	CHECK(world.component_version_in<VA>(INVALID_ENTITY_ID) == 0u);
	CHECK(world.component_version_in<VA>(EntityID { 999, 1 }) == 0u);
}

TEST_CASE("component_version_in is non-zero immediately after create_entity", "[ecs][World][version]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	CHECK(world.component_version_in<VA>(eid) > 0u);
}

TEST_CASE("component_version_in increases when another entity is created in the same archetype", "[ecs][World][version]") {
	World world;
	EntityID const a = world.create_entity(VA { 1 });
	uint64_t const v0 = world.component_version_in<VA>(a);

	world.create_entity(VA { 2 });
	uint64_t const v1 = world.component_version_in<VA>(a);
	CHECK(v1 > v0);
}

TEST_CASE("component_version_in increases when an entity is destroyed in the archetype", "[ecs][World][version]") {
	World world;
	EntityID const a = world.create_entity(VA { 1 });
	EntityID const b = world.create_entity(VA { 2 });

	uint64_t const v0 = world.component_version_in<VA>(a);
	world.destroy_entity(b);
	uint64_t const v1 = world.component_version_in<VA>(a);
	CHECK(v1 > v0);
}

TEST_CASE("component_version_in unchanged across in-place mutations", "[ecs][World][version]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	uint64_t const v0 = world.component_version_in<VA>(eid);

	world.get_component<VA>(eid)->v = 42;
	uint64_t const v1 = world.component_version_in<VA>(eid);
	CHECK(v0 == v1); // structural mutation is what bumps, not field writes
}

TEST_CASE("component_version_in tracks tag columns too", "[ecs][World][version][tag]") {
	World world;
	EntityID const a = world.create_entity(VA { 1 }, VTag {});
	uint64_t const v0 = world.component_version_in<VTag>(a);
	CHECK(v0 > 0u);

	world.create_entity(VA { 2 }, VTag {});
	uint64_t const v1 = world.component_version_in<VTag>(a);
	CHECK(v1 > v0);
}

TEST_CASE("CachedRef::from caches the component pointer", "[ecs][CachedRef]") {
	World world;
	EntityID const eid = world.create_entity(VA { 7 });
	auto ref = CachedRef<VA>::from(world, eid);

	CHECK(ref.entity() == eid);
	CHECK(ref.is_valid(world));

	VA* p = ref.get(world);
	CHECK(p != nullptr);
	CHECK(p->v == 7);
}

TEST_CASE("CachedRef::from on dead entity yields invalid ref", "[ecs][CachedRef]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	world.destroy_entity(eid);

	auto ref = CachedRef<VA>::from(world, eid);
	CHECK_FALSE(ref.is_valid(world));
	CHECK(ref.get(world) == nullptr);
}

TEST_CASE("CachedRef::get returns same pointer when no structural change", "[ecs][CachedRef]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	auto ref = CachedRef<VA>::from(world, eid);

	VA* p1 = ref.get(world);
	VA* p2 = ref.get(world);
	CHECK(p1 != nullptr);
	CHECK(p1 == p2);
}

TEST_CASE("CachedRef::get re-resolves after structural change in archetype", "[ecs][CachedRef]") {
	World world;
	EntityID const a = world.create_entity(VA { 1 });
	EntityID const b = world.create_entity(VA { 2 });

	auto ref_a = CachedRef<VA>::from(world, a);
	auto ref_b = CachedRef<VA>::from(world, b);
	(void) ref_a.get(world);
	(void) ref_b.get(world);

	// Destroying `a` swap-pops it. `b` (last row) gets relocated into a's slot.
	world.destroy_entity(a);

	// ref_a should now resolve to nullptr.
	CHECK_FALSE(ref_a.is_valid(world));
	CHECK(ref_a.get(world) == nullptr);

	// ref_b is still alive but the underlying pointer changed — get() must re-resolve.
	VA* fresh_b = ref_b.get(world);
	CHECK(fresh_b != nullptr);
	CHECK(fresh_b->v == 2);
}

TEST_CASE("CachedRef::is_valid reflects entity liveness", "[ecs][CachedRef]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	auto ref = CachedRef<VA>::from(world, eid);
	CHECK(ref.is_valid(world));

	world.destroy_entity(eid);
	CHECK_FALSE(ref.is_valid(world));
}

TEST_CASE("CachedRef::invalidate resets cache, get re-resolves", "[ecs][CachedRef]") {
	World world;
	EntityID const eid = world.create_entity(VA { 5 });
	auto ref = CachedRef<VA>::from(world, eid);
	CHECK(ref.get(world) != nullptr);

	ref.invalidate();
	CHECK(ref.cached_pointer == nullptr);
	CHECK(ref.cached_version == 0u);

	VA* p = ref.get(world);
	CHECK(p != nullptr);
	CHECK(p->v == 5);
}

TEST_CASE("CachedRef survives entity migration via add_component", "[ecs][CachedRef][migration]") {
	World world;
	EntityID const eid = world.create_entity(VA { 9 });
	auto ref = CachedRef<VA>::from(world, eid);
	CHECK(ref.get(world)->v == 9);

	// Migrate the entity — the VA pointer changes (new column in new archetype).
	world.add_component<VB>(eid, VB { 0 });

	VA* fresh = ref.get(world);
	CHECK(fresh != nullptr);
	CHECK(fresh->v == 9);
}

TEST_CASE("CachedRef::entity returns stored EntityID", "[ecs][CachedRef]") {
	World world;
	EntityID const eid = world.create_entity(VA { 1 });
	auto ref = CachedRef<VA>::from(world, eid);
	CHECK(ref.entity() == eid);
}

TEST_CASE("Default-constructed CachedRef is invalid", "[ecs][CachedRef]") {
	World world;
	CachedRef<VA> ref;
	CHECK(ref.entity() == INVALID_ENTITY_ID);
	CHECK_FALSE(ref.is_valid(world));
	CHECK(ref.get(world) == nullptr);
}
