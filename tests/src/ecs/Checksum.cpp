#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/Checksum.hpp"
#include "openvic-simulation/ecs/ChecksumTraits.hpp"
#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === Full-state checksum (ecs/Checksum.hpp) ===
// The measuring instrument behind the determinism gates: equal checksums <=> equal live
// ECS state (entities + singletons) under the canonical memory-order walk.

namespace {
	struct CkValue {
		int64_t v = 0;
	};
	struct CkPair {
		int32_t a = 0;
		int32_t b = 0;
	};
	struct CkTag {};
	// Component unique to the empty-archetype test — its archetype exists only in the world
	// that created (then destroyed) it.
	struct CkUnique {
		int64_t u = 0;
	};
	// Uniquely representable AND carrying a custom hash that deliberately ignores `ignored` —
	// proves the custom path takes precedence over the byte path.
	struct CkCustom {
		int64_t counted = 0;
		int64_t ignored = 0;
	};
	inline uint64_t ecs_checksum(CkCustom const& c, uint64_t seed) {
		return fold_uint64(static_cast<uint64_t>(c.counted), seed);
	}
	// Heap-holding singleton: the custom hash walks size + elements in index order.
	struct CkVecSingleton {
		std::vector<int64_t> values;
	};
	inline uint64_t ecs_checksum(CkVecSingleton const& s, uint64_t seed) {
		uint64_t h = fold_uint64(s.values.size(), seed);
		for (int64_t value : s.values) {
			h = fold_uint64(static_cast<uint64_t>(value), h);
		}
		return h;
	}
	struct CkConfigA {
		int64_t v = 0;
	};
	struct CkConfigB {
		int64_t v = 0;
	};
	struct CkTagSingleton {};

	// Worker-count-invariance components (modeled on WorkerCountInvariance.cpp).
	struct CkSeed {
		int64_t seed = 0;
	};
	struct CkSpawned {
		int64_t derived = 0;
	};

	// Trait-only types for the compile-time enforcement checks — never used in a World.
	struct CkPadded {
		uint64_t a = 0;
		uint32_t b = 0; // 4 bytes tail padding -> not uniquely representable
	};
	struct CkBoolPad {
		bool flag = false; // 3 bytes interior padding before v
		int32_t v = 0;
	};
	struct CkStringNoHash {
		std::string s;
	};
}

ECS_COMPONENT(CkValue, "test_Checksum::Value")
ECS_COMPONENT(CkPair, "test_Checksum::Pair")
ECS_COMPONENT(CkTag, "test_Checksum::Tag")
ECS_COMPONENT(CkUnique, "test_Checksum::Unique")
ECS_COMPONENT(CkCustom, "test_Checksum::Custom")
ECS_COMPONENT(CkVecSingleton, "test_Checksum::VecSingleton")
ECS_COMPONENT(CkConfigA, "test_Checksum::ConfigA")
ECS_COMPONENT(CkConfigB, "test_Checksum::ConfigB")
ECS_COMPONENT(CkTagSingleton, "test_Checksum::TagSingleton")
ECS_COMPONENT(CkSeed, "test_Checksum::Seed")
ECS_COMPONENT(CkSpawned, "test_Checksum::Spawned")

// === (d) compile-time enforcement — void_t detection traits, paired positive/negative so a
// typo cannot make a negative pass vacuously (MSVC mishandles !requires{...}; see
// ImmutableEntity.cpp for the pattern rationale). ===
static_assert(is_checksummable_v<CkValue>, "plain int64 component byte-hashes");
static_assert(is_checksummable_v<CkPair>, "two int32s, no padding");
static_assert(is_checksummable_v<CkTag>, "tags are exempt (presence via signature)");
static_assert(is_checksummable_v<CkCustom>, "custom hash satisfies the rule");
static_assert(is_checksummable_v<CkVecSingleton>, "heap-holder with custom hash");
static_assert(!is_checksummable_v<CkPadded>, "tail padding without custom hash is rejected");
static_assert(!is_checksummable_v<CkBoolPad>, "interior padding without custom hash is rejected");
static_assert(!is_checksummable_v<CkStringNoHash>, "heap-holder without custom hash is rejected");
static_assert(has_custom_checksum_v<CkCustom>, "ADL detection finds ecs_checksum");
static_assert(!has_custom_checksum_v<CkValue>, "no false-positive custom detection");

namespace {
	// Threaded spawner: every CkSeed entity spawns one CkSpawned with a deterministic value.
	struct CkSpawner : SystemThreaded<CkSpawner> {
		void tick(TickContext const& ctx, EntityID, CkSeed const& s) {
			ctx.cmd.create_entity(ctx.world, CkSpawned { s.seed * 31 + 7 });
		}
	};
	// Serial churn: mutates CkValue on the seed entities every tick.
	struct CkChurn : System<CkChurn> {
		void tick(TickContext const& /*ctx*/, CkValue& v, CkSeed const& s) {
			v.v = v.v * 31 + s.seed * 7;
		}
	};
}
ECS_SYSTEM(CkSpawner)
ECS_SYSTEM(CkChurn)

namespace {
	// Shared creation script for the determinism tests: >= 2 archetypes (one with a tag)
	// plus singletons.
	void build_reference_world(World& world) {
		for (int64_t i = 0; i < 20; ++i) {
			world.create_entity(CkValue { i + 1 });
		}
		for (int64_t i = 0; i < 10; ++i) {
			world.create_entity(CkValue { 100 + i }, CkTag {});
		}
		for (int32_t i = 0; i < 5; ++i) {
			world.create_entity(CkPair { i, -i });
		}
		world.set_singleton(CkConfigA { 42 });
		world.set_singleton(CkVecSingleton { { 1, 2, 3 } });
	}
}

// === (a) determinism / read-only ===

TEST_CASE("Identical creation scripts in two Worlds produce equal checksums", "[ecs][Checksum]") {
	World a;
	World b;
	build_reference_world(a);
	build_reference_world(b);
	CHECK(world_checksum(a) == world_checksum(b));
}

TEST_CASE("Repeated checksum of an untouched World is stable and read-only", "[ecs][Checksum]") {
	World world;
	build_reference_world(world);

	uint64_t const first = world_checksum(world);
	CHECK(world_checksum(world) == first);

	// A query in between (touches the lazily-built query cache) must not move the checksum.
	int64_t sum = 0;
	world.for_each_with_entity<CkValue>([&](EntityID, CkValue& v) {
		sum += v.v;
	});
	CHECK(sum != 0);
	CHECK(world_checksum(world) == first);
}

TEST_CASE("Breakdown total matches world_checksum and refolds from its entries", "[ecs][Checksum]") {
	World world;
	build_reference_world(world);

	WorldChecksumBreakdown const breakdown = world_checksum_breakdown(world);
	CHECK(breakdown.total == world_checksum(world));
	CHECK(fold_checksum_breakdown(breakdown) == breakdown.total);
	// 3 non-empty archetypes (CkValue / CkValue+CkTag / CkPair), 2 singletons.
	CHECK(breakdown.archetype_entries.size() == 3u);
	CHECK(breakdown.singleton_entries.size() == 2u);
}

// === (b) sensitivity ===

TEST_CASE("Flipping a single component field changes the checksum", "[ecs][Checksum]") {
	World world;
	build_reference_world(world);
	EntityID const extra = world.create_entity(CkValue { 7 });

	uint64_t const before = world_checksum(world);
	world.get_component<CkValue>(extra)->v += 1;
	CHECK(world_checksum(world) != before);
}

TEST_CASE("Destroying an entity changes the checksum", "[ecs][Checksum]") {
	World world;
	build_reference_world(world);
	EntityID const extra = world.create_entity(CkValue { 7 });

	uint64_t const before = world_checksum(world);
	world.destroy_entity(extra);
	CHECK(world_checksum(world) != before);
}

TEST_CASE("Generation bump changes the checksum even with identical component values", "[ecs][Checksum]") {
	World world;
	EntityID const eid = world.create_entity(CkValue { 7 });
	uint64_t const before = world_checksum(world);

	world.destroy_entity(eid);
	EntityID const reborn = world.create_entity(CkValue { 7 });
	// Slot reuse: same index, bumped generation.
	CHECK(reborn.index == eid.index);
	CHECK(reborn.generation != eid.generation);
	CHECK(world_checksum(world) != before);
}

TEST_CASE("Mutating a plain singleton changes the checksum", "[ecs][Checksum]") {
	World world;
	build_reference_world(world);

	uint64_t const before = world_checksum(world);
	world.get_singleton<CkConfigA>()->v += 1;
	CHECK(world_checksum(world) != before);
}

TEST_CASE("Mutating an element inside a vector-holding singleton changes the checksum", "[ecs][Checksum]") {
	World world;
	build_reference_world(world);

	uint64_t const before = world_checksum(world);
	world.get_singleton<CkVecSingleton>()->values[1] = 99;
	CHECK(world_checksum(world) != before);
}

TEST_CASE("Tag presence contributes to the checksum via the signature fold", "[ecs][Checksum]") {
	World with_tag;
	with_tag.create_entity(CkValue { 7 }, CkTag {});
	World without_tag;
	without_tag.create_entity(CkValue { 7 });
	CHECK(world_checksum(with_tag) != world_checksum(without_tag));
}

TEST_CASE("Checksum is insensitive to drained archetypes", "[ecs][Checksum]") {
	// World A once held a CkUnique archetype that is now empty; World B never created it.
	// Live state is identical — a future loader would not recreate the empty archetype, so
	// the checksum must not see it.
	World a;
	a.create_entity(CkValue { 7 });
	EntityID const transient = a.create_entity(CkUnique { 1 });
	a.destroy_entity(transient);

	World b;
	b.create_entity(CkValue { 7 });

	CHECK(world_checksum(a) == world_checksum(b));
}

// === (e) custom hash path + singleton ordering ===

TEST_CASE("Custom hash takes precedence over the byte path", "[ecs][Checksum]") {
	// CkCustom IS uniquely representable — if the byte path were taken, `ignored` would
	// move the checksum.
	World a;
	a.create_entity(CkCustom { 5, 111 });
	World b;
	b.create_entity(CkCustom { 5, 222 });
	CHECK(world_checksum(a) == world_checksum(b));

	World c;
	c.create_entity(CkCustom { 6, 111 });
	CHECK(world_checksum(a) != world_checksum(c));
}

TEST_CASE("Tag columns carry no hash thunk; data columns do", "[ecs][Checksum]") {
	CHECK(column_vtable_for<CkValue>().hash_rows != nullptr);
	CHECK(column_vtable_for<CkCustom>().hash_rows != nullptr);
	CHECK(column_vtable_for<CkTag>().hash_rows == nullptr);
}

TEST_CASE("Singletons fold in component-id order regardless of set order", "[ecs][Checksum]") {
	World ab;
	ab.set_singleton(CkConfigA { 1 });
	ab.set_singleton(CkConfigB { 2 });
	World reversed;
	reversed.set_singleton(CkConfigB { 2 });
	reversed.set_singleton(CkConfigA { 1 });

	CHECK(world_checksum(ab) == world_checksum(reversed));

	WorldChecksumBreakdown const breakdown = world_checksum_breakdown(ab);
	REQUIRE(breakdown.singleton_entries.size() == 2u);
	CHECK(breakdown.singleton_entries[0].id < breakdown.singleton_entries[1].id);
}

TEST_CASE("Tag singleton presence is visible; clearing it restores the checksum", "[ecs][Checksum]") {
	World world;
	world.create_entity(CkValue { 7 });

	uint64_t const before = world_checksum(world);
	world.set_singleton(CkTagSingleton {});
	uint64_t const with_tag_singleton = world_checksum(world);
	CHECK(with_tag_singleton != before);

	CHECK(world.clear_singleton<CkTagSingleton>());
	CHECK(world_checksum(world) == before);
}

// === (c) worker-count invariance — the template the full-sim gate extends ===

namespace {
	uint64_t run_and_checksum(uint32_t worker_count, bool serial_mode, std::size_t seed_count, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count);
		if (serial_mode) {
			world.set_serial_mode(true);
		}

		for (std::size_t i = 0; i < seed_count; ++i) {
			world.create_entity(
				CkSeed { static_cast<int64_t>((i * 17) % 251 + 1) },
				CkValue { static_cast<int64_t>(i + 1) }
			);
		}
		world.set_singleton(CkVecSingleton { { 4, 5, 6 } });

		world.register_system<CkSpawner>();
		world.register_system<CkChurn>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		return world_checksum(world);
	}
}

TEST_CASE("Full-state checksum is identical across worker counts and serial mode",
          "[ecs][Checksum][determinism][WorkerCountInvariance]") {
	std::size_t const seeds = 200;
	int const ticks = 5;
	uint64_t const baseline = run_and_checksum(1, false, seeds, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		uint64_t const result = run_and_checksum(wc, false, seeds, ticks);
		CHECK(result == baseline);
	}

	uint64_t const serial = run_and_checksum(1, true, seeds, ticks);
	CHECK(serial == baseline);
}
