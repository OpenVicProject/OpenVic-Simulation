#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/QueryFilter.hpp"
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

// === Determinism gate for System-level exclusion filters. ===
// Same starting World + same input → bit-identical post-tick state for any worker_count, for
// FILTERED systems. The centerpiece is the multi-system parallel stage: it is the only path where
// a worker thread looks up query_cache with a system's (require, exclude) key, so it is where a
// prewarm/dispatch key mismatch (empty vs real exclude list) would surface as a data race or a
// torn read → a digest that diverges from the wc=1 baseline. See SystemScheduler.cpp prewarm and
// detail::build_tick_query.

namespace {
	struct SfwSeed { int64_t k = 0; };
	struct SfwValue { int64_t v = 0; };
	struct SfwDead {}; // excluded tag
	struct SfwOther { int64_t v = 0; }; // extra archetype variety (not excluded here)
}
ECS_COMPONENT(SfwSeed, "test_SystemFiltersWCI::Seed")
ECS_COMPONENT(SfwValue, "test_SystemFiltersWCI::Value")
ECS_COMPONENT(SfwDead, "test_SystemFiltersWCI::Dead")
ECS_COMPONENT(SfwOther, "test_SystemFiltersWCI::Other")

namespace {
	// Pure per-row arithmetic — no shared state, no RNG. Excludes SfwDead.
	struct SfwExcludeSerial : System<SfwExcludeSerial> {
		using Filters = Filter<Without<SfwDead>>;
		void tick(TickContext const& /*ctx*/, SfwValue& v, SfwSeed const& s) {
			v.v = v.v * 31 + s.k * 7;
		}
	};

	struct SfwExcludeThreaded : SystemThreaded<SfwExcludeThreaded> {
		using Filters = Filter<Without<SfwDead>>;
		void tick(TickContext const& /*ctx*/, SfwValue& v, SfwSeed const& s) {
			v.v = v.v * 31 + s.k * 7;
		}
	};
}
ECS_SYSTEM(SfwExcludeSerial)
ECS_SYSTEM(SfwExcludeThreaded)

namespace {
	// Mixed archetypes: matched ({Value,Seed} and {Value,Seed,Other}) plus excluded ({...,Dead}).
	std::vector<EntityID> seed_single(World& world, std::size_t n) {
		std::vector<EntityID> ids;
		ids.reserve(n * 3);
		for (std::size_t i = 0; i < n; ++i) {
			int64_t const k = static_cast<int64_t>(i + 1);
			ids.push_back(world.create_entity(SfwValue { k }, SfwSeed { k }));
			ids.push_back(world.create_entity(SfwValue { k }, SfwSeed { k }, SfwDead {}));
			ids.push_back(world.create_entity(SfwValue { k }, SfwSeed { k }, SfwOther { k }));
		}
		return ids;
	}

	template<typename SystemT>
	int64_t run_single_and_digest(uint32_t worker_count, std::size_t n, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count);
		std::vector<EntityID> ids = seed_single(world, n);

		world.register_system<SystemT>();
		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		int64_t digest = 0;
		for (EntityID const& id : ids) {
			SfwValue const* v = world.get_component<SfwValue>(id);
			digest = digest * 1000003 + (v != nullptr ? v->v : 0);
		}
		return digest;
	}
}

TEST_CASE("Filtered serial system: digest is identical across worker counts",
          "[ecs][SystemFilters][determinism][WorkerCountInvariance]") {
	std::size_t const entities = 400;
	int const ticks = 10;
	int64_t const baseline = run_single_and_digest<SfwExcludeSerial>(1, entities, ticks);
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		CHECK(run_single_and_digest<SfwExcludeSerial>(wc, entities, ticks) == baseline);
	}
}

TEST_CASE("Filtered threaded system: digest is identical across worker counts",
          "[ecs][SystemFilters][determinism][WorkerCountInvariance]") {
	std::size_t const entities = 400;
	int const ticks = 10;
	int64_t const baseline = run_single_and_digest<SfwExcludeThreaded>(1, entities, ticks);
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		CHECK(run_single_and_digest<SfwExcludeThreaded>(wc, entities, ticks) == baseline);
	}
}

// ============================================================================
// Centerpiece: a multi-system parallel stage of filtered systems.
//
// Four systems with DISJOINT writes (A, B, C, D), all reading MfSeed (R/R) → no conflicts → one
// stage. A MIX of SystemThreaded and plain System<> so both the worker-side `collect_matching_chunks`
// (threaded) and worker-side `dispatch_serial`→`for_each` (plain) hit resolve_query_cache with a
// NON-EMPTY exclude key. Distinct filters prewarm distinct key shapes in the one stage.
// ============================================================================

namespace {
	struct MfSeed { int64_t k = 0; };
	struct MfA { int64_t v = 0; };
	struct MfB { int64_t v = 0; };
	struct MfC { int64_t v = 0; };
	struct MfD { int64_t v = 0; };
	struct MfDead {}; // excluded by A and D
	struct MfOther {}; // excluded by B
}
ECS_COMPONENT(MfSeed, "test_SystemFiltersWCI::MfSeed")
ECS_COMPONENT(MfA, "test_SystemFiltersWCI::MfA")
ECS_COMPONENT(MfB, "test_SystemFiltersWCI::MfB")
ECS_COMPONENT(MfC, "test_SystemFiltersWCI::MfC")
ECS_COMPONENT(MfD, "test_SystemFiltersWCI::MfD")
ECS_COMPONENT(MfDead, "test_SystemFiltersWCI::MfDead")
ECS_COMPONENT(MfOther, "test_SystemFiltersWCI::MfOther")

namespace {
	// Threaded, excludes MfDead, writes MfA.
	struct MfExcludeAThreaded : SystemThreaded<MfExcludeAThreaded> {
		using Filters = Filter<Without<MfDead>>;
		void tick(TickContext const& /*ctx*/, MfSeed const& s, MfA& a) {
			a.v = a.v * 31 + s.k * 7 + 1;
		}
	};

	// Plain, excludes MfOther, writes MfB.
	struct MfExcludeBSerial : System<MfExcludeBSerial> {
		using Filters = Filter<Without<MfOther>>;
		void tick(TickContext const& /*ctx*/, MfSeed const& s, MfB& b) {
			b.v = b.v * 13 + s.k * 3 + 1;
		}
	};

	// Threaded, no filter, writes MfC.
	struct MfPlainCThreaded : SystemThreaded<MfPlainCThreaded> {
		void tick(TickContext const& /*ctx*/, MfSeed const& s, MfC& c) {
			c.v = c.v * 17 + s.k;
		}
	};

	// Plain, excludes MfDead, writes MfD.
	struct MfExcludeDSerial : System<MfExcludeDSerial> {
		using Filters = Filter<Without<MfDead>>;
		void tick(TickContext const& /*ctx*/, MfSeed const& s, MfD& d) {
			d.v = d.v * 5 + s.k * 2 + 3;
		}
	};

	// Four archetypes; every entity carries all four writable components so each writer can write
	// wherever its filter admits it. The markers (Dead/Other) vary which systems skip which rows.
	std::vector<EntityID> seed_multi(World& world, std::size_t n) {
		std::vector<EntityID> ids;
		ids.reserve(n * 4);
		for (std::size_t i = 0; i < n; ++i) {
			int64_t const k = static_cast<int64_t>(i + 1);
			ids.push_back(world.create_entity(MfSeed { k }, MfA {}, MfB {}, MfC {}, MfD {})); // P
			ids.push_back(world.create_entity(MfSeed { k }, MfA {}, MfB {}, MfC {}, MfD {}, MfDead {})); // Q
			ids.push_back(world.create_entity(MfSeed { k }, MfA {}, MfB {}, MfC {}, MfD {}, MfOther {})); // R
			ids.push_back(world.create_entity(
				MfSeed { k }, MfA {}, MfB {}, MfC {}, MfD {}, MfDead {}, MfOther {})); // S
		}
		return ids;
	}

	template<typename C>
	int64_t comp_v(World& world, EntityID id) {
		C const* p = world.get_component<C>(id);
		return p != nullptr ? p->v : -1;
	}

	int64_t run_multi_and_digest(uint32_t worker_count, std::size_t n, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count);
		std::vector<EntityID> ids = seed_multi(world, n);

		world.register_system<MfExcludeAThreaded>();
		world.register_system<MfExcludeBSerial>();
		world.register_system<MfPlainCThreaded>();
		world.register_system<MfExcludeDSerial>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		int64_t digest = 0;
		for (EntityID const& id : ids) {
			MfA const* a = world.get_component<MfA>(id);
			MfB const* b = world.get_component<MfB>(id);
			MfC const* c = world.get_component<MfC>(id);
			MfD const* d = world.get_component<MfD>(id);
			digest = digest * 1000003 + (a != nullptr ? a->v : 0);
			digest = digest * 1000003 + (b != nullptr ? b->v : 0);
			digest = digest * 1000003 + (c != nullptr ? c->v : 0);
			digest = digest * 1000003 + (d != nullptr ? d->v : 0);
		}
		return digest;
	}
}
ECS_SYSTEM(MfExcludeAThreaded)
ECS_SYSTEM(MfExcludeBSerial)
ECS_SYSTEM(MfPlainCThreaded)
ECS_SYSTEM(MfExcludeDSerial)

TEST_CASE("Multi-system filtered parallel stage: digest is identical across worker counts",
          "[ecs][SystemFilters][determinism][WorkerCountInvariance]") {
	// wc=1 runs the same multi-system branch but with a single worker — the prewarm still happens,
	// yet with no concurrency the result is race-free → trusted baseline. wc>=2 introduces real
	// concurrency, where each filtered system's worker-side query_cache lookup must hit the
	// prewarmed {require, exclude} key. A mismatch would race the mutable cache or return torn
	// archetype_indices → a digest that differs from the baseline.
	std::size_t const entities = 600;
	int const ticks = 10;
	int64_t const baseline = run_multi_and_digest(1, entities, ticks);
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		CHECK(run_multi_and_digest(wc, entities, ticks) == baseline);
	}
}

TEST_CASE("Multi-system filtered parallel stage applies each filter independently",
          "[ecs][SystemFilters]") {
	World world;
	world.set_ecs_worker_count(4);
	std::vector<EntityID> ids = seed_multi(world, 1); // exactly P, Q, R, S
	REQUIRE(ids.size() == 4u);

	world.register_system<MfExcludeAThreaded>();
	world.register_system<MfExcludeBSerial>();
	world.register_system<MfPlainCThreaded>();
	world.register_system<MfExcludeDSerial>();
	world.tick_systems(Date {});

	EntityID const P = ids[0]; // no markers
	EntityID const Q = ids[1]; // Dead
	EntityID const R = ids[2]; // Other
	EntityID const S = ids[3]; // Dead + Other

	// A excludes Dead → writes P, R only (k=1: 0*31 + 7 + 1 = 8).
	CHECK(comp_v<MfA>(world, P) == 8);
	CHECK(comp_v<MfA>(world, Q) == 0);
	CHECK(comp_v<MfA>(world, R) == 8);
	CHECK(comp_v<MfA>(world, S) == 0);

	// B excludes Other → writes P, Q only (k=1: 0*13 + 3 + 1 = 4).
	CHECK(comp_v<MfB>(world, P) == 4);
	CHECK(comp_v<MfB>(world, Q) == 4);
	CHECK(comp_v<MfB>(world, R) == 0);
	CHECK(comp_v<MfB>(world, S) == 0);

	// C has no filter → writes all (k=1: 0*17 + 1 = 1).
	CHECK(comp_v<MfC>(world, P) == 1);
	CHECK(comp_v<MfC>(world, Q) == 1);
	CHECK(comp_v<MfC>(world, R) == 1);
	CHECK(comp_v<MfC>(world, S) == 1);

	// D excludes Dead → writes P, R only (k=1: 0*5 + 2 + 3 = 5).
	CHECK(comp_v<MfD>(world, P) == 5);
	CHECK(comp_v<MfD>(world, Q) == 0);
	CHECK(comp_v<MfD>(world, R) == 5);
	CHECK(comp_v<MfD>(world, S) == 0);
}

TEST_CASE("Filtered systems with disjoint writes share a stage (schedule_hash stable)",
          "[ecs][SystemFilters][determinism]") {
	uint64_t h1 = 0;
	uint64_t h2 = 0;
	{
		World w;
		w.register_system<MfExcludeAThreaded>();
		w.register_system<MfExcludeBSerial>();
		w.register_system<MfPlainCThreaded>();
		w.register_system<MfExcludeDSerial>();
		h1 = w.schedule_hash();
	}
	{
		World w;
		w.register_system<MfExcludeDSerial>();
		w.register_system<MfPlainCThreaded>();
		w.register_system<MfExcludeBSerial>();
		w.register_system<MfExcludeAThreaded>();
		h2 = w.schedule_hash();
	}
	CHECK(h1 != 0u);
	CHECK(h1 == h2); // exclusion injects no access → no phantom conflict → one stage, any order
}

// ============================================================================
// Real-concurrency check: two FILTERED threaded systems with disjoint writes must actually run
// their tick bodies concurrently (peak >= 2). Guards against the determinism gate passing
// vacuously because the stage silently serialized. Mirrors MultiSystemMixedStage Test 9.
// ============================================================================

namespace sfw_concurrency {
	std::atomic<int> g_active { 0 };
	std::atomic<int> g_peak { 0 };

	void enter_tick_and_observe() {
		int const now = g_active.fetch_add(1) + 1;
		int prev = g_peak.load();
		while (now > prev && !g_peak.compare_exchange_weak(prev, now)) {
			// retry
		}
		volatile int spin = 0;
		for (int i = 0; i < 2000; ++i) {
			spin += i;
		}
		(void) spin;
		g_active.fetch_sub(1);
	}

	struct SfwConcA : SystemThreaded<SfwConcA> {
		using Filters = Filter<Without<MfDead>>;
		void tick(TickContext const& /*ctx*/, MfSeed const& /*s*/, MfA& a) {
			enter_tick_and_observe();
			a.v = 1;
		}
	};

	struct SfwConcC : SystemThreaded<SfwConcC> {
		void tick(TickContext const& /*ctx*/, MfSeed const& /*s*/, MfC& c) {
			enter_tick_and_observe();
			c.v = 1;
		}
	};
}
ECS_SYSTEM(sfw_concurrency::SfwConcA)
ECS_SYSTEM(sfw_concurrency::SfwConcC)

TEST_CASE("Filtered systems in a multi-system stage run bodies concurrently",
          "[ecs][SystemFilters]") {
	using namespace sfw_concurrency;

	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 2000; // many chunks → many work items → ample overlap opportunity
	for (std::size_t i = 0; i < N; ++i) {
		// No MfDead → SfwConcA (Without<MfDead>) matches every row, same as SfwConcC.
		world.create_entity(MfSeed { static_cast<int64_t>(i) }, MfA {}, MfC {});
	}

	g_active.store(0);
	g_peak.store(0);

	world.register_system<SfwConcA>();
	world.register_system<SfwConcC>();
	world.tick_systems(Date {});

	CHECK(g_peak.load() >= 2);
}

// ============================================================================
// Disjoint-iteration conflict override (provably_disjoint): two systems that WRITE THE SAME
// component but iterate provably-disjoint archetypes co-schedule into one stage. This is the
// determinism gate for that dropped edge. A threaded writer requires DjGate; a plain writer
// writes DjShared `Without<DjGate>` — so the plain System<>'s worker-side `for_each` carries a
// NON-EMPTY exclude key while running concurrently with the threaded chunks, against the
// scheduler's prewarmed (require, exclude) cache. Each entity is written by exactly one of the
// two, so the post-tick state must be bit-identical for every worker_count.
// ============================================================================

namespace {
	struct DjShared { int64_t v = 0; }; // written by both systems, on disjoint entities
	struct DjGate { int64_t k = 0; };   // required by the threaded writer; excluded by the plain one
	struct DjOther { int64_t v = 0; };  // neutral archetype variety
}
ECS_COMPONENT(DjShared, "test_SystemFiltersWCI::DjShared")
ECS_COMPONENT(DjGate, "test_SystemFiltersWCI::DjGate")
ECS_COMPONENT(DjOther, "test_SystemFiltersWCI::DjOther")

namespace {
	// Threaded; writes DjShared on entities WITH DjGate (require = {DjShared, DjGate}).
	struct DjThreadedWithGate : SystemThreaded<DjThreadedWithGate> {
		void tick(TickContext const& /*ctx*/, DjShared& s, DjGate const& g) {
			s.v = s.v * 31 + g.k * 7 + 1;
		}
	};

	// Plain; writes DjShared on entities WITHOUT DjGate (require = {DjShared}, exclude = {DjGate}).
	struct DjPlainWithoutGate : System<DjPlainWithoutGate> {
		using Filters = Filter<Without<DjGate>>;
		void tick(TickContext const& /*ctx*/, DjShared& s) {
			s.v = s.v * 13 + 3;
		}
	};

	// Four archetypes; gated rows go to the threaded writer, ungated rows to the plain writer.
	std::vector<EntityID> seed_disjoint(World& world, std::size_t n) {
		std::vector<EntityID> ids;
		ids.reserve(n * 4);
		for (std::size_t i = 0; i < n; ++i) {
			int64_t const k = static_cast<int64_t>(i + 1);
			ids.push_back(world.create_entity(DjShared { 0 }, DjGate { k }));                 // gated
			ids.push_back(world.create_entity(DjShared { 0 }));                               // ungated
			ids.push_back(world.create_entity(DjShared { 0 }, DjGate { k }, DjOther { k }));  // gated + variety
			ids.push_back(world.create_entity(DjShared { 0 }, DjOther { k }));                // ungated + variety
		}
		return ids;
	}

	int64_t run_disjoint_and_digest(uint32_t worker_count, std::size_t n, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count);
		std::vector<EntityID> ids = seed_disjoint(world, n);

		world.register_system<DjThreadedWithGate>();
		world.register_system<DjPlainWithoutGate>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		int64_t digest = 0;
		for (EntityID const& id : ids) {
			DjShared const* s = world.get_component<DjShared>(id);
			digest = digest * 1000003 + (s != nullptr ? s->v : 0);
		}
		return digest;
	}
}
ECS_SYSTEM(DjThreadedWithGate)
ECS_SYSTEM(DjPlainWithoutGate)

TEST_CASE("Disjoint-filter same-component writers: digest is identical across worker counts",
          "[ecs][SystemFilters][determinism][WorkerCountInvariance]") {
	// Guard against a vacuous pass: confirm the two writers actually co-schedule into one
	// stage (the dropped-edge path). If they serialised, the gate below would still pass but
	// would no longer be testing concurrent same-component writes.
	{
		World probe;
		probe.register_system<DjThreadedWithGate>();
		probe.register_system<DjPlainWithoutGate>();
		REQUIRE(probe.debug_stage_count() == 1u);
	}

	std::size_t const entities = 400;
	int const ticks = 10;
	int64_t const baseline = run_disjoint_and_digest(1, entities, ticks);
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		CHECK(run_disjoint_and_digest(wc, entities, ticks) == baseline);
	}
}
