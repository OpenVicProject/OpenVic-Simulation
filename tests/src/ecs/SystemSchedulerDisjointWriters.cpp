#include "openvic-simulation/ecs/ChunkSystem.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/QueryFilter.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemAccess.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === Filter-aware DAG conflict override (provably_disjoint). ===
// Two systems that write the SAME component are normally serialised. When their iteration
// archetypes are provably disjoint (one requires a component the other excludes) AND the
// shared write is archetype-iterated on both sides (in both require sets, never reached via
// extra_reads/extra_writes), the scheduler drops the conflict edge so they share a stage.
// These tests pin
// the structural decision (which stage each system lands in), the soundness guard for
// extra_reads, end-to-end correctness, and real concurrency. The cross-worker-count
// determinism gate for this path lives in SystemFiltersWorkerCountInvariance.cpp.

namespace {
	struct DwShared { int64_t v = 0; }; // written by both gate-partitioned systems
	struct DwGate { int64_t g = 0; };   // read by the with-gate system; excluded by the other
	struct DwOther { int64_t v = 0; };  // separate write target for the extra_reads soundness case
}
ECS_COMPONENT(DwShared, "test_DisjointWriters::Shared")
ECS_COMPONENT(DwGate, "test_DisjointWriters::Gate")
ECS_COMPONENT(DwOther, "test_DisjointWriters::Other")

namespace {
	// Writes DwShared and READS DwGate → require = {DwShared, DwGate}, no exclude.
	struct DwWriterWithGate : System<DwWriterWithGate> {
		void tick(TickContext const& /*ctx*/, DwShared& s, DwGate const& /*g*/) {
			s.v += 1;
		}
	};

	// Writes DwShared `Without<DwGate>` → require = {DwShared}, exclude = {DwGate}.
	// Disjoint from DwWriterWithGate via DwGate ∈ require(WithGate) ∩ exclude(WithoutGate).
	struct DwWriterWithoutGate : System<DwWriterWithoutGate> {
		using Filters = Filter<Without<DwGate>>;
		void tick(TickContext const& /*ctx*/, DwShared& s) {
			s.v += 100;
		}
	};

	// Two UNFILTERED writers of DwShared → require = {DwShared}, no exclude. They share the
	// {DwShared}-without-DwGate archetypes, so they are NOT provably disjoint → serialised.
	struct DwWriterPlainA : System<DwWriterPlainA> {
		void tick(TickContext const& /*ctx*/, DwShared& s) {
			s.v += 1;
		}
	};
	struct DwWriterPlainB : System<DwWriterPlainB> {
		void tick(TickContext const& /*ctx*/, DwShared& s) {
			s.v += 1;
		}
	};

	// Writes DwOther but CROSS-ARCHETYPE READS DwShared via extra_reads → access has DwShared
	// as Read, but DwShared ∉ require (require = {DwOther}). The extra-read can touch a DwShared
	// on ANY entity, so disjoint iteration cannot separate it from a DwShared writer.
	struct DwExtraReader : System<DwExtraReader> {
		static constexpr std::array<component_type_id_t, 1> extra_reads() {
			return { component_type_id_of<DwShared>() };
		}
		void tick(TickContext const& /*ctx*/, DwOther& o) {
			o.v += 1;
		}
	};

	// Identical to DwExtraReader EXCEPT it does not extra-read DwShared. The only difference is
	// the declaration — used to isolate that extra_reads is what forces serialisation below.
	struct DwPlainOtherWriter : System<DwPlainOtherWriter> {
		void tick(TickContext const& /*ctx*/, DwOther& o) {
			o.v += 1;
		}
	};

	// Writes DwShared `Without<DwOther>` → require = {DwShared}, exclude = {DwOther}. Iteration
	// is disjoint from any DwOther-requiring system (DwOther ∈ require(other) ∩ exclude(here)).
	struct DwWriterWithoutOther : System<DwWriterWithoutOther> {
		using Filters = Filter<Without<DwOther>>;
		void tick(TickContext const& /*ctx*/, DwShared& s) {
			s.v += 100;
		}
	};

	// HYBRID writer: DwShared is in the tick pack (require) AND in extra_writes — it writes
	// its own rows plus arbitrary other rows. Require membership alone would let the
	// disjoint-iteration override drop the edge to DwWriterWithoutGate (Rule 1 holds via
	// DwGate); the extra-list check must keep it.
	struct DwHybridWriter : System<DwHybridWriter> {
		static constexpr std::array<component_type_id_t, 1> extra_writes() {
			return { component_type_id_of<DwShared>() };
		}
		void tick(TickContext const& /*ctx*/, DwShared& s, DwGate const& /*g*/) {
			s.v += 1;
		}
	};

	// HYBRID reader: same shape on the read side — DwShared in the tick pack as Read AND in
	// extra_reads (own row plus arbitrary rows). Pins the pre-hardening unsoundness that
	// existed for extra_reads alone.
	struct DwHybridReader : System<DwHybridReader> {
		static constexpr std::array<component_type_id_t, 1> extra_reads() {
			return { component_type_id_of<DwShared>() };
		}
		void tick(TickContext const& /*ctx*/, DwShared const& /*s*/, DwGate const& /*g*/) {}
	};
}
ECS_SYSTEM(DwWriterWithGate)
ECS_SYSTEM(DwWriterWithoutGate)
ECS_SYSTEM(DwWriterPlainA)
ECS_SYSTEM(DwWriterPlainB)
ECS_SYSTEM(DwExtraReader)
ECS_SYSTEM(DwPlainOtherWriter)
ECS_SYSTEM(DwWriterWithoutOther)
ECS_SYSTEM(DwHybridWriter)
ECS_SYSTEM(DwHybridReader)

namespace {
	int64_t shared_value(World& world, EntityID id) {
		DwShared const* s = world.get_component<DwShared>(id);
		return s != nullptr ? s->v : -1;
	}
}

// === sorted_intersects primitive (unit) ===

TEST_CASE("sorted_intersects detects shared elements in sorted lists", "[ecs][SystemAccess]") {
	std::vector<component_type_id_t> const empty {};
	std::vector<component_type_id_t> const a { 1, 3, 5, 7 };
	std::vector<component_type_id_t> const disjoint { 2, 4, 6, 8 };
	std::vector<component_type_id_t> const overlap_first { 1, 100 };  // shares 1 (start of a)
	std::vector<component_type_id_t> const overlap_mid { 0, 5, 99 };  // shares 5 (middle of a)
	std::vector<component_type_id_t> const overlap_last { 7 };        // shares 7 (end of a)
	std::vector<component_type_id_t> const multi { 3, 5 };            // shares 3 and 5

	CHECK_FALSE(sorted_intersects(empty, empty));
	CHECK_FALSE(sorted_intersects(empty, a));
	CHECK_FALSE(sorted_intersects(a, empty));
	CHECK_FALSE(sorted_intersects(a, disjoint));
	CHECK(sorted_intersects(a, overlap_first));
	CHECK(sorted_intersects(a, overlap_mid));
	CHECK(sorted_intersects(a, overlap_last));
	CHECK(sorted_intersects(a, multi));
	CHECK(sorted_intersects(a, a)); // self-overlap
}

// === Structural decisions (which stage each system lands in) ===

TEST_CASE("Disjoint-filter writers of the same component share a stage",
          "[ecs][SystemScheduler][DisjointWriters]") {
	// Writes DwShared from both, but one requires DwGate and the other excludes it — provably
	// disjoint, so the conflict edge is dropped and they co-schedule into one stage.
	World world;
	world.register_system<DwWriterWithGate>();
	world.register_system<DwWriterWithoutGate>();

	std::size_t const stage_with = world.debug_stage_index_of(system_type_id_of<DwWriterWithGate>());
	std::size_t const stage_without =
		world.debug_stage_index_of(system_type_id_of<DwWriterWithoutGate>());

	CHECK(stage_with != SIZE_MAX);
	CHECK(stage_without != SIZE_MAX);
	CHECK(stage_with == stage_without);
	CHECK(world.debug_stage_count() == 1u);
}

TEST_CASE("Unfiltered writers of the same component stay in different stages",
          "[ecs][SystemScheduler][DisjointWriters]") {
	// No filters → not provably disjoint → the write/write conflict edge is kept → serialised.
	World world;
	world.register_system<DwWriterPlainA>();
	world.register_system<DwWriterPlainB>();

	CHECK(world.debug_stage_index_of(system_type_id_of<DwWriterPlainA>())
		!= world.debug_stage_index_of(system_type_id_of<DwWriterPlainB>()));
	CHECK(world.debug_stage_count() == 2u);
}

TEST_CASE("A one-sided filter does not prove disjointness", "[ecs][SystemScheduler][DisjointWriters]") {
	// WithoutGate excludes DwGate; PlainA has no filter. Both match {DwShared}-without-DwGate
	// archetypes → they CAN share an entity → edge kept → different stages.
	World world;
	world.register_system<DwWriterWithoutGate>();
	world.register_system<DwWriterPlainA>();

	CHECK(world.debug_stage_index_of(system_type_id_of<DwWriterWithoutGate>())
		!= world.debug_stage_index_of(system_type_id_of<DwWriterPlainA>()));
	CHECK(world.debug_stage_count() == 2u);
}

// === Soundness guard: a cross-archetype extra_read is NOT separated by disjoint iteration ===

TEST_CASE("extra_reads on the conflicting component keeps systems serialised",
          "[ecs][SystemScheduler][DisjointWriters]") {
	// DwExtraReader writes DwOther and extra-reads DwShared; DwWriterWithoutOther writes
	// DwShared and excludes DwOther. Their ITERATIONS are disjoint (DwOther required vs
	// excluded), but the conflict is on DwShared, which DwExtraReader reaches CROSS-ARCHETYPE
	// (DwShared ∉ its require). Disjoint iteration cannot separate that → edge must be kept.
	World world;
	world.register_system<DwExtraReader>();
	world.register_system<DwWriterWithoutOther>();

	CHECK(world.debug_stage_index_of(system_type_id_of<DwExtraReader>())
		!= world.debug_stage_index_of(system_type_id_of<DwWriterWithoutOther>()));
	CHECK(world.debug_stage_count() == 2u);
}

TEST_CASE("Without the extra_read the same pair has no conflict at all",
          "[ecs][SystemScheduler][DisjointWriters]") {
	// DwPlainOtherWriter is DwExtraReader minus the extra_reads<DwShared> declaration. It only
	// touches DwOther, which DwWriterWithoutOther never touches → no shared component → no edge
	// → one stage. Isolates that the extra_read (not the iteration shape) caused the
	// serialisation in the previous test.
	World world;
	world.register_system<DwPlainOtherWriter>();
	world.register_system<DwWriterWithoutOther>();

	CHECK(world.debug_stage_index_of(system_type_id_of<DwPlainOtherWriter>())
		== world.debug_stage_index_of(system_type_id_of<DwWriterWithoutOther>()));
	CHECK(world.debug_stage_count() == 1u);
}

TEST_CASE("extra_writes on an also-required component defeats the disjoint-iteration override",
          "[ecs][SystemScheduler][DisjointWriters]") {
	// DwHybridWriter requires {DwShared, DwGate} and ALSO declares extra_writes(DwShared);
	// DwWriterWithoutGate writes DwShared excluding DwGate. Rule 1 holds (DwGate required vs
	// excluded) and DwShared is in BOTH require sets — pre-hardening this pair co-scheduled,
	// letting the hybrid's cross-archetype write race the disjoint writer. The extra-list
	// check must keep the edge.
	World world;
	world.register_system<DwHybridWriter>();
	world.register_system<DwWriterWithoutGate>();

	CHECK(world.debug_stage_index_of(system_type_id_of<DwHybridWriter>())
		!= world.debug_stage_index_of(system_type_id_of<DwWriterWithoutGate>()));
	CHECK(world.debug_stage_count() == 2u);
}

TEST_CASE("extra_reads of an also-required component defeats the disjoint-iteration override",
          "[ecs][SystemScheduler][DisjointWriters]") {
	// Read-side twin of the previous test: DwShared is in DwHybridReader's tick pack (Read)
	// AND in its extra_reads, so the read reaches rows outside its iteration. The pre-existing
	// Rule 2 (require membership only) wrongly dropped this edge.
	World world;
	world.register_system<DwHybridReader>();
	world.register_system<DwWriterWithoutGate>();

	CHECK(world.debug_stage_index_of(system_type_id_of<DwHybridReader>())
		!= world.debug_stage_index_of(system_type_id_of<DwWriterWithoutGate>()));
	CHECK(world.debug_stage_count() == 2u);
}

TEST_CASE("Co-scheduled disjoint writers: schedule_hash is registration-order independent",
          "[ecs][SystemScheduler][DisjointWriters]") {
	uint64_t h1 = 0;
	uint64_t h2 = 0;
	{
		World w;
		w.register_system<DwWriterWithGate>();
		w.register_system<DwWriterWithoutGate>();
		h1 = w.schedule_hash();
	}
	{
		World w;
		w.register_system<DwWriterWithoutGate>();
		w.register_system<DwWriterWithGate>();
		h2 = w.schedule_hash();
	}
	CHECK(h1 != 0u);
	CHECK(h1 == h2); // one stage, order-independent
}

// === End-to-end correctness ===

TEST_CASE("Co-scheduled disjoint writers each write only their own entities",
          "[ecs][SystemScheduler][DisjointWriters]") {
	auto run = [](bool serial) {
		World world;
		world.set_ecs_worker_count(4);
		world.set_serial_mode(serial);

		std::vector<EntityID> gated;
		std::vector<EntityID> ungated;
		for (int i = 0; i < 200; ++i) {
			gated.push_back(world.create_entity(DwShared { 0 }, DwGate { 0 }));
			ungated.push_back(world.create_entity(DwShared { 0 }));
		}

		world.register_system<DwWriterWithGate>();    // +1 on entities WITH DwGate
		world.register_system<DwWriterWithoutGate>(); // +100 on entities WITHOUT DwGate
		world.tick_systems(Date {});

		for (EntityID const& id : gated) {
			CHECK(shared_value(world, id) == 1); // visited only by the with-gate system
		}
		for (EntityID const& id : ungated) {
			CHECK(shared_value(world, id) == 100); // visited only by the without-gate system
		}
	};

	run(/*serial=*/true);  // baseline
	run(/*serial=*/false); // co-scheduled parallel — must match the baseline
}

// === Real concurrency (guards against a vacuous pass where the stage silently serialises) ===

namespace dw_concurrency {
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

	struct DwConcWithGate : SystemThreaded<DwConcWithGate> {
		void tick(TickContext const& /*ctx*/, DwShared& s, DwGate const& /*g*/) {
			enter_tick_and_observe();
			s.v += 1;
		}
	};

	struct DwConcWithoutGate : SystemThreaded<DwConcWithoutGate> {
		using Filters = Filter<Without<DwGate>>;
		void tick(TickContext const& /*ctx*/, DwShared& s) {
			enter_tick_and_observe();
			s.v += 100;
		}
	};
}
ECS_SYSTEM(dw_concurrency::DwConcWithGate)
ECS_SYSTEM(dw_concurrency::DwConcWithoutGate)

TEST_CASE("Co-scheduled disjoint writers run their tick bodies concurrently",
          "[ecs][SystemScheduler][DisjointWriters]") {
	using namespace dw_concurrency;

	World world;
	world.set_ecs_worker_count(4);

	std::size_t const N = 2000; // many chunks on both sides → ample overlap opportunity
	for (std::size_t i = 0; i < N; ++i) {
		world.create_entity(DwShared { 0 }, DwGate { 0 }); // → DwConcWithGate
		world.create_entity(DwShared { 0 });               // → DwConcWithoutGate
	}

	g_active.store(0);
	g_peak.store(0);

	world.register_system<DwConcWithGate>();
	world.register_system<DwConcWithoutGate>();
	world.tick_systems(Date {});

	CHECK(g_peak.load() >= 2);
}
