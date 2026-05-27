#include "openvic-simulation/ecs/Chunk.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	// Heavy components that produce small chunk capacity for both src and target archetypes.
	struct CMigrA {
		std::uint64_t pad[60] {};
		int marker = 0;
		// Fills the tail-padding gap after `marker` so the type is uniquely representable
		// (checksum byte path).
		std::int32_t _pad = 0;
	};
	struct CMigrB {
		std::uint64_t pad[60] {};
		int marker = 0;
		std::int32_t _pad = 0;
	};
}

ECS_COMPONENT(CMigrA, "test_ChunkMigration::CMigrA")
ECS_COMPONENT(CMigrB, "test_ChunkMigration::CMigrB")

namespace {
	std::size_t probe_capacity_for_a_only() {
		World world;
		for (std::size_t i = 0; i < 4000; ++i) {
			world.create_entity(CMigrA {});
			std::size_t chunks = 0;
			std::size_t first = 0;
			world.for_each_chunk<CMigrA>([&](ChunkView<CMigrA> view) {
				if (chunks == 0) {
					first = view.count();
				}
				++chunks;
			});
			if (chunks > 1) {
				return first;
			}
		}
		return 0;
	}
}

TEST_CASE("add_component on an entity in a multi-chunk archetype migrates correctly", "[ecs][ChunkMigration]") {
	std::size_t const cap = probe_capacity_for_a_only();
	REQUIRE(cap > 0u);

	World world;
	std::size_t const total = cap * 2 + 3;
	std::vector<EntityID> ids;
	for (std::size_t i = 0; i < total; ++i) {
		ids.push_back(world.create_entity(CMigrA { .marker = static_cast<int>(i) }));
	}

	// Migrate a middle entity (in the second chunk) to {CMigrA, CMigrB}.
	EntityID const target = ids[cap + 1];
	world.add_component<CMigrB>(target, CMigrB { .marker = 999 });

	CHECK(world.has_component<CMigrA>(target));
	CHECK(world.has_component<CMigrB>(target));
	CHECK(world.get_component<CMigrA>(target)->marker == static_cast<int>(cap + 1));
	CHECK(world.get_component<CMigrB>(target)->marker == 999);

	// Source archetype still has total - 1 entities; target archetype has 1.
	int a_count = 0;
	world.for_each<CMigrA>([&](CMigrA&) { ++a_count; });
	CHECK(a_count == static_cast<int>(total));  // both archetypes carry CMigrA

	int b_count = 0;
	world.for_each<CMigrA, CMigrB>([&](CMigrA&, CMigrB&) { ++b_count; });
	CHECK(b_count == 1);
}

TEST_CASE("Migrating many entities into the same target archetype overflows its chunk", "[ecs][ChunkMigration]") {
	std::size_t const cap = probe_capacity_for_a_only();
	REQUIRE(cap > 0u);

	World world;
	std::vector<EntityID> ids;
	std::size_t const total = cap + 10;
	for (std::size_t i = 0; i < total; ++i) {
		ids.push_back(world.create_entity(CMigrA { .marker = static_cast<int>(i) }));
	}

	// Migrate every entity to {CMigrA, CMigrB}. Target archetype's chunks should now hold them.
	for (EntityID id : ids) {
		world.add_component<CMigrB>(id, CMigrB { .marker = 0 });
	}

	std::size_t chunk_count = 0;
	std::size_t total_rows = 0;
	world.for_each_chunk<CMigrA, CMigrB>([&](ChunkView<CMigrA, CMigrB> view) {
		++chunk_count;
		total_rows += view.count();
	});
	CHECK(total_rows == total);
	CHECK(chunk_count >= 2u);

	// Source archetype {CMigrA} should be empty now.
	int src_count = 0;
	world.for_each<CMigrA>([&](CMigrA&) { ++src_count; });
	CHECK(src_count == static_cast<int>(total));
}
