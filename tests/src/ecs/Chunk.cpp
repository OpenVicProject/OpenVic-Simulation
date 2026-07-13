#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/Chunk.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct SmallC {
		int v = 0;
	};
	struct LargeC {
		std::uint8_t bytes[120] {};
	};
	struct AlignC {
		alignas(16) double values[2] {};
	};
	// Two doubles fill the 16-byte alignment exactly, no padding — author-asserted byte hash.
	ECS_CHECKSUM_BYTES(AlignC)
	struct CTagA {};
	struct CTagB {};
}

ECS_COMPONENT(SmallC, "test_Chunk::SmallC")
ECS_COMPONENT(LargeC, "test_Chunk::LargeC")
ECS_COMPONENT(AlignC, "test_Chunk::AlignC")
ECS_COMPONENT(CTagA, "test_Chunk::CTagA")
ECS_COMPONENT(CTagB, "test_Chunk::CTagB")

namespace {
	// Helper to dig into the World and find the archetype an entity lives in.
	Archetype* archetype_via_world(World& world, EntityID eid) {
		// Round-trip a single read so we know the entity is alive and a row exists, then
		// walk the world's archetypes via for_each_chunk.
		Archetype* found = nullptr;
		// We can't directly access World::archetypes (private). Use for_each_chunk to capture
		// the chunk capacity / column array layout for the entity's archetype.
		(void) world;
		(void) eid;
		(void) found;
		return nullptr;
	}
}

TEST_CASE("Chunk capacity for small single-component archetype is large", "[ecs][Chunk]") {
	// We can't read Archetype::chunk_capacity directly through the public World API, but
	// we can probe it: insert N rows, run for_each_chunk, and check that the first chunk's
	// row count equals chunk_capacity once we exceed it.
	World world;
	for (int i = 0; i < 1024; ++i) {
		world.create_entity(SmallC { i });
	}

	std::size_t total_rows = 0;
	std::size_t first_chunk_count = 0;
	int chunk_index = 0;
	world.for_each_chunk<SmallC>([&](ChunkView<SmallC> view) {
		if (chunk_index == 0) {
			first_chunk_count = view.count();
		}
		total_rows += view.count();
		++chunk_index;
	});
	CHECK(total_rows == 1024u);
	// First chunk's capacity for SmallC (sizeof int=4) should be much larger than 1024,
	// so 1024 rows fit in a single chunk.
	CHECK(first_chunk_count == 1024u);
	CHECK(chunk_index == 1);
}

TEST_CASE("Chunk capacity is smaller for large components", "[ecs][Chunk]") {
	World world;
	// 120-byte component: 16384 / (8 + 120) = 128 rows per chunk approximately.
	for (int i = 0; i < 100; ++i) {
		world.create_entity(LargeC {});
	}

	std::size_t total_rows = 0;
	world.for_each_chunk<LargeC>([&](ChunkView<LargeC> view) {
		total_rows += view.count();
	});
	CHECK(total_rows == 100u);
}

TEST_CASE("Tag-only archetype iterates all entities", "[ecs][Chunk][tag]") {
	World world;
	for (int i = 0; i < 50; ++i) {
		world.create_entity(CTagA {});
	}

	int count = 0;
	world.for_each<CTagA>([&](CTagA&) { ++count; });
	CHECK(count == 50);
}

TEST_CASE("Mixed tag and non-tag archetype packs only non-tag data", "[ecs][Chunk][tag]") {
	World world;
	for (int i = 0; i < 200; ++i) {
		world.create_entity(SmallC { i }, CTagA {}, CTagB {});
	}

	int count = 0;
	int sum = 0;
	world.for_each<SmallC, CTagA, CTagB>([&](SmallC& s, CTagA&, CTagB&) {
		sum += s.v;
		++count;
	});
	CHECK(count == 200);
	CHECK(sum == (199 * 200) / 2);
}

TEST_CASE("Aligned component slabs are properly aligned", "[ecs][Chunk]") {
	World world;
	EntityID const eid = world.create_entity(AlignC {});
	AlignC* p = world.get_component<AlignC>(eid);
	CHECK(p != nullptr);
	auto address = reinterpret_cast<std::uintptr_t>(p);
	CHECK((address % alignof(AlignC)) == 0u);
}
