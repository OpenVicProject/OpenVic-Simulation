#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"

#include <cstdint>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct CompA {
		int x;
	};
	struct CompB {
		float y;
	};
	struct CompC {
		double z;
	};
}

ECS_COMPONENT(CompA, "test_EntityID::CompA")
ECS_COMPONENT(CompB, "test_EntityID::CompB")
ECS_COMPONENT(CompC, "test_EntityID::CompC")

TEST_CASE("EntityID default-constructed is invalid", "[ecs][EntityID]") {
	EntityID const eid;
	CHECK(eid.index == 0u);
	CHECK(eid.generation == 0u);
	CHECK_FALSE(eid.is_valid());
}

TEST_CASE("EntityID INVALID_ENTITY_ID matches default-constructed", "[ecs][EntityID]") {
	CHECK(INVALID_ENTITY_ID == EntityID {});
	CHECK_FALSE(INVALID_ENTITY_ID.is_valid());
}

TEST_CASE("EntityID equality and inequality", "[ecs][EntityID]") {
	EntityID const a { 1, 1 };
	EntityID const b { 1, 1 };
	EntityID const c { 1, 2 };
	EntityID const d { 2, 1 };

	CHECK(a == b);
	CHECK_FALSE(a != b);
	CHECK_FALSE(a == c);
	CHECK(a != c);
	CHECK_FALSE(a == d);
	CHECK(a != d);
}

TEST_CASE("EntityID is_valid only when generation > 0", "[ecs][EntityID]") {
	CHECK_FALSE(EntityID { 0, 0 }.is_valid());
	CHECK_FALSE(EntityID { 5, 0 }.is_valid());
	CHECK(EntityID { 0, 1 }.is_valid());
	CHECK(EntityID { 5, 1 }.is_valid());
	CHECK(EntityID { 0xFFFFFFFFu, 0xFFFFFFFFu }.is_valid());
}

TEST_CASE("EntityID to_uint64 packs generation in high bits", "[ecs][EntityID]") {
	EntityID const eid { 0x12345678u, 0xABCDEF01u };
	uint64_t const encoded = eid.to_uint64();
	CHECK(encoded == 0xABCDEF0112345678ULL);
}

TEST_CASE("EntityID from_uint64 round-trips to_uint64", "[ecs][EntityID]") {
	EntityID const original { 42, 7 };
	EntityID const decoded = EntityID::from_uint64(original.to_uint64());
	CHECK(decoded == original);
	CHECK(decoded.index == 42u);
	CHECK(decoded.generation == 7u);
}

TEST_CASE("EntityID from_uint64(0) yields invalid", "[ecs][EntityID]") {
	EntityID const eid = EntityID::from_uint64(0);
	CHECK(eid == INVALID_ENTITY_ID);
	CHECK_FALSE(eid.is_valid());
}

TEST_CASE("EntityID round-trips edge values", "[ecs][EntityID]") {
	uint64_t const max_index_only = 0x00000000FFFFFFFFULL;
	uint64_t const max_gen_only = 0xFFFFFFFF00000000ULL;
	uint64_t const all_ones = 0xFFFFFFFFFFFFFFFFULL;

	EntityID a = EntityID::from_uint64(max_index_only);
	CHECK(a.index == 0xFFFFFFFFu);
	CHECK(a.generation == 0u);
	CHECK_FALSE(a.is_valid());
	CHECK(a.to_uint64() == max_index_only);

	EntityID b = EntityID::from_uint64(max_gen_only);
	CHECK(b.index == 0u);
	CHECK(b.generation == 0xFFFFFFFFu);
	CHECK(b.is_valid());
	CHECK(b.to_uint64() == max_gen_only);

	EntityID c = EntityID::from_uint64(all_ones);
	CHECK(c.index == 0xFFFFFFFFu);
	CHECK(c.generation == 0xFFFFFFFFu);
	CHECK(c.is_valid());
	CHECK(c.to_uint64() == all_ones);
}

TEST_CASE("component_type_id_of returns same value across calls", "[ecs][ComponentTypeID]") {
	component_type_id_t const a1 = component_type_id_of<CompA>();
	component_type_id_t const a2 = component_type_id_of<CompA>();
	CHECK(a1 == a2);
}

TEST_CASE("component_type_id_of returns distinct values per type", "[ecs][ComponentTypeID]") {
	component_type_id_t const a = component_type_id_of<CompA>();
	component_type_id_t const b = component_type_id_of<CompB>();
	component_type_id_t const c = component_type_id_of<CompC>();

	CHECK(a != b);
	CHECK(a != c);
	CHECK(b != c);
}

TEST_CASE("component_type_id_of strips cv/ref qualifiers via caller", "[ecs][ComponentTypeID]") {
	// component_type_id_of<C> is keyed on the exact type; remove_cvref is the caller's
	// responsibility (World::create_entity / get_component do this internally). Here we
	// just confirm the un-qualified type maps to a stable id.
	component_type_id_t const a1 = component_type_id_of<CompA>();
	component_type_id_t const a2 = component_type_id_of<CompA>();
	CHECK(a1 == a2);
}
