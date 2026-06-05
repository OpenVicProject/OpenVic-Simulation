#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <array>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// Integration test: a SystemThreaded calls cmd.create_entity inside its tick body. A downstream
// serial system, sequenced via run_after, observes the spawned entities through a query and folds
// them into a singleton total. Exercises the full scheduler pipeline: parallel chunk dispatch →
// per-chunk deferred record → chunk_idx-ascending merge → stage-barrier apply → next stage's
// access to freshly-finalised entities.

namespace {
	struct STSSource {
		int32_t id = 0;
	};
	struct STSSpawnCount {
		int32_t count = 0;
	};
	struct STSSpawned {
		int32_t source_id = 0;
	};
	struct STSSpawnedTotal {
		int64_t value = 0;
	};
}
ECS_COMPONENT(STSSource, "test_SystemThreadedSpawn::Source")
ECS_COMPONENT(STSSpawnCount, "test_SystemThreadedSpawn::SpawnCount")
ECS_COMPONENT(STSSpawned, "test_SystemThreadedSpawn::Spawned")
ECS_COMPONENT(STSSpawnedTotal, "test_SystemThreadedSpawn::SpawnedTotal")

namespace {
	// SpawnSystem: for each (Source, SpawnCount) row, queue `count` deferred Spawned creates
	// carrying a back-reference to the source's `id`. Runs chunk-parallel; placeholders resolve
	// at the stage barrier in chunk_idx ascending order.
	struct STSSpawnSystem : SystemThreaded<STSSpawnSystem> {
		void tick(TickContext const& ctx, EntityID, STSSource const& src, STSSpawnCount const& sc) {
			for (int i = 0; i < sc.count; ++i) {
				ctx.cmd.create_entity(ctx.world, STSSpawned { src.id });
			}
		}
	};

	// SpawnedCounter: serial; runs after SpawnSystem; sums the spawned-population size into the
	// SpawnedTotal singleton. Verifies the spawned entities are visible (real EntityIDs assigned,
	// archetype materialised) by the next stage.
	struct STSSpawnedCounter : System<STSSpawnedCounter> {
		void tick(TickContext const& ctx, STSSpawned const&) {
			STSSpawnedTotal* total = ctx.world.get_singleton<STSSpawnedTotal>();
			if (total != nullptr) {
				total->value += 1;
			}
		}

		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<STSSpawnSystem>() };
		}
	};
}
ECS_SYSTEM(STSSpawnSystem)
ECS_SYSTEM(STSSpawnedCounter)

namespace {
	// Run the scenario: pre-populate world, register both systems, tick once, return totals.
	struct ScenarioResult {
		int64_t spawned_total = 0;
		std::size_t source_alive_count = 0;
		std::size_t spawned_with_valid_back_ref = 0;
		std::vector<int32_t> spawned_source_ids; // captured in iteration order
	};

	ScenarioResult run_scenario(uint32_t worker_count, std::size_t source_count) {
		World world;
		world.set_ecs_worker_count(worker_count);
		world.set_singleton<STSSpawnedTotal>(STSSpawnedTotal { 0 });

		// Pre-populate sources. count = (i % 4) + 1 → average 2.5 spawns per source.
		std::vector<EntityID> source_ids;
		source_ids.reserve(source_count);
		for (std::size_t i = 0; i < source_count; ++i) {
			source_ids.push_back(world.create_entity(
				STSSource { static_cast<int32_t>(i) },
				STSSpawnCount { static_cast<int32_t>((i % 4) + 1) }
			));
		}

		world.register_system<STSSpawnSystem>();
		world.register_system<STSSpawnedCounter>();
		world.tick_systems(Date {});

		ScenarioResult r;
		STSSpawnedTotal const* total = world.get_singleton<STSSpawnedTotal>();
		if (total != nullptr) {
			r.spawned_total = total->value;
		}

		// Confirm originals are still alive.
		for (EntityID const& id : source_ids) {
			if (world.is_alive(id)) {
				++r.source_alive_count;
			}
		}

		// Capture spawned-id list in iteration order — workers may differ in chunking, but the
		// ordering of finalised spawned entities must be identical at the same world seed.
		world.for_each<STSSpawned>([&](STSSpawned& s) {
			r.spawned_source_ids.push_back(s.source_id);
			// Walk back: every spawned entity should reference a still-alive Source entity by id.
			bool found = false;
			world.for_each<STSSource>([&](STSSource& src) {
				if (src.id == s.source_id) {
					found = true;
				}
			});
			if (found) {
				++r.spawned_with_valid_back_ref;
			}
		});

		return r;
	}
}

TEST_CASE("SystemThreaded can spawn via cmd.create_entity, downstream stage observes them",
          "[ecs][SystemThreadedSpawn]") {
	std::size_t const sources = 100;
	ScenarioResult r = run_scenario(8, sources);

	// Expected total: sum_{i=0..99} ((i % 4) + 1). Each block of 4 sources produces 1+2+3+4 = 10
	// spawns; 25 blocks → 250 spawns total.
	int64_t expected_total = 0;
	for (std::size_t i = 0; i < sources; ++i) {
		expected_total += static_cast<int64_t>((i % 4) + 1);
	}
	CHECK(r.spawned_total == expected_total);
	CHECK(r.source_alive_count == sources);
	CHECK(r.spawned_with_valid_back_ref == static_cast<std::size_t>(expected_total));
	CHECK(r.spawned_source_ids.size() == static_cast<std::size_t>(expected_total));
}

TEST_CASE("SystemThreaded spawn produces identical finalised order across worker counts",
          "[ecs][SystemThreadedSpawn][determinism]") {
	std::size_t const sources = 200;
	ScenarioResult baseline = run_scenario(1, sources);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		ScenarioResult r = run_scenario(wc, sources);
		CHECK(r.spawned_total == baseline.spawned_total);
		CHECK(r.source_alive_count == baseline.source_alive_count);
		REQUIRE(r.spawned_source_ids.size() == baseline.spawned_source_ids.size());
		// Iteration order is governed by archetype/chunk/row layout — finalisation order being
		// worker-count-invariant means the resulting rows land in identical positions.
		for (std::size_t i = 0; i < baseline.spawned_source_ids.size(); ++i) {
			CHECK(r.spawned_source_ids[i] == baseline.spawned_source_ids[i]);
		}
	}
}
