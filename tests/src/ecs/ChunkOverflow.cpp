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
	// Heavy component to force a small chunk capacity and produce multiple chunks for
	// a manageable number of entities. With sizeof(Heavy) ~512, chunk_capacity is roughly
	// 16384/(8+512) ~ 31 rows per chunk.
	struct Heavy {
		std::uint64_t pad[64] {};
		int marker = 0;
	};
}

ECS_COMPONENT(Heavy, "test_ChunkOverflow::Heavy")

namespace {
	// Probe chunk_capacity by inserting until a second chunk starts. Returns the row
	// count of the first chunk (== chunk_capacity).
	std::size_t probe_chunk_capacity() {
		World world;
		std::size_t cap = 0;
		for (std::size_t i = 0; i < 4000; ++i) {
			world.create_entity(Heavy {});
			cap = 0;
			std::size_t chunk_idx = 0;
			std::size_t first = 0;
			world.for_each_chunk<Heavy>([&](ChunkView<Heavy> view) {
				if (chunk_idx == 0) {
					first = view.count();
				}
				++chunk_idx;
				cap = chunk_idx;
			});
			if (cap > 1) {
				return first;
			}
		}
		return 0;
	}
}

TEST_CASE("Inserting chunk_capacity+1 entities produces two chunks", "[ecs][ChunkOverflow]") {
	std::size_t const cap = probe_chunk_capacity();
	REQUIRE(cap > 0u);

	World world;
	for (std::size_t i = 0; i < cap + 1; ++i) {
		world.create_entity(Heavy { .marker = static_cast<int>(i) });
	}

	std::vector<std::size_t> chunk_counts;
	world.for_each_chunk<Heavy>([&](ChunkView<Heavy> view) {
		chunk_counts.push_back(view.count());
	});
	REQUIRE(chunk_counts.size() == 2u);
	CHECK(chunk_counts[0] == cap);
	CHECK(chunk_counts[1] == 1u);
}

TEST_CASE("Inserting many entities packs across chunks; for_each visits all", "[ecs][ChunkOverflow]") {
	std::size_t const cap = probe_chunk_capacity();
	REQUIRE(cap > 0u);

	World world;
	std::size_t const total = cap * 3 + 5;
	for (std::size_t i = 0; i < total; ++i) {
		world.create_entity(Heavy { .marker = static_cast<int>(i) });
	}

	int count = 0;
	std::int64_t sum_markers = 0;
	world.for_each<Heavy>([&](Heavy& h) {
		++count;
		sum_markers += h.marker;
	});
	CHECK(count == static_cast<int>(total));
	std::int64_t expected = static_cast<std::int64_t>(total) * (static_cast<std::int64_t>(total) - 1) / 2;
	CHECK(sum_markers == expected);
}

TEST_CASE("destroy_entity in the first chunk relocates last entity from last chunk", "[ecs][ChunkOverflow]") {
	std::size_t const cap = probe_chunk_capacity();
	REQUIRE(cap > 0u);

	World world;
	std::size_t const total = cap + 5;
	std::vector<EntityID> ids;
	for (std::size_t i = 0; i < total; ++i) {
		ids.push_back(world.create_entity(Heavy { .marker = static_cast<int>(i) }));
	}

	// Destroy entity at index 0 (first chunk). The very last entity should get relocated
	// into its slot via cross-chunk swap-pop.
	EntityID first = ids[0];
	EntityID last = ids[total - 1];
	world.destroy_entity(first);

	CHECK_FALSE(world.is_alive(first));
	CHECK(world.is_alive(last));
	CHECK(world.get_component<Heavy>(last)->marker == static_cast<int>(total - 1));

	// All other entities still alive and have their original markers.
	for (std::size_t i = 1; i < total - 1; ++i) {
		CHECK(world.is_alive(ids[i]));
		CHECK(world.get_component<Heavy>(ids[i])->marker == static_cast<int>(i));
	}

	// After the destroy, only `total - 1` entities remain.
	int count = 0;
	world.for_each<Heavy>([&](Heavy&) { ++count; });
	CHECK(count == static_cast<int>(total - 1));
}

TEST_CASE("Destroying enough entities drops the trailing empty chunk", "[ecs][ChunkOverflow]") {
	std::size_t const cap = probe_chunk_capacity();
	REQUIRE(cap > 0u);

	World world;
	std::size_t const total = cap + 1;
	std::vector<EntityID> ids;
	for (std::size_t i = 0; i < total; ++i) {
		ids.push_back(world.create_entity(Heavy { .marker = static_cast<int>(i) }));
	}

	// Destroy the lone entity in the second chunk. The trailing chunk should be dropped.
	world.destroy_entity(ids.back());

	std::size_t chunk_count = 0;
	world.for_each_chunk<Heavy>([&](ChunkView<Heavy>) { ++chunk_count; });
	CHECK(chunk_count == 1u);
}
