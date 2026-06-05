#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <atomic>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// ============================================================================
// Phase 0 gate: multi-system stages where any mix of System<> and SystemThreaded
// run in parallel via one outer parallel_for. Asserts correctness (per-row
// values), determinism (digest invariance across worker counts), CB merge order
// (deferred-create finalisation order is worker-count-invariant), and the
// existing single-system path still works.
// ============================================================================

// === Components ===
// Each component is touched by exactly one of the systems in the stage so that
// the systems are conflict-free and the scheduler lands them in the same stage.

namespace {
	struct MmsA { int64_t v = 0; };
	struct MmsB { int64_t v = 0; };
	struct MmsC { int64_t v = 0; };
	struct MmsD { int64_t v = 0; };
	struct MmsSeed { int64_t k = 0; };
}
ECS_COMPONENT(MmsA, "test_MultiSystemMixedStage::A")
ECS_COMPONENT(MmsB, "test_MultiSystemMixedStage::B")
ECS_COMPONENT(MmsC, "test_MultiSystemMixedStage::C")
ECS_COMPONENT(MmsD, "test_MultiSystemMixedStage::D")
ECS_COMPONENT(MmsSeed, "test_MultiSystemMixedStage::Seed")

// === Systems ===
// Each writes a different component so the scheduler lands them all in the same stage
// (no access conflicts → no auto-orientation forced ordering).

namespace {
	// Threaded system writing component A.
	struct MmsWriteAThreaded : SystemThreaded<MmsWriteAThreaded> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& s, MmsA& a) {
			a.v = s.k * 31 + 7;
		}
	};

	// Threaded system writing component B.
	struct MmsWriteBThreaded : SystemThreaded<MmsWriteBThreaded> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& s, MmsB& b) {
			b.v = s.k * 13 - 11;
		}
	};

	// Plain System<> writing component C.
	struct MmsWriteCSerial : System<MmsWriteCSerial> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& s, MmsC& c) {
			c.v = s.k * 5 + 2;
		}
	};

	// Plain System<> writing component D.
	struct MmsWriteDSerial : System<MmsWriteDSerial> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& s, MmsD& d) {
			d.v = s.k - 3;
		}
	};
}
ECS_SYSTEM(MmsWriteAThreaded)
ECS_SYSTEM(MmsWriteBThreaded)
ECS_SYSTEM(MmsWriteCSerial)
ECS_SYSTEM(MmsWriteDSerial)

namespace {
	// Common fixture: N entities each carrying Seed + all four writable components.
	// Returns the created entity ids in insertion order.
	std::vector<EntityID> seed_world(World& world, std::size_t N) {
		std::vector<EntityID> ids;
		ids.reserve(N);
		for (std::size_t i = 0; i < N; ++i) {
			ids.push_back(world.create_entity(
				MmsSeed { static_cast<int64_t>(i + 1) },
				MmsA {}, MmsB {}, MmsC {}, MmsD {}
			));
		}
		return ids;
	}
}

// ============================================================================
// Test 1: Two SystemThreaded sharing a stage.
// ============================================================================

TEST_CASE("Two SystemThreaded sharing a stage write disjoint components correctly",
          "[ecs][MultiSystemMixedStage]") {
	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 500;
	std::vector<EntityID> const ids = seed_world(world, N);

	world.register_system<MmsWriteAThreaded>();
	world.register_system<MmsWriteBThreaded>();
	world.tick_systems(Date {});

	for (std::size_t i = 0; i < N; ++i) {
		MmsA const* a = world.get_component<MmsA>(ids[i]);
		MmsB const* b = world.get_component<MmsB>(ids[i]);
		REQUIRE(a != nullptr);
		REQUIRE(b != nullptr);
		CHECK(a->v == static_cast<int64_t>(i + 1) * 31 + 7);
		CHECK(b->v == static_cast<int64_t>(i + 1) * 13 - 11);
	}
}

// ============================================================================
// Test 2: Two plain System<> sharing a stage.
// ============================================================================

TEST_CASE("Two plain System<> sharing a stage write disjoint components correctly",
          "[ecs][MultiSystemMixedStage]") {
	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 200;
	std::vector<EntityID> const ids = seed_world(world, N);

	world.register_system<MmsWriteCSerial>();
	world.register_system<MmsWriteDSerial>();
	world.tick_systems(Date {});

	for (std::size_t i = 0; i < N; ++i) {
		MmsC const* c = world.get_component<MmsC>(ids[i]);
		MmsD const* d = world.get_component<MmsD>(ids[i]);
		REQUIRE(c != nullptr);
		REQUIRE(d != nullptr);
		CHECK(c->v == static_cast<int64_t>(i + 1) * 5 + 2);
		CHECK(d->v == static_cast<int64_t>(i + 1) - 3);
	}
}

// ============================================================================
// Test 3: Mixed 1 SystemThreaded + 1 plain System<>.
// ============================================================================

TEST_CASE("Mixed stage 1 SystemThreaded + 1 plain System<> both correct",
          "[ecs][MultiSystemMixedStage]") {
	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 300;
	std::vector<EntityID> const ids = seed_world(world, N);

	world.register_system<MmsWriteAThreaded>();
	world.register_system<MmsWriteCSerial>();
	world.tick_systems(Date {});

	for (std::size_t i = 0; i < N; ++i) {
		MmsA const* a = world.get_component<MmsA>(ids[i]);
		MmsC const* c = world.get_component<MmsC>(ids[i]);
		REQUIRE(a != nullptr);
		REQUIRE(c != nullptr);
		CHECK(a->v == static_cast<int64_t>(i + 1) * 31 + 7);
		CHECK(c->v == static_cast<int64_t>(i + 1) * 5 + 2);
	}
}

// ============================================================================
// Test 4: Mixed 2 SystemThreaded + 2 plain System<> (full combinatorial case).
// ============================================================================

TEST_CASE("Mixed stage with 2 SystemThreaded + 2 plain System<> all correct",
          "[ecs][MultiSystemMixedStage]") {
	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 400;
	std::vector<EntityID> const ids = seed_world(world, N);

	world.register_system<MmsWriteAThreaded>();
	world.register_system<MmsWriteBThreaded>();
	world.register_system<MmsWriteCSerial>();
	world.register_system<MmsWriteDSerial>();
	world.tick_systems(Date {});

	for (std::size_t i = 0; i < N; ++i) {
		int64_t const k = static_cast<int64_t>(i + 1);
		MmsA const* a = world.get_component<MmsA>(ids[i]);
		MmsB const* b = world.get_component<MmsB>(ids[i]);
		MmsC const* c = world.get_component<MmsC>(ids[i]);
		MmsD const* d = world.get_component<MmsD>(ids[i]);
		REQUIRE(a != nullptr);
		REQUIRE(b != nullptr);
		REQUIRE(c != nullptr);
		REQUIRE(d != nullptr);
		CHECK(a->v == k * 31 + 7);
		CHECK(b->v == k * 13 - 11);
		CHECK(c->v == k * 5 + 2);
		CHECK(d->v == k - 3);
	}
}

// ============================================================================
// Test 5: Worker-count invariance — multi-system stage digest is bit-identical
// across worker_count ∈ {1, 2, 4, 8, 16}. This is the determinism gate.
// ============================================================================

namespace {
	int64_t run_mixed_and_digest(uint32_t worker_count, std::size_t N, int ticks) {
		World world;
		world.set_ecs_worker_count(worker_count);
		std::vector<EntityID> ids = seed_world(world, N);

		world.register_system<MmsWriteAThreaded>();
		world.register_system<MmsWriteBThreaded>();
		world.register_system<MmsWriteCSerial>();
		world.register_system<MmsWriteDSerial>();

		for (int t = 0; t < ticks; ++t) {
			world.tick_systems(Date {});
		}

		int64_t digest = 0;
		for (EntityID const& id : ids) {
			MmsA const* a = world.get_component<MmsA>(id);
			MmsB const* b = world.get_component<MmsB>(id);
			MmsC const* c = world.get_component<MmsC>(id);
			MmsD const* d = world.get_component<MmsD>(id);
			digest = digest * 1000003 + (a ? a->v : 0);
			digest = digest * 1000003 + (b ? b->v : 0);
			digest = digest * 1000003 + (c ? c->v : 0);
			digest = digest * 1000003 + (d ? d->v : 0);
		}
		return digest;
	}
}

TEST_CASE("Multi-system mixed stage: digest is identical across worker counts",
          "[ecs][MultiSystemMixedStage][determinism]") {
	std::size_t const entities = 500;
	int const ticks = 10;
	int64_t baseline = run_mixed_and_digest(1, entities, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		int64_t result = run_mixed_and_digest(wc, entities, ticks);
		CHECK(result == baseline);
	}
}

// ============================================================================
// Test 6: Deterministic per-system CB merge order — a SystemThreaded calling
// ctx.cmd.create_entity from a multi-system stage produces the same finalised
// entity layout across worker counts. This is the same invariant the existing
// SystemThreadedSpawn.cpp tests for a single-system stage; here we put the
// spawner in a multi-system stage to confirm the new merge path keeps the
// chunk_local_idx-ascending order.
// ============================================================================

namespace {
	struct MmsSpawnSrc { int32_t id = 0; };
	struct MmsSpawned { int32_t source_id = 0; };
}
ECS_COMPONENT(MmsSpawnSrc, "test_MultiSystemMixedStage::SpawnSrc")
ECS_COMPONENT(MmsSpawned, "test_MultiSystemMixedStage::Spawned")

namespace {
	// Threaded spawner — one Spawned per source.
	struct MmsSpawnerThreaded : SystemThreaded<MmsSpawnerThreaded> {
		void tick(TickContext const& ctx, EntityID, MmsSpawnSrc const& src) {
			ctx.cmd.create_entity(ctx.world, MmsSpawned { src.id * 31 + 7 });
		}
	};

	// Pure noop plain System<> living in the same stage — its presence forces the
	// scheduler down the multi-system parallel path. Touches Seed (R-only) so it
	// conflicts with nothing — same stage as the threaded spawner.
	struct MmsNoopSerial : System<MmsNoopSerial> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& /*s*/) {}
	};
}
ECS_SYSTEM(MmsSpawnerThreaded)
ECS_SYSTEM(MmsNoopSerial)

namespace {
	std::vector<int32_t> spawn_and_capture(uint32_t worker_count, std::size_t source_count) {
		World world;
		world.set_ecs_worker_count(worker_count);

		// Sources carry both SpawnSrc (for the spawner) and Seed (for the noop). Same
		// archetype keeps the analysis simple.
		for (std::size_t i = 0; i < source_count; ++i) {
			world.create_entity(
				MmsSpawnSrc { static_cast<int32_t>(i) },
				MmsSeed { static_cast<int64_t>(i) }
			);
		}

		world.register_system<MmsSpawnerThreaded>();
		world.register_system<MmsNoopSerial>();
		world.tick_systems(Date {});

		std::vector<int32_t> order;
		world.for_each<MmsSpawned>([&order](MmsSpawned& s) {
			order.push_back(s.source_id);
		});
		return order;
	}
}

TEST_CASE("Multi-system stage with threaded spawner: finalised order invariant",
          "[ecs][MultiSystemMixedStage][determinism]") {
	std::size_t const sources = 250;
	std::vector<int32_t> const baseline = spawn_and_capture(1, sources);
	REQUIRE(baseline.size() == sources);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		std::vector<int32_t> const order = spawn_and_capture(wc, sources);
		REQUIRE(order.size() == baseline.size());
		for (std::size_t i = 0; i < baseline.size(); ++i) {
			CHECK(order[i] == baseline[i]);
		}
	}
}

// ============================================================================
// Test 7: Single-system stages unchanged — both a single SystemThreaded and a
// single plain System<> should keep producing the same results as before.
// ============================================================================

TEST_CASE("Single SystemThreaded stage still produces expected per-row results",
          "[ecs][MultiSystemMixedStage]") {
	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 250;
	std::vector<EntityID> const ids = seed_world(world, N);

	world.register_system<MmsWriteAThreaded>();
	world.tick_systems(Date {});

	for (std::size_t i = 0; i < N; ++i) {
		MmsA const* a = world.get_component<MmsA>(ids[i]);
		REQUIRE(a != nullptr);
		CHECK(a->v == static_cast<int64_t>(i + 1) * 31 + 7);
	}
}

TEST_CASE("Single plain System<> stage still produces expected per-row results",
          "[ecs][MultiSystemMixedStage]") {
	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 250;
	std::vector<EntityID> const ids = seed_world(world, N);

	world.register_system<MmsWriteCSerial>();
	world.tick_systems(Date {});

	for (std::size_t i = 0; i < N; ++i) {
		MmsC const* c = world.get_component<MmsC>(ids[i]);
		REQUIRE(c != nullptr);
		CHECK(c->v == static_cast<int64_t>(i + 1) * 5 + 2);
	}
}

// ============================================================================
// Test 8: Multi-system stage with a plain System<> recording cmd.add_component.
// The deferred op applies at the stage barrier and the next-stage system sees
// the new archetype. Confirms the apply loop is still correct on the new path.
// ============================================================================

namespace {
	struct MmsTag {}; // tag component added at apply time
}
ECS_COMPONENT(MmsTag, "test_MultiSystemMixedStage::Tag")

namespace {
	// Plain System<> in the stage — defers an add_component<MmsTag>(eid) per row.
	struct MmsTaggerSerial : System<MmsTaggerSerial> {
		void tick(TickContext const& ctx, EntityID eid, MmsSeed const& /*s*/) {
			ctx.cmd.template add_component<MmsTag>(eid);
		}
	};

	// SystemThreaded sharing the stage — touches a disjoint component so it co-stages
	// with the tagger. Just reads Seed (R-only).
	struct MmsThreadedReader : SystemThreaded<MmsThreadedReader> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& /*s*/) {}
	};
}
ECS_SYSTEM(MmsTaggerSerial)
ECS_SYSTEM(MmsThreadedReader)

// ============================================================================
// Test 9: Real-concurrency detection. Records the peak number of tick bodies
// running simultaneously across both systems in a multi-system stage. With a
// brief in-tick spin and worker_count == 4 we expect peak >= 2 — proving the
// new path runs work bodies on multiple workers rather than silently funnelling
// to one. Held in its own namespace so the global state doesn't leak.
// ============================================================================

namespace mms_concurrency {
	std::atomic<int> g_active { 0 };
	std::atomic<int> g_peak { 0 };

	void enter_tick_and_observe() {
		int const now = g_active.fetch_add(1) + 1;
		int prev = g_peak.load();
		while (now > prev && !g_peak.compare_exchange_weak(prev, now)) {
			// loop
		}
		// Brief busy-wait so other workers have a window to enter the tick and
		// raise `g_active` above 1. Non-blocking — no syscall, no allocator. The
		// constant is chosen empirically: long enough that thread scheduling has
		// time to overlap, short enough not to dominate test runtime.
		volatile int spin = 0;
		for (int i = 0; i < 2000; ++i) {
			spin += i;
		}
		(void) spin;
		g_active.fetch_sub(1);
	}

	struct MmsConcThreadedA : SystemThreaded<MmsConcThreadedA> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& /*s*/, MmsA& a) {
			enter_tick_and_observe();
			a.v = 1;
		}
	};

	struct MmsConcThreadedB : SystemThreaded<MmsConcThreadedB> {
		void tick(TickContext const& /*ctx*/, MmsSeed const& /*s*/, MmsB& b) {
			enter_tick_and_observe();
			b.v = 1;
		}
	};
}
ECS_SYSTEM(mms_concurrency::MmsConcThreadedA)
ECS_SYSTEM(mms_concurrency::MmsConcThreadedB)

TEST_CASE("Multi-system stage actually runs bodies concurrently",
          "[ecs][MultiSystemMixedStage]") {
	using namespace mms_concurrency;

	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 2000; // many chunks → many work items → ample overlap opportunity
	(void) seed_world(world, N);

	g_active.store(0);
	g_peak.store(0);

	world.register_system<MmsConcThreadedA>();
	world.register_system<MmsConcThreadedB>();
	world.tick_systems(Date {});

	int const peak = g_peak.load();
	// At worker_count == 4 with 2 threaded systems × many chunks across 4 workers,
	// at least 2 tick bodies should overlap. If this ever drops to 1, the new
	// multi-system path has regressed to silent serial dispatch.
	CHECK(peak >= 2);
}

TEST_CASE("Multi-system stage applies deferred add_component at stage barrier",
          "[ecs][MultiSystemMixedStage]") {
	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 60;
	std::vector<EntityID> ids;
	ids.reserve(N);
	for (std::size_t i = 0; i < N; ++i) {
		ids.push_back(world.create_entity(MmsSeed { static_cast<int64_t>(i) }));
	}

	world.register_system<MmsTaggerSerial>();
	world.register_system<MmsThreadedReader>();
	world.tick_systems(Date {});

	// After tick: every original entity should carry MmsTag (added via cmd in the
	// multi-system stage and applied at the stage barrier).
	std::size_t tagged = 0;
	for (EntityID const& id : ids) {
		if (world.has_component<MmsTag>(id)) {
			++tagged;
		}
	}
	CHECK(tagged == N);
}
