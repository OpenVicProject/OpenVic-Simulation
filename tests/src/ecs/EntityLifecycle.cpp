#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct LifeA {
		int v = 0;
	};
	struct LifeB {
		int w = 0;
	};
}

ECS_COMPONENT(LifeA, "test_EntityLifecycle::LifeA")
ECS_COMPONENT(LifeB, "test_EntityLifecycle::LifeB")

TEST_CASE("World::create_entity returns valid EntityID", "[ecs][World][lifecycle]") {
	World world;
	EntityID const eid = world.create_entity(LifeA { 1 });
	CHECK(eid.is_valid());
	CHECK(world.is_alive(eid));
}

TEST_CASE("World::is_alive on never-allocated index returns false", "[ecs][World][lifecycle]") {
	World world;
	CHECK_FALSE(world.is_alive(EntityID { 0, 1 }));
	CHECK_FALSE(world.is_alive(EntityID { 999, 1 }));
	CHECK_FALSE(world.is_alive(INVALID_ENTITY_ID));
}

TEST_CASE("World::destroy_entity makes entity not alive", "[ecs][World][lifecycle]") {
	World world;
	EntityID const eid = world.create_entity(LifeA { 1 });
	CHECK(world.is_alive(eid));
	world.destroy_entity(eid);
	CHECK_FALSE(world.is_alive(eid));
}

TEST_CASE("World::destroy_entity is no-op for invalid / dead IDs", "[ecs][World][lifecycle]") {
	World world;
	EntityID const eid = world.create_entity(LifeA { 1 });
	world.destroy_entity(eid);
	world.destroy_entity(eid); // double-destroy: should be safe
	world.destroy_entity(INVALID_ENTITY_ID);
	world.destroy_entity(EntityID { 999, 1 });
	CHECK_FALSE(world.is_alive(eid));
}

TEST_CASE("World::create_entity reuses freed slot with bumped generation", "[ecs][World][lifecycle]") {
	World world;
	EntityID const a = world.create_entity(LifeA { 1 });
	world.destroy_entity(a);

	EntityID const b = world.create_entity(LifeA { 2 });
	CHECK(b.index == a.index);
	CHECK(b.generation > a.generation);

	// Stale handle to the slot is rejected.
	CHECK_FALSE(world.is_alive(a));
	CHECK(world.is_alive(b));
}

TEST_CASE("World allocates fresh slots when no free-list", "[ecs][World][lifecycle]") {
	World world;
	EntityID const a = world.create_entity(LifeA { 1 });
	EntityID const b = world.create_entity(LifeA { 2 });
	EntityID const c = world.create_entity(LifeA { 3 });

	CHECK(a.index != b.index);
	CHECK(a.index != c.index);
	CHECK(b.index != c.index);
	CHECK(world.is_alive(a));
	CHECK(world.is_alive(b));
	CHECK(world.is_alive(c));
}

TEST_CASE("Free-list LIFO ordering: most-recently-freed slot is reused first", "[ecs][World][lifecycle]") {
	World world;
	EntityID const a = world.create_entity(LifeA { 1 });
	EntityID const b = world.create_entity(LifeA { 2 });
	EntityID const c = world.create_entity(LifeA { 3 });

	world.destroy_entity(a);
	world.destroy_entity(b);
	world.destroy_entity(c);

	EntityID const x = world.create_entity(LifeA { 4 });
	CHECK(x.index == c.index);

	EntityID const y = world.create_entity(LifeA { 5 });
	CHECK(y.index == b.index);

	EntityID const z = world.create_entity(LifeA { 6 });
	CHECK(z.index == a.index);
}

TEST_CASE("destroy_entity with swap-pop relocates last entity correctly", "[ecs][World][lifecycle]") {
	World world;
	EntityID const a = world.create_entity(LifeA { 100 });
	EntityID const b = world.create_entity(LifeA { 200 });
	EntityID const c = world.create_entity(LifeA { 300 });

	// Destroy `a` (the first row in its archetype). `c` (the last row) should be relocated.
	world.destroy_entity(a);

	CHECK_FALSE(world.is_alive(a));
	CHECK(world.is_alive(b));
	CHECK(world.is_alive(c));

	LifeA* lb = world.get_component<LifeA>(b);
	LifeA* lc = world.get_component<LifeA>(c);
	CHECK(lb != nullptr);
	CHECK(lc != nullptr);
	CHECK(lb->v == 200);
	CHECK(lc->v == 300);
}

TEST_CASE("Stale EntityID after slot reuse fails is_alive even if other entities exist", "[ecs][World][lifecycle]") {
	World world;
	EntityID const a = world.create_entity(LifeA { 1 });
	world.destroy_entity(a);
	EntityID const b = world.create_entity(LifeA { 2 });
	CHECK(b.index == a.index);

	CHECK_FALSE(world.is_alive(a));
	CHECK(world.is_alive(b));

	// Even after creating more entities, the original ID should remain invalid.
	world.create_entity(LifeA { 3 });
	world.create_entity(LifeA { 4 });
	CHECK_FALSE(world.is_alive(a));
}

TEST_CASE("create_entity with multiple components produces a single archetype", "[ecs][World][lifecycle]") {
	World world;
	EntityID const a = world.create_entity(LifeA { 11 }, LifeB { 22 });
	EntityID const b = world.create_entity(LifeB { 44 }, LifeA { 33 }); // ordering shouldn't matter

	CHECK(world.is_alive(a));
	CHECK(world.is_alive(b));
	CHECK(world.has_component<LifeA>(a));
	CHECK(world.has_component<LifeB>(a));
	CHECK(world.has_component<LifeA>(b));
	CHECK(world.has_component<LifeB>(b));

	CHECK(world.get_component<LifeA>(a)->v == 11);
	CHECK(world.get_component<LifeB>(a)->w == 22);
	CHECK(world.get_component<LifeA>(b)->v == 33);
	CHECK(world.get_component<LifeB>(b)->w == 44);
}

TEST_CASE("Destroying many entities does not leak (smoke)", "[ecs][World][lifecycle]") {
	World world;
	std::vector<EntityID> ids;
	for (int i = 0; i < 200; ++i) {
		ids.push_back(world.create_entity(LifeA { i }));
	}
	for (EntityID id : ids) {
		world.destroy_entity(id);
	}
	for (EntityID id : ids) {
		CHECK_FALSE(world.is_alive(id));
	}
	// Re-create — should reuse slots.
	for (int i = 0; i < 200; ++i) {
		EntityID const e = world.create_entity(LifeA { i + 1000 });
		CHECK(world.is_alive(e));
	}
}
