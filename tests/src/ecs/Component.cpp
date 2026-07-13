#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <string>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct Health {
		int hp = 0;
		int max_hp = 0;
	};
	struct Name {
		std::string value;
	};
	// Checksum enforcement: heap-holding type — walk size then bytes in index order.
	inline uint64_t ecs_checksum(Name const& name, uint64_t seed) {
		uint64_t h = fold_uint64(name.value.size(), seed);
		return fnv1a_64_bytes(name.value.data(), name.value.size(), h);
	}
	struct Velocity {
		float dx = 0;
		float dy = 0;
	};
	// Two floats, no padding — author-asserted byte hash for the checksum enforcement.
	ECS_CHECKSUM_BYTES(Velocity)
}

ECS_COMPONENT(Health, "test_Component::Health")
ECS_COMPONENT(Name, "test_Component::Name")
ECS_COMPONENT(Velocity, "test_Component::Velocity")

TEST_CASE("get_component returns valid pointer for existing component", "[ecs][World][component]") {
	World world;
	EntityID const eid = world.create_entity(Health { 50, 100 });
	Health* h = world.get_component<Health>(eid);
	CHECK(h != nullptr);
	CHECK(h->hp == 50);
	CHECK(h->max_hp == 100);
}

TEST_CASE("get_component (const) returns valid pointer", "[ecs][World][component]") {
	World world;
	EntityID const eid = world.create_entity(Health { 25, 50 });
	World const& cw = world;
	Health const* h = cw.get_component<Health>(eid);
	CHECK(h != nullptr);
	CHECK(h->hp == 25);
}

TEST_CASE("get_component returns nullptr for dead entity", "[ecs][World][component]") {
	World world;
	EntityID const eid = world.create_entity(Health { 1, 1 });
	world.destroy_entity(eid);
	CHECK(world.get_component<Health>(eid) == nullptr);
}

TEST_CASE("get_component returns nullptr for invalid EntityID", "[ecs][World][component]") {
	World world;
	CHECK(world.get_component<Health>(INVALID_ENTITY_ID) == nullptr);
	CHECK(world.get_component<Health>(EntityID { 999, 1 }) == nullptr);
}

TEST_CASE("get_component returns nullptr for component not in archetype", "[ecs][World][component]") {
	World world;
	EntityID const eid = world.create_entity(Health { 10, 20 });
	CHECK(world.get_component<Velocity>(eid) == nullptr);
	CHECK(world.get_component<Name>(eid) == nullptr);
}

TEST_CASE("has_component reflects archetype membership", "[ecs][World][component]") {
	World world;
	EntityID const eid = world.create_entity(Health { 5, 10 }, Velocity { 1.0f, 0.0f });
	CHECK(world.has_component<Health>(eid));
	CHECK(world.has_component<Velocity>(eid));
	CHECK_FALSE(world.has_component<Name>(eid));
}

TEST_CASE("has_component returns false for dead entity", "[ecs][World][component]") {
	World world;
	EntityID const eid = world.create_entity(Health { 5, 10 });
	world.destroy_entity(eid);
	CHECK_FALSE(world.has_component<Health>(eid));
}

TEST_CASE("has_component returns false for invalid EntityID", "[ecs][World][component]") {
	World world;
	CHECK_FALSE(world.has_component<Health>(INVALID_ENTITY_ID));
}

TEST_CASE("Component values are preserved across other entity operations", "[ecs][World][component]") {
	World world;
	EntityID const a = world.create_entity(Health { 10, 100 });
	EntityID const b = world.create_entity(Health { 20, 200 });
	EntityID const c = world.create_entity(Health { 30, 300 });

	// Mutate c's state via pointer.
	world.get_component<Health>(c)->hp = 999;

	// Read everything back.
	CHECK(world.get_component<Health>(a)->hp == 10);
	CHECK(world.get_component<Health>(b)->hp == 20);
	CHECK(world.get_component<Health>(c)->hp == 999);
}

TEST_CASE("Components with non-trivial dtor are destroyed properly", "[ecs][World][component]") {
	World world;
	EntityID const eid = world.create_entity(Name { "Alice" });
	Name* n = world.get_component<Name>(eid);
	CHECK(n != nullptr);
	CHECK(n->value == "Alice");

	// Destroy the entity — Name's std::string dtor should run, no leak.
	world.destroy_entity(eid);
	CHECK_FALSE(world.is_alive(eid));
}

TEST_CASE("Different archetypes coexist in the same World", "[ecs][World][component]") {
	World world;
	EntityID const a = world.create_entity(Health { 1, 1 });
	EntityID const b = world.create_entity(Velocity { 0, 0 });
	EntityID const c = world.create_entity(Health { 2, 2 }, Velocity { 1, 1 });

	CHECK(world.has_component<Health>(a));
	CHECK_FALSE(world.has_component<Velocity>(a));
	CHECK(world.has_component<Velocity>(b));
	CHECK_FALSE(world.has_component<Health>(b));
	CHECK(world.has_component<Health>(c));
	CHECK(world.has_component<Velocity>(c));
}
