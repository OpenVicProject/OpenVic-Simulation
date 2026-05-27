#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>
#include <set>
#include <type_traits>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct Tag1 {};
	struct Tag2 {};
	struct Payload {
		int v = 0;
	};

	static_assert(std::is_empty_v<Tag1>);
	static_assert(std::is_empty_v<Tag2>);
}

ECS_COMPONENT(Tag1, "test_Tag::Tag1")
ECS_COMPONENT(Tag2, "test_Tag::Tag2")
ECS_COMPONENT(Payload, "test_Tag::Payload")

TEST_CASE("create_entity with only a tag works", "[ecs][World][tag]") {
	World world;
	EntityID const eid = world.create_entity(Tag1 {});
	CHECK(world.is_alive(eid));
	CHECK(world.has_component<Tag1>(eid));
}

TEST_CASE("get_component<Tag> returns nullptr (no data slot)", "[ecs][World][tag]") {
	World world;
	EntityID const eid = world.create_entity(Tag1 {});
	CHECK(world.get_component<Tag1>(eid) == nullptr);
}

TEST_CASE("has_component<Tag> reflects archetype membership", "[ecs][World][tag]") {
	World world;
	EntityID const a = world.create_entity(Tag1 {});
	EntityID const b = world.create_entity(Payload { 0 });
	CHECK(world.has_component<Tag1>(a));
	CHECK_FALSE(world.has_component<Tag1>(b));
}

TEST_CASE("for_each over a tag-only archetype iterates all rows", "[ecs][World][tag][iter]") {
	World world;
	world.create_entity(Tag1 {});
	world.create_entity(Tag1 {});
	world.create_entity(Tag1 {});

	int count = 0;
	world.for_each<Tag1>([&](Tag1&) { ++count; });
	CHECK(count == 3);
}

TEST_CASE("for_each over Tag + Payload visits matching entities", "[ecs][World][tag][iter]") {
	World world;
	world.create_entity(Payload { 1 });
	world.create_entity(Payload { 2 }, Tag1 {});
	world.create_entity(Payload { 3 }, Tag1 {});
	world.create_entity(Payload { 4 }, Tag2 {});

	int sum = 0;
	int count = 0;
	world.for_each<Payload, Tag1>([&](Payload& p, Tag1&) {
		sum += p.v;
		++count;
	});
	CHECK(count == 2);
	CHECK(sum == 5);
}

TEST_CASE("destroy_entity in a tag archetype works", "[ecs][World][tag]") {
	World world;
	EntityID const a = world.create_entity(Tag1 {});
	EntityID const b = world.create_entity(Tag1 {});
	EntityID const c = world.create_entity(Tag1 {});

	world.destroy_entity(b);
	CHECK_FALSE(world.is_alive(b));
	CHECK(world.is_alive(a));
	CHECK(world.is_alive(c));

	int count = 0;
	world.for_each<Tag1>([&](Tag1&) { ++count; });
	CHECK(count == 2);
}

TEST_CASE("add_component<Tag> migrates entity to tag-extended archetype", "[ecs][World][tag][migration]") {
	World world;
	EntityID const eid = world.create_entity(Payload { 5 });
	CHECK_FALSE(world.has_component<Tag1>(eid));

	world.add_component<Tag1>(eid);
	CHECK(world.has_component<Tag1>(eid));
	CHECK(world.has_component<Payload>(eid));
	CHECK(world.get_component<Payload>(eid)->v == 5);
}

TEST_CASE("remove_component<Tag> migrates back", "[ecs][World][tag][migration]") {
	World world;
	EntityID const eid = world.create_entity(Payload { 7 }, Tag1 {});
	world.remove_component<Tag1>(eid);
	CHECK_FALSE(world.has_component<Tag1>(eid));
	CHECK(world.has_component<Payload>(eid));
	CHECK(world.get_component<Payload>(eid)->v == 7);
}

TEST_CASE("Tag component column version still bumps on push/pop", "[ecs][World][tag][version]") {
	World world;
	EntityID const a = world.create_entity(Tag1 {});
	uint64_t v0 = world.component_version_in<Tag1>(a);
	CHECK(v0 > 0u);

	world.create_entity(Tag1 {});
	uint64_t v1 = world.component_version_in<Tag1>(a);
	CHECK(v1 > v0);
}

TEST_CASE("for_each_with_entity over tag passes correct EntityIDs", "[ecs][World][tag][iter]") {
	World world;
	EntityID const a = world.create_entity(Tag1 {});
	EntityID const b = world.create_entity(Tag1 {});

	std::set<uint64_t> seen;
	world.for_each_with_entity<Tag1>([&](EntityID e, Tag1&) { seen.insert(e.to_uint64()); });
	CHECK(seen.size() == 2u);
	CHECK(seen.count(a.to_uint64()) == 1u);
	CHECK(seen.count(b.to_uint64()) == 1u);
}

TEST_CASE("Two tag components on the same entity coexist", "[ecs][World][tag]") {
	World world;
	EntityID const eid = world.create_entity(Tag1 {}, Tag2 {});
	CHECK(world.has_component<Tag1>(eid));
	CHECK(world.has_component<Tag2>(eid));
	CHECK(world.get_component<Tag1>(eid) == nullptr);
	CHECK(world.get_component<Tag2>(eid) == nullptr);
}
