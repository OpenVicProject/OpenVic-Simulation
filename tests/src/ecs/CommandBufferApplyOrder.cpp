#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstdint>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === Cross-system stage-barrier CommandBuffer apply order. ===
// Within one stage, pending buffers apply in the stage's deterministic emit order —
// ascending system_type_id_t, independent of registration order (SystemScheduler.cpp,
// Phase 5 priority queue keyed (depth, type_id)). SystemThreadedSpawn.cpp pins only the
// within-system per-chunk merge; no other test pins the cross-system half. Probe: both
// systems queue cmd.add_component<CbaoProbe>(probe, value) on the SAME pre-existing
// entity. apply() replaces an already-present component IN PLACE (destroy +
// move-construct, no refusal, no log), so the final value is exactly the LAST-applied
// buffer's value: the system with the GREATER type id wins.

namespace {
	struct CbaoIterA { int32_t v = 0; }; // iterated/written only by CbaoWriterA
	struct CbaoIterB { int32_t v = 0; }; // iterated/written only by CbaoWriterB
	struct CbaoProbe { int32_t v = 0; }; // on the probe entity; never in any tick signature
}
ECS_COMPONENT(CbaoIterA, "test_CmdBufApplyOrder::IterA")
ECS_COMPONENT(CbaoIterB, "test_CmdBufApplyOrder::IterB")
ECS_COMPONENT(CbaoProbe, "test_CmdBufApplyOrder::Probe")

namespace {
	constexpr int32_t CBAO_VALUE_A = 1;
	constexpr int32_t CBAO_VALUE_B = 2;

	// Set by the test before tick_systems, read by the tick bodies. Same pattern as
	// g_scheduler_dag_log in SystemScheduler_DAG.cpp — written on the main thread
	// before dispatch, read-only during the tick.
	EntityID g_cbao_probe_eid {};

	// Disjoint declared access ({CbaoIterA} vs {CbaoIterB}) → no conflict edge → the
	// two systems co-schedule into ONE stage. Each queues one replace of the SAME
	// probe component; the barrier apply order decides the winner.
	struct CbaoWriterA : System<CbaoWriterA> {
		void tick(TickContext const& ctx, CbaoIterA& a) {
			a.v += 1;
			ctx.cmd.add_component<CbaoProbe>(g_cbao_probe_eid, CbaoProbe { CBAO_VALUE_A });
		}
	};
	struct CbaoWriterB : System<CbaoWriterB> {
		void tick(TickContext const& ctx, CbaoIterB& b) {
			b.v += 1;
			ctx.cmd.add_component<CbaoProbe>(g_cbao_probe_eid, CbaoProbe { CBAO_VALUE_B });
		}
	};
}
ECS_SYSTEM(CbaoWriterA)
ECS_SYSTEM(CbaoWriterB)

TEST_CASE(
	"Stage-barrier buffers apply in ascending system_type_id_t, independent of registration order",
	"[ecs][CommandBuffer][SystemScheduler][determinism]"
) {
	constexpr system_type_id_t id_a = system_type_id_of<CbaoWriterA>();
	constexpr system_type_id_t id_b = system_type_id_of<CbaoWriterB>();
	REQUIRE(id_a != id_b); // distinct names → distinct FNV-1a ids (documents the assumption)

	// Later apply wins the in-place replace → the GREATER type id's value is final.
	// Derived, not hardcoded: this pins the rule, not the accidental hash order.
	int32_t const expected = (id_a < id_b) ? CBAO_VALUE_B : CBAO_VALUE_A;

	auto run = [&](bool serial, bool swapped) {
		World world;
		world.set_serial_mode(serial);
		world.set_ecs_worker_count(4);

		world.create_entity(CbaoIterA { 0 }); // one row each → exactly one queued op each
		world.create_entity(CbaoIterB { 0 });
		// Probe pre-carries CbaoProbe so BOTH applies take the in-place replace path.
		g_cbao_probe_eid = world.create_entity(CbaoProbe { 0 });

		if (swapped) {
			world.register_system<CbaoWriterB>();
			world.register_system<CbaoWriterA>();
		} else {
			world.register_system<CbaoWriterA>();
			world.register_system<CbaoWriterB>();
		}

		// Precondition: disjoint access really co-scheduled them into one stage —
		// otherwise the value assertion would vacuously pin inter-stage order instead.
		CHECK(world.debug_stage_index_of(id_a) == world.debug_stage_index_of(id_b));
		CHECK(world.debug_stage_count() == 1u);

		world.tick_systems(Date {});

		CbaoProbe const* probe = world.get_component<CbaoProbe>(g_cbao_probe_eid);
		REQUIRE(probe != nullptr);
		CHECK(probe->v == expected);
	};

	run(/*serial=*/true, /*swapped=*/false);
	run(/*serial=*/true, /*swapped=*/true); // registration order must not matter
	run(/*serial=*/false, /*swapped=*/false); // parallel multi-system stage branch
	run(/*serial=*/false, /*swapped=*/true);
}
