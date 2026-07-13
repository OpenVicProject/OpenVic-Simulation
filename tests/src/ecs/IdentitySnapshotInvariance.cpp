#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === The save/load determinism gate (ECS_SIM_ARCHITECTURE §10) ===
// digest(tick^k(restore(snapshot(s)))) == digest(tick^k(s)), at every worker count.
//
// Scenario design note — id assignment is packing-SENSITIVE by construction, and that shapes
// this test deliberately:
//   - IsiSeed (the spawner's source archetype) is created up front and never destroyed, so its
//     canonical slot-index recreation order equals its historical packing. The per-row
//     cmd.create_entity requests therefore happen in the SAME sequence in the continued and
//     restored runs, and the identity layer guarantees the same sequence yields the same ids.
//     A spawner iterating an archetype whose packing differs from the saved run WOULD assign
//     different ids — by design; that is the §10 obligation on game code (iterate in id /
//     dense-index order for anything id-assignment-sensitive), not an identity-layer bug.
//   - IsiChurn (a separate archetype) IS churned before the save point, so its restored packing
//     genuinely differs from the continued run's historical (swap-pop scrambled) packing. The
//     pure per-row IsiChurnStep over it proves value-level packing invariance, and the freed
//     churn slots are what the spawner's deferred creates must reuse identically in both
//     branches — the free-list-fidelity claim, end-to-end (and what lets EntityID key the
//     §8 RNG and full-state digests).

namespace {
	struct IsiSeed {
		int64_t seed = 0;
	};
	struct IsiSpawned {
		int64_t derived = 0;
	};
	struct IsiChurn {
		int64_t v = 0;
	};
}
ECS_COMPONENT(IsiSeed, "test_IdentitySnapshotInvariance::Seed")
ECS_COMPONENT(IsiSpawned, "test_IdentitySnapshotInvariance::Spawned")
ECS_COMPONENT(IsiChurn, "test_IdentitySnapshotInvariance::Churn")

namespace {
	// Threaded spawner: one IsiSpawned per IsiSeed per tick, deterministic derived value.
	struct IsiSpawnerThreaded : SystemThreaded<IsiSpawnerThreaded> {
		void tick(TickContext const& ctx, IsiSeed const& s) {
			ctx.cmd.create_entity(ctx.world, IsiSpawned { s.seed * 31 + 7 });
		}
	};

	// Pure per-row arithmetic over the churned archetype — packing-invariant by §10.
	struct IsiChurnStep : SystemThreaded<IsiChurnStep> {
		void tick(TickContext const& /*ctx*/, IsiChurn& c) {
			c.v = c.v * 31 + 11;
		}
	};
}
ECS_SYSTEM(IsiSpawnerThreaded)
ECS_SYSTEM(IsiChurnStep)

namespace {
	std::size_t const SEED_COUNT = 200;
	std::size_t const CHURN_COUNT = 300;
	int const TICKS_AFTER_SAVE = 5;

	// Captured live entity at the save point: which archetype + the component value.
	struct CapturedEntity {
		EntityID eid;
		bool is_seed = false;
		int64_t value = 0;
	};

	// Seeds first (slot-index order == creation order, never destroyed), then churn entities,
	// then scattered churn destroys (~90 of 300) that scramble churn packing via swap-pop and
	// seed the free list the post-save spawner will drain.
	void build_base(World& world) {
		for (std::size_t i = 0; i < SEED_COUNT; ++i) {
			world.create_entity(IsiSeed { static_cast<int64_t>((i * 17) % 251 + 1) });
		}
		std::vector<EntityID> churn_ids;
		churn_ids.reserve(CHURN_COUNT);
		for (std::size_t i = 0; i < CHURN_COUNT; ++i) {
			churn_ids.push_back(world.create_entity(IsiChurn { static_cast<int64_t>((i * 13) % 97 + 1) }));
		}
		for (std::size_t i = 0; i < CHURN_COUNT; ++i) {
			if (i % 10 < 3) {
				world.destroy_entity(churn_ids[i]);
			}
		}
	}

	void register_and_tick(World& world) {
		world.register_system<IsiSpawnerThreaded>();
		world.register_system<IsiChurnStep>();
		for (int t = 0; t < TICKS_AFTER_SAVE; ++t) {
			world.tick_systems(Date {});
		}
	}

	// Packing-invariant full-state digest: collect (eid, value) per component type, sort by
	// slot index (unique per live entity), fold value and id.
	int64_t digest_world(World& world) {
		std::vector<CapturedEntity> entries;
		world.for_each_with_entity<IsiSeed>([&](EntityID e, IsiSeed& s) {
			entries.push_back(CapturedEntity { e, true, s.seed });
		});
		world.for_each_with_entity<IsiChurn>([&](EntityID e, IsiChurn& c) {
			entries.push_back(CapturedEntity { e, false, c.v });
		});
		world.for_each_with_entity<IsiSpawned>([&](EntityID e, IsiSpawned& s) {
			entries.push_back(CapturedEntity { e, false, s.derived });
		});
		std::sort(entries.begin(), entries.end(), [](CapturedEntity const& a, CapturedEntity const& b) {
			return a.eid.index < b.eid.index;
		});
		int64_t digest = 0;
		for (CapturedEntity const& entry : entries) {
			digest = digest * 1000003 + entry.value;
			digest ^= static_cast<int64_t>(entry.eid.to_uint64());
		}
		return digest;
	}

	std::vector<uint64_t> sorted_spawned_ids(World& world) {
		std::vector<uint64_t> ids;
		world.for_each_with_entity<IsiSpawned>([&](EntityID e, IsiSpawned& /*s*/) {
			ids.push_back(e.to_uint64());
		});
		std::sort(ids.begin(), ids.end());
		return ids;
	}

	struct RunResult {
		int64_t continued_digest = 0;
		int64_t restored_digest = 0;
		std::vector<uint64_t> continued_spawned;
		std::vector<uint64_t> restored_spawned;
	};

	RunResult run_both_branches(uint32_t worker_count) {
		RunResult result;

		// Build the pre-save state and take the save point.
		World continued;
		continued.set_ecs_worker_count(worker_count);
		build_base(continued);

		WorldIdentitySnapshot snap;
		REQUIRE(continued.snapshot_identity(snap));

		std::vector<CapturedEntity> captured;
		continued.for_each_with_entity<IsiSeed>([&](EntityID e, IsiSeed& s) {
			captured.push_back(CapturedEntity { e, true, s.seed });
		});
		continued.for_each_with_entity<IsiChurn>([&](EntityID e, IsiChurn& c) {
			captured.push_back(CapturedEntity { e, false, c.v });
		});

		// Branch A: continue the never-saved run.
		register_and_tick(continued);
		result.continued_digest = digest_world(continued);
		result.continued_spawned = sorted_spawned_ids(continued);

		// Branch B: restore into a fresh World; recreate live entities in canonical slot-index
		// order (churn packing comes back canonical != historical — deliberately).
		World restored;
		restored.set_ecs_worker_count(worker_count);
		REQUIRE(restored.restore_identity(snap));
		std::sort(captured.begin(), captured.end(), [](CapturedEntity const& a, CapturedEntity const& b) {
			return a.eid.index < b.eid.index;
		});
		for (CapturedEntity const& entry : captured) {
			if (entry.is_seed) {
				REQUIRE(restored.restore_entity(entry.eid, IsiSeed { entry.value }));
			} else {
				REQUIRE(restored.restore_entity(entry.eid, IsiChurn { entry.value }));
			}
		}
		register_and_tick(restored);
		result.restored_digest = digest_world(restored);
		result.restored_spawned = sorted_spawned_ids(restored);

		return result;
	}
}

TEST_CASE("restore + tick digest equals continue + tick digest across worker counts",
          "[ecs][determinism][identity]") {
	RunResult const baseline = run_both_branches(1);
	CHECK(baseline.restored_digest == baseline.continued_digest);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		RunResult const result = run_both_branches(wc);
		CHECK(result.restored_digest == result.continued_digest);
		CHECK(result.continued_digest == baseline.continued_digest);
		CHECK(result.restored_digest == baseline.continued_digest);
	}
}

TEST_CASE("restored free-list reuse hands spawned entities identical EntityIDs",
          "[ecs][determinism][identity]") {
	// Sharper failure message than a digest mismatch: the exact spawned-id sets must match —
	// early ticks drain the churned free list, later ticks grow fresh slots past it.
	RunResult const result = run_both_branches(4);
	REQUIRE(result.continued_spawned.size() == result.restored_spawned.size());
	CHECK(result.continued_spawned == result.restored_spawned);
}
