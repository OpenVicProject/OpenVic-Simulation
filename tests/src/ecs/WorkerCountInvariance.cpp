#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === The multiplayer-determinism contract test. ===
// Same starting World + same input → bit-identical post-tick state for any worker_count.
// This is what guarantees lockstep-multiplayer correctness within the ECS scheduler.

namespace {
	struct WciValue {
		int64_t v = 0;
	};
	struct WciDelta {
		int64_t d = 0;
	};
}
ECS_COMPONENT(WciValue, "test_WorkerCountInvariance::Value")
ECS_COMPONENT(WciDelta, "test_WorkerCountInvariance::Delta")

namespace {
	// Serial system: writes WciValue, reads WciDelta. Pure per-row arithmetic — no global
	// shared state, no RNG. Output depends only on the per-entity WciValue/WciDelta pair
	// and the number of ticks.
	struct WciStepSerial : System<WciStepSerial> {
		void tick(TickContext const& /*ctx*/, WciValue& v, WciDelta const& d) {
			v.v = v.v * 31 + d.d * 7;
		}
	};

	// Threaded variant doing the same per-row computation. Uses SystemThreaded base —
	// chunk-parallel iteration, per-chunk CommandBuffers.
	struct WciStepThreaded : SystemThreaded<WciStepThreaded> {
		void tick(TickContext const& /*ctx*/, WciValue& v, WciDelta const& d) {
			v.v = v.v * 31 + d.d * 7;
		}
	};
}
ECS_SYSTEM(WciStepSerial)
ECS_SYSTEM(WciStepThreaded)

namespace {
	// Build a deterministic World state and tick the chosen system N times. Returns the
	// final state digest (sum of all WciValue.v values, in EntityID order).
	template<typename SystemT>
	int64_t run_and_digest(uint32_t worker_count, std::size_t entity_count, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count);

		std::vector<EntityID> ids;
		ids.reserve(entity_count);
		for (std::size_t i = 0; i < entity_count; ++i) {
			ids.push_back(world.create_entity(
				WciValue { static_cast<int64_t>(i + 1) },
				WciDelta { static_cast<int64_t>((i * 17) % 13 + 1) }
			));
		}

		world.register_system<SystemT>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		// Digest: sum WciValue.v in (deterministic) EntityID-order traversal.
		int64_t digest = 0;
		for (EntityID const& id : ids) {
			WciValue const* val = world.get_component<WciValue>(id);
			if (val != nullptr) {
				digest = digest * 1000003 + val->v;
			}
		}
		return digest;
	}
}

TEST_CASE("Serial system: digest is identical across worker counts",
          "[ecs][determinism][WorkerCountInvariance]") {
	std::size_t const entities = 500;
	int const ticks = 10;
	int64_t baseline = run_and_digest<WciStepSerial>(1, entities, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		int64_t result = run_and_digest<WciStepSerial>(wc, entities, ticks);
		CHECK(result == baseline);
	}
}

TEST_CASE("Threaded system: digest is identical across worker counts",
          "[ecs][determinism][WorkerCountInvariance]") {
	std::size_t const entities = 500;
	int const ticks = 10;
	int64_t baseline = run_and_digest<WciStepThreaded>(1, entities, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		int64_t result = run_and_digest<WciStepThreaded>(wc, entities, ticks);
		CHECK(result == baseline);
	}
}

TEST_CASE("Serial and threaded systems produce identical results",
          "[ecs][determinism][WorkerCountInvariance]") {
	std::size_t const entities = 500;
	int const ticks = 10;
	int64_t serial_digest = run_and_digest<WciStepSerial>(1, entities, ticks);
	int64_t threaded_digest = run_and_digest<WciStepThreaded>(8, entities, ticks);
	CHECK(serial_digest == threaded_digest);
}

// === Deferred-create-from-threaded determinism ===
// Verifies that a SystemThreaded body calling cmd.create_entity produces a bit-identical
// post-tick state regardless of worker count. The deferred-create path resolves placeholders
// to real EntityIDs at apply time, on a single thread, in chunk_idx ascending order — so the
// allocation order is worker-count-invariant by construction.

namespace {
	struct WciSeed {
		int64_t seed = 0;
	};
	struct WciSpawned {
		int64_t derived = 0;
	};
}
ECS_COMPONENT(WciSeed, "test_WorkerCountInvariance::Seed")
ECS_COMPONENT(WciSpawned, "test_WorkerCountInvariance::Spawned")

namespace {
	// Threaded spawner: every WciSeed entity spawns exactly one WciSpawned with a deterministic
	// derived value. Pure per-row compute, no shared state — all spawn order ambiguity comes
	// from the deferred-resolution pipeline.
	struct WciSpawnerThreaded : SystemThreaded<WciSpawnerThreaded> {
		void tick(TickContext const& ctx, EntityID, WciSeed const& s) {
			ctx.cmd.create_entity(ctx.world, WciSpawned { s.seed * 31 + 7 });
		}
	};
}
ECS_SYSTEM(WciSpawnerThreaded)

namespace {
	// Build a deterministic seed population, tick the spawner once, digest the full WciSpawned
	// population in chunk-iteration order. Since World iteration order is itself
	// archetype-then-chunk-then-row deterministic given identical apply order, the digest
	// captures whether deferred-allocation order varied across worker counts.
	int64_t spawn_and_digest(uint32_t worker_count, std::size_t seed_count, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count);

		for (std::size_t i = 0; i < seed_count; ++i) {
			world.create_entity(WciSeed { static_cast<int64_t>((i * 17) % 251 + 1) });
		}

		world.register_system<WciSpawnerThreaded>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		int64_t digest = 0;
		world.for_each_with_entity<WciSpawned>([&](EntityID e, WciSpawned& s) {
			digest = digest * 1000003 + s.derived;
			digest ^= static_cast<int64_t>(e.to_uint64());
		});
		return digest;
	}
}

TEST_CASE("Deferred-create from SystemThreaded: digest is identical across worker counts",
          "[ecs][determinism][WorkerCountInvariance][deferred]") {
	std::size_t const seeds = 500;
	int const ticks = 1;
	int64_t baseline = spawn_and_digest(1, seeds, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		int64_t result = spawn_and_digest(wc, seeds, ticks);
		CHECK(result == baseline);
	}
}

TEST_CASE("Deferred-create from SystemThreaded: digest stays identical across multiple ticks",
          "[ecs][determinism][WorkerCountInvariance][deferred]") {
	// Multi-tick: each tick adds another generation of WciSpawned. Catches ordering instability
	// that compounds across tick boundaries.
	std::size_t const seeds = 200;
	int const ticks = 5;
	int64_t baseline = spawn_and_digest(1, seeds, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		int64_t result = spawn_and_digest(wc, seeds, ticks);
		CHECK(result == baseline);
	}
}
