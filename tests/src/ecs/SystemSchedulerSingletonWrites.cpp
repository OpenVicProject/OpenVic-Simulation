#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === Singleton access declarations (extra_writes / extra_reads). ===
// Singletons share component_type_id_t with components but never appear in a tick
// signature, so without a declaration they are invisible to the scheduler's access model —
// a co-scheduled writer/reader pair on the same singleton corrupts determinism silently.
// These tests pin the structural decisions (stage assignment from declared singleton
// access), the deterministic auto-orientation, the undeclared-hazard baseline, and the
// worker-count-invariance digest for a declared singleton pipeline.

namespace {
	struct SgwState { int64_t acc = 0; }; // the singleton — never attached to an entity
	struct SgwA { int64_t v = 0; };       // writer-side iterated rows
	struct SgwB { int64_t v = 0; };       // reader-side iterated rows
}
ECS_COMPONENT(SgwState, "test_SingletonWrites::State")
ECS_COMPONENT(SgwA, "test_SingletonWrites::A")
ECS_COMPONENT(SgwB, "test_SingletonWrites::B")

namespace {
	std::vector<int>* g_sgw_log = nullptr;

	// Iterates SgwA, folds each row into the singleton → declares extra_writes. The only
	// component shared with the readers below is the singleton id itself.
	struct SgwWriter : System<SgwWriter> {
		static constexpr std::array<component_type_id_t, 1> extra_writes() {
			return { component_type_id_of<SgwState>() };
		}
		void tick(TickContext const& ctx, SgwA& a) {
			SgwState* state = ctx.world.get_singleton<SgwState>();
			state->acc = (state->acc * 31 + a.v) % 1000003;
			a.v = (a.v * 7 + 3) % 1000003;
			if (g_sgw_log) g_sgw_log->push_back(1);
		}
	};

	// Second writer with a DIFFERENT iterated component — the singleton is the only
	// conflict between SgwWriter and SgwWriterTwo.
	struct SgwWriterTwo : System<SgwWriterTwo> {
		static constexpr std::array<component_type_id_t, 1> extra_writes() {
			return { component_type_id_of<SgwState>() };
		}
		void tick(TickContext const& ctx, SgwB& b) {
			SgwState* state = ctx.world.get_singleton<SgwState>();
			state->acc = (state->acc * 17 + b.v) % 1000003;
			b.v = (b.v * 5 + 1) % 1000003;
		}
	};

	// Reads the singleton into its own rows → declares extra_reads.
	struct SgwReader : System<SgwReader> {
		static constexpr std::array<component_type_id_t, 1> extra_reads() {
			return { component_type_id_of<SgwState>() };
		}
		void tick(TickContext const& ctx, SgwB& b) {
			SgwState const* state = ctx.world.get_singleton<SgwState>();
			b.v = (b.v * 13 + state->acc) % 1000003;
			if (g_sgw_log) g_sgw_log->push_back(2);
		}
	};

	// Second reader with a different iterated component — only the singleton (R/R) is shared
	// with SgwReader, so the pair must co-schedule.
	struct SgwReaderOnA : System<SgwReaderOnA> {
		static constexpr std::array<component_type_id_t, 1> extra_reads() {
			return { component_type_id_of<SgwState>() };
		}
		void tick(TickContext const& ctx, SgwA& a) {
			SgwState const* state = ctx.world.get_singleton<SgwState>();
			a.v = (a.v * 11 + state->acc) % 1000003;
		}
	};

	// The HAZARD pair: identical shapes minus the declarations. They share no declared
	// component at all, so the scheduler co-schedules them — this pins exactly the silent
	// corruption the declarations exist to prevent. (Their tick bodies deliberately do NOT
	// touch the singleton — running them co-scheduled must stay safe.)
	struct SgwUndeclaredWriter : System<SgwUndeclaredWriter> {
		void tick(TickContext const& /*ctx*/, SgwA& a) {
			a.v += 1;
		}
	};
	struct SgwUndeclaredReader : System<SgwUndeclaredReader> {
		void tick(TickContext const& /*ctx*/, SgwB& b) {
			b.v += 1;
		}
	};

	// Threaded reader for the worker-count-invariance digest — extra_reads on a singleton is
	// legal for SystemThreaded (concurrent reads of a stable pointer), unlike extra_writes.
	struct SgwReaderThreaded : SystemThreaded<SgwReaderThreaded> {
		static constexpr std::array<component_type_id_t, 1> extra_reads() {
			return { component_type_id_of<SgwState>() };
		}
		void tick(TickContext const& ctx, SgwB& b) {
			SgwState const* state = ctx.world.get_singleton<SgwState>();
			b.v = (b.v * 19 + state->acc) % 1000003;
		}
	};
}
ECS_SYSTEM(SgwWriter)
ECS_SYSTEM(SgwWriterTwo)
ECS_SYSTEM(SgwReader)
ECS_SYSTEM(SgwReaderOnA)
ECS_SYSTEM(SgwUndeclaredWriter)
ECS_SYSTEM(SgwUndeclaredReader)
ECS_SYSTEM(SgwReaderThreaded)

// === Structural decisions (stage assignment from declared singleton access) ===

TEST_CASE("Declared singleton writer and reader land in different stages",
          "[ecs][SystemScheduler][SingletonWrites]") {
	// The only shared id is the singleton (W via extra_writes vs R via extra_reads) —
	// singletons are never in a require set, so the disjoint-iteration override can never
	// drop this edge.
	World world;
	world.set_singleton<SgwState>();
	world.register_system<SgwWriter>();
	world.register_system<SgwReader>();

	std::size_t const stage_writer = world.debug_stage_index_of(system_type_id_of<SgwWriter>());
	std::size_t const stage_reader = world.debug_stage_index_of(system_type_id_of<SgwReader>());

	CHECK(stage_writer != SIZE_MAX);
	CHECK(stage_reader != SIZE_MAX);
	CHECK(stage_writer != stage_reader);
	CHECK(world.debug_stage_count() == 2u);
}

TEST_CASE("Two declared singleton writers are serialised",
          "[ecs][SystemScheduler][SingletonWrites]") {
	// Different iterated components (SgwA vs SgwB) — the singleton W/W is the only conflict.
	World world;
	world.set_singleton<SgwState>();
	world.register_system<SgwWriter>();
	world.register_system<SgwWriterTwo>();

	CHECK(world.debug_stage_index_of(system_type_id_of<SgwWriter>())
		!= world.debug_stage_index_of(system_type_id_of<SgwWriterTwo>()));
	CHECK(world.debug_stage_count() == 2u);
}

TEST_CASE("Two declared singleton readers share a stage",
          "[ecs][SystemScheduler][SingletonWrites]") {
	// R/R on the singleton is not a conflict; nothing else is shared → one stage.
	World world;
	world.set_singleton<SgwState>();
	world.register_system<SgwReader>();
	world.register_system<SgwReaderOnA>();

	CHECK(world.debug_stage_index_of(system_type_id_of<SgwReader>())
		== world.debug_stage_index_of(system_type_id_of<SgwReaderOnA>()));
	CHECK(world.debug_stage_count() == 1u);
}

TEST_CASE("Undeclared singleton access co-schedules — the hazard the declaration prevents",
          "[ecs][SystemScheduler][SingletonWrites]") {
	// Same iterated shapes as SgwWriter/SgwReader but with no extra_* declarations: the
	// scheduler sees no shared component and puts them in ONE stage. This is the silent
	// corruption vector for a real writer/reader pair — declarations are mandatory.
	World world;
	world.set_singleton<SgwState>();
	world.register_system<SgwUndeclaredWriter>();
	world.register_system<SgwUndeclaredReader>();

	CHECK(world.debug_stage_index_of(system_type_id_of<SgwUndeclaredWriter>())
		== world.debug_stage_index_of(system_type_id_of<SgwUndeclaredReader>()));
	CHECK(world.debug_stage_count() == 1u);
}

// === Deterministic auto-orientation ===

TEST_CASE("Singleton conflict auto-orientation is registration-order independent",
          "[ecs][SystemScheduler][SingletonWrites]") {
	std::vector<int> log_run1;
	std::vector<int> log_run2;
	uint64_t h1 = 0;
	uint64_t h2 = 0;

	{
		std::vector<int> log;
		g_sgw_log = &log;
		World world;
		world.set_singleton<SgwState>();
		world.create_entity(SgwA {});
		world.create_entity(SgwB {});
		world.register_system<SgwWriter>();
		world.register_system<SgwReader>();
		world.tick_systems(Date {});
		h1 = world.schedule_hash();
		log_run1 = log;
		g_sgw_log = nullptr;
	}
	{
		std::vector<int> log;
		g_sgw_log = &log;
		World world;
		world.set_singleton<SgwState>();
		world.create_entity(SgwA {});
		world.create_entity(SgwB {});
		world.register_system<SgwReader>();
		world.register_system<SgwWriter>();
		world.tick_systems(Date {});
		h2 = world.schedule_hash();
		log_run2 = log;
		g_sgw_log = nullptr;
	}

	REQUIRE(log_run1.size() == 2u);
	REQUIRE(log_run2.size() == 2u);
	CHECK(log_run1 == log_run2);
	CHECK(h1 != 0u);
	CHECK(h1 == h2);
}

// === Worker-count invariance (the determinism gate) ===

namespace {
	// Serial declared writer folds SgwA rows into the singleton; one serial and one
	// threaded declared reader consume the singleton into SgwB rows. All three conflict on
	// the singleton (and the readers on SgwB), so the schedule is fully serialised between
	// them — the digest must be bit-identical at every worker count.
	int64_t run_singleton_pipeline_and_digest(
		uint32_t worker_count, bool serial_mode, std::size_t per_side, int tick_count
	) {
		World world;
		world.set_ecs_worker_count(worker_count);
		world.set_serial_mode(serial_mode);

		// Create the singleton BEFORE the first tick — creating it mid-tick would mutate
		// the singleton map under concurrent readers.
		world.set_singleton<SgwState>();

		std::vector<EntityID> a_ids;
		std::vector<EntityID> b_ids;
		for (std::size_t i = 0; i < per_side; ++i) {
			a_ids.push_back(world.create_entity(SgwA { static_cast<int64_t>(i * 17 % 251 + 1) }));
			b_ids.push_back(world.create_entity(SgwB { static_cast<int64_t>(i * 13 % 239 + 1) }));
		}

		world.register_system<SgwWriter>();
		world.register_system<SgwReader>();
		world.register_system<SgwReaderThreaded>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		int64_t digest = 0;
		for (EntityID const& id : a_ids) {
			SgwA const* a = world.get_component<SgwA>(id);
			if (a != nullptr) {
				digest = digest * 1000003 + a->v;
			}
		}
		for (EntityID const& id : b_ids) {
			SgwB const* b = world.get_component<SgwB>(id);
			if (b != nullptr) {
				digest = digest * 1000003 + b->v;
			}
		}
		SgwState const* state = world.get_singleton<SgwState>();
		digest = digest * 1000003 + (state != nullptr ? state->acc : -1);
		return digest;
	}
}

TEST_CASE("Declared singleton pipeline: digest is identical across worker counts",
          "[ecs][determinism][WorkerCountInvariance][SingletonWrites]") {
	std::size_t const per_side = 200;
	int const ticks = 100;
	int64_t const baseline = run_singleton_pipeline_and_digest(1, /*serial_mode=*/false, per_side, ticks);

	// Forced-serial run must agree with the scheduled baseline.
	CHECK(run_singleton_pipeline_and_digest(1, /*serial_mode=*/true, per_side, ticks) == baseline);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		CHECK(run_singleton_pipeline_and_digest(wc, /*serial_mode=*/false, per_side, ticks) == baseline);
	}
}
