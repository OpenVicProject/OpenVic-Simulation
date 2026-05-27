#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/QueryFilter.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemPhase.hpp"
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

// === Phase-anchor sugar (SystemPhase.hpp): ECS_PHASE_ANCHOR / ECS_IN_PHASE. ===
//
// The macros must be PURE SUGAR: a schedule built from macro-generated anchors/members
// must be identical — equal schedule_hash, equal stage layout — to one built from
// hand-written PhaseAnchorSystem subclasses and hand-written declared_run_after /
// declared_run_before statics carrying the same SystemName identities.
//
// Manual-only check (not automatable): a system using ECS_IN_PHASE that ALSO hand-writes
// declared_run_after fails to compile — MSVC C2556/C2371 in-class member redefinition,
// never silent shadowing. Verified by hand during implementation; in-class redefinition
// is a hard error outside any SFINAE-probeable context.

namespace {
	struct PhaseTagA { int64_t v = 0; };
	struct PhaseTagB { int64_t v = 0; };
	struct PhaseTagC { int64_t v = 0; };
}
ECS_COMPONENT(PhaseTagA, "test_SystemPhase::TagA")
ECS_COMPONENT(PhaseTagB, "test_SystemPhase::TagB")
ECS_COMPONENT(PhaseTagC, "test_SystemPhase::TagC")

// --- Macro-built fixture: 4 anchors fencing 3 phases, 2 member systems per phase. ---
//
// ECS_PHASE_ANCHOR expands ECS_SYSTEM, so these live at global namespace scope (names
// must be TU-unique across the test binary — hence the PhaseSugar prefix).
ECS_PHASE_ANCHOR_FIRST(PhaseSugarP0)
ECS_PHASE_ANCHOR(PhaseSugarP1, PhaseSugarP0)
ECS_PHASE_ANCHOR(PhaseSugarP2, PhaseSugarP1)
ECS_PHASE_ANCHOR(PhaseSugarP3, PhaseSugarP2)

namespace {
	// Set before a tick to record member execution order (anchors must contribute nothing).
	std::vector<int>* g_phase_log = nullptr;

	void phase_log(int member_id) {
		if (g_phase_log != nullptr) {
			g_phase_log->push_back(member_id);
		}
	}

	struct SugarP0SysA;
	struct SugarP0SysB;
	struct SugarP1SysA;
	struct SugarP1SysB;
	struct SugarP2SysA;
	struct SugarP2SysB;
}
// ECS_SYSTEM identities must precede the systems that reference them via system_type_id_of<>
// (e.g. SugarP1SysB's ECS_IN_PHASE extra names SugarP1SysA) — see the note in
// SystemScheduler_DAG.cpp. Forward-declare, specialise, then define.
ECS_SYSTEM(SugarP0SysA)
ECS_SYSTEM(SugarP0SysB)
ECS_SYSTEM(SugarP1SysA)
ECS_SYSTEM(SugarP1SysB)
ECS_SYSTEM(SugarP2SysA)
ECS_SYSTEM(SugarP2SysB)

namespace {
	// Phase 0 (between P0 and P1): disjoint components — co-staging is legal.
	struct SugarP0SysA : System<SugarP0SysA> {
		ECS_IN_PHASE(PhaseSugarP0, PhaseSugarP1)
		void tick(TickContext const& /*ctx*/, PhaseTagA& /*a*/) {
			phase_log(1);
		}
	};
	struct SugarP0SysB : System<SugarP0SysB> {
		ECS_IN_PHASE(PhaseSugarP0, PhaseSugarP1)
		void tick(TickContext const& /*ctx*/, PhaseTagB& /*b*/) {
			phase_log(2);
		}
	};

	// Phase 1 (between P1 and P2): disjoint components, PLUS a variadic intra-phase edge
	// (SysB after SysA) — the edge is the ONLY thing ordering them.
	struct SugarP1SysA : System<SugarP1SysA> {
		ECS_IN_PHASE(PhaseSugarP1, PhaseSugarP2)
		void tick(TickContext const& /*ctx*/, PhaseTagA& /*a*/) {
			phase_log(3);
		}
	};
	struct SugarP1SysB : System<SugarP1SysB> {
		ECS_IN_PHASE(PhaseSugarP1, PhaseSugarP2, SugarP1SysA)
		void tick(TickContext const& /*ctx*/, PhaseTagB& /*b*/) {
			phase_log(4);
		}
	};

	// Phase 2 (between P2 and P3).
	struct SugarP2SysA : System<SugarP2SysA> {
		ECS_IN_PHASE(PhaseSugarP2, PhaseSugarP3)
		void tick(TickContext const& /*ctx*/, PhaseTagA& /*a*/) {
			phase_log(5);
		}
	};
	struct SugarP2SysB : System<SugarP2SysB> {
		ECS_IN_PHASE(PhaseSugarP2, PhaseSugarP3)
		void tick(TickContext const& /*ctx*/, PhaseTagB& /*b*/) {
			phase_log(6);
		}
	};
}

// --- Hand-written twin fixture. ---
//
// Distinct C++ types whose SystemName specialisations carry the SAME strings as the macro
// set, with anchors hand-written as PhaseAnchorSystem subclasses and all edges hand-written
// as declared_run_after / declared_run_before arrays. Same name strings -> same
// system_type_id_t -> same edges/access -> the schedules must hash identically. (Same-string
// SystemName specialisations for distinct types are fine as long as the two sets are never
// registered in the same World.)
namespace {
	struct HandPhaseP0;
	struct HandPhaseP1;
	struct HandPhaseP2;
	struct HandPhaseP3;
	struct HandP0SysA;
	struct HandP0SysB;
	struct HandP1SysA;
	struct HandP1SysB;
	struct HandP2SysA;
	struct HandP2SysB;
}
// The SystemName specialisations must be complete before the definitions whose
// declared_run_after / declared_run_before reference these types via system_type_id_of<> (see
// the note in SystemScheduler_DAG.cpp). Specialise up front against the forward declarations.
namespace OpenVic::ecs {
	template<> struct SystemName<HandPhaseP0> { static constexpr std::string_view value = "PhaseSugarP0"; };
	template<> struct SystemName<HandPhaseP1> { static constexpr std::string_view value = "PhaseSugarP1"; };
	template<> struct SystemName<HandPhaseP2> { static constexpr std::string_view value = "PhaseSugarP2"; };
	template<> struct SystemName<HandPhaseP3> { static constexpr std::string_view value = "PhaseSugarP3"; };
	template<> struct SystemName<HandP0SysA> { static constexpr std::string_view value = "SugarP0SysA"; };
	template<> struct SystemName<HandP0SysB> { static constexpr std::string_view value = "SugarP0SysB"; };
	template<> struct SystemName<HandP1SysA> { static constexpr std::string_view value = "SugarP1SysA"; };
	template<> struct SystemName<HandP1SysB> { static constexpr std::string_view value = "SugarP1SysB"; };
	template<> struct SystemName<HandP2SysA> { static constexpr std::string_view value = "SugarP2SysA"; };
	template<> struct SystemName<HandP2SysB> { static constexpr std::string_view value = "SugarP2SysB"; };
}
namespace {
	struct HandPhaseP0 : PhaseAnchorSystem<HandPhaseP0> {};
	struct HandPhaseP1 : PhaseAnchorSystem<HandPhaseP1> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP0>() };
		}
	};
	struct HandPhaseP2 : PhaseAnchorSystem<HandPhaseP2> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP1>() };
		}
	};
	struct HandPhaseP3 : PhaseAnchorSystem<HandPhaseP3> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP2>() };
		}
	};

	struct HandP0SysA : System<HandP0SysA> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP0>() };
		}
		static constexpr std::array<system_type_id_t, 1> declared_run_before() {
			return { system_type_id_of<HandPhaseP1>() };
		}
		void tick(TickContext const& /*ctx*/, PhaseTagA& /*a*/) {}
	};
	struct HandP0SysB : System<HandP0SysB> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP0>() };
		}
		static constexpr std::array<system_type_id_t, 1> declared_run_before() {
			return { system_type_id_of<HandPhaseP1>() };
		}
		void tick(TickContext const& /*ctx*/, PhaseTagB& /*b*/) {}
	};
	struct HandP1SysA : System<HandP1SysA> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP1>() };
		}
		static constexpr std::array<system_type_id_t, 1> declared_run_before() {
			return { system_type_id_of<HandPhaseP2>() };
		}
		void tick(TickContext const& /*ctx*/, PhaseTagA& /*a*/) {}
	};
	struct HandP1SysB : System<HandP1SysB> {
		// Twin of the variadic form: prev anchor + intra-phase extra, hand-appended.
		static constexpr std::array<system_type_id_t, 2> declared_run_after() {
			return { system_type_id_of<HandPhaseP1>(), system_type_id_of<HandP1SysA>() };
		}
		static constexpr std::array<system_type_id_t, 1> declared_run_before() {
			return { system_type_id_of<HandPhaseP2>() };
		}
		void tick(TickContext const& /*ctx*/, PhaseTagB& /*b*/) {}
	};
	struct HandP2SysA : System<HandP2SysA> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP2>() };
		}
		static constexpr std::array<system_type_id_t, 1> declared_run_before() {
			return { system_type_id_of<HandPhaseP3>() };
		}
		void tick(TickContext const& /*ctx*/, PhaseTagA& /*a*/) {}
	};
	struct HandP2SysB : System<HandP2SysB> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<HandPhaseP2>() };
		}
		static constexpr std::array<system_type_id_t, 1> declared_run_before() {
			return { system_type_id_of<HandPhaseP3>() };
		}
		void tick(TickContext const& /*ctx*/, PhaseTagB& /*b*/) {}
	};
}

// --- Feature-composition fixture (Filters + SystemThreaded inside a phase). ---
namespace {
	struct SugarThreadedMember : SystemThreaded<SugarThreadedMember> {
		using Filters = Filter<Without<PhaseTagC>>;
		ECS_IN_PHASE(PhaseSugarP1, PhaseSugarP2)
		void tick(TickContext const& /*ctx*/, PhaseTagA& a) {
			a.v += 1;
		}
	};

	// --- Intra-phase conflict fixture: both write PhaseTagA, no declared edge. ---
	struct SugarConflictSysA : System<SugarConflictSysA> {
		ECS_IN_PHASE(PhaseSugarP1, PhaseSugarP2)
		void tick(TickContext const& /*ctx*/, PhaseTagA& a) {
			a.v += 1;
		}
	};
	struct SugarConflictSysB : System<SugarConflictSysB> {
		ECS_IN_PHASE(PhaseSugarP1, PhaseSugarP2)
		void tick(TickContext const& /*ctx*/, PhaseTagA& a) {
			a.v += 10;
		}
	};
}
ECS_SYSTEM(SugarThreadedMember)
ECS_SYSTEM(SugarConflictSysA)
ECS_SYSTEM(SugarConflictSysB)

namespace {
	void register_macro_set(World& world) {
		world.register_system<PhaseSugarP0>();
		world.register_system<PhaseSugarP1>();
		world.register_system<PhaseSugarP2>();
		world.register_system<PhaseSugarP3>();
		world.register_system<SugarP0SysA>();
		world.register_system<SugarP0SysB>();
		world.register_system<SugarP1SysA>();
		world.register_system<SugarP1SysB>();
		world.register_system<SugarP2SysA>();
		world.register_system<SugarP2SysB>();
	}

	std::size_t stage_of(World& world, system_type_id_t type_id) {
		return world.debug_stage_index_of(type_id);
	}
}

TEST_CASE("Phase members land strictly between their anchors", "[ecs][SystemPhase]") {
	World world;
	register_macro_set(world);

	std::size_t const p0 = stage_of(world, system_type_id_of<PhaseSugarP0>());
	std::size_t const p1 = stage_of(world, system_type_id_of<PhaseSugarP1>());
	std::size_t const p2 = stage_of(world, system_type_id_of<PhaseSugarP2>());
	std::size_t const p3 = stage_of(world, system_type_id_of<PhaseSugarP3>());

	// The anchor chain itself is ordered.
	CHECK(p0 < p1);
	CHECK(p1 < p2);
	CHECK(p2 < p3);

	// Every member sits strictly inside its own phase's fence. (Relative comparisons
	// only — unrelated systems may legally co-stage with an anchor at the same depth.)
	std::size_t const p0_sys_a = stage_of(world, system_type_id_of<SugarP0SysA>());
	std::size_t const p0_sys_b = stage_of(world, system_type_id_of<SugarP0SysB>());
	CHECK(p0 < p0_sys_a); CHECK(p0_sys_a < p1);
	CHECK(p0 < p0_sys_b); CHECK(p0_sys_b < p1);

	std::size_t const p1_sys_a = stage_of(world, system_type_id_of<SugarP1SysA>());
	std::size_t const p1_sys_b = stage_of(world, system_type_id_of<SugarP1SysB>());
	CHECK(p1 < p1_sys_a); CHECK(p1_sys_a < p2);
	CHECK(p1 < p1_sys_b); CHECK(p1_sys_b < p2);

	std::size_t const p2_sys_a = stage_of(world, system_type_id_of<SugarP2SysA>());
	std::size_t const p2_sys_b = stage_of(world, system_type_id_of<SugarP2SysB>());
	CHECK(p2 < p2_sys_a); CHECK(p2_sys_a < p3);
	CHECK(p2 < p2_sys_b); CHECK(p2_sys_b < p3);
}

TEST_CASE("Macro-built schedule hashes identically to the hand-written form",
          "[ecs][SystemPhase][Hash]") {
	World macro_world;
	register_macro_set(macro_world);

	World hand_world;
	hand_world.register_system<HandPhaseP0>();
	hand_world.register_system<HandPhaseP1>();
	hand_world.register_system<HandPhaseP2>();
	hand_world.register_system<HandPhaseP3>();
	hand_world.register_system<HandP0SysA>();
	hand_world.register_system<HandP0SysB>();
	hand_world.register_system<HandP1SysA>();
	hand_world.register_system<HandP1SysB>();
	hand_world.register_system<HandP2SysA>();
	hand_world.register_system<HandP2SysB>();

	CHECK(macro_world.schedule_hash() != 0);
	CHECK(macro_world.schedule_hash() == hand_world.schedule_hash());
	CHECK(macro_world.debug_stage_count() == hand_world.debug_stage_count());
}

TEST_CASE("Variadic ECS_IN_PHASE extras order systems within a phase",
          "[ecs][SystemPhase]") {
	World world;
	register_macro_set(world);

	// SysA / SysB touch disjoint components — the appended run_after extra is the ONLY
	// thing that can order them.
	std::size_t const p1 = stage_of(world, system_type_id_of<PhaseSugarP1>());
	std::size_t const p2 = stage_of(world, system_type_id_of<PhaseSugarP2>());
	std::size_t const sys_a = stage_of(world, system_type_id_of<SugarP1SysA>());
	std::size_t const sys_b = stage_of(world, system_type_id_of<SugarP1SysB>());
	CHECK(sys_a < sys_b);
	CHECK(p1 < sys_a);
	CHECK(sys_b < p2);
}

TEST_CASE("Anchor/member registration order does not affect schedule_hash",
          "[ecs][SystemPhase][Hash][determinism]") {
	World forward;
	register_macro_set(forward);

	// Shuffled: members first, anchors last, phases interleaved.
	World shuffled;
	shuffled.register_system<SugarP2SysB>();
	shuffled.register_system<SugarP0SysA>();
	shuffled.register_system<SugarP1SysB>();
	shuffled.register_system<PhaseSugarP2>();
	shuffled.register_system<SugarP1SysA>();
	shuffled.register_system<PhaseSugarP0>();
	shuffled.register_system<SugarP2SysA>();
	shuffled.register_system<PhaseSugarP3>();
	shuffled.register_system<SugarP0SysB>();
	shuffled.register_system<PhaseSugarP1>();

	CHECK(forward.schedule_hash() != 0);
	CHECK(forward.schedule_hash() == shuffled.schedule_hash());
}

TEST_CASE("Members execute in phase order; anchors contribute nothing",
          "[ecs][SystemPhase]") {
	std::vector<int> log;
	g_phase_log = &log;

	World world;
	// Serial mode: disjoint same-phase members co-stage, and the shared log vector must
	// not be written from concurrent workers.
	world.set_serial_mode(true);
	world.create_entity(PhaseTagA {});
	world.create_entity(PhaseTagB {});
	register_macro_set(world);

	world.tick_systems(Date {});
	g_phase_log = nullptr;

	// One PhaseTagA row + one PhaseTagB row -> exactly one log entry per member system;
	// the four anchors add none.
	REQUIRE(log.size() == 6u);

	// Phase-0 members (1, 2) before phase-1 members (3, 4) before phase-2 members (5, 6);
	// within phase 1 the variadic edge forces 3 before 4. Same-phase disjoint members may
	// co-stage, so only the partial order is asserted.
	std::array<std::size_t, 7> pos {};
	for (std::size_t i = 0; i < log.size(); ++i) {
		pos[static_cast<std::size_t>(log[i])] = i;
	}
	CHECK(pos[1] < pos[3]); CHECK(pos[1] < pos[4]);
	CHECK(pos[2] < pos[3]); CHECK(pos[2] < pos[4]);
	CHECK(pos[3] < pos[4]); // intra-phase variadic edge
	CHECK(pos[3] < pos[5]); CHECK(pos[3] < pos[6]);
	CHECK(pos[4] < pos[5]); CHECK(pos[4] < pos[6]);
}

TEST_CASE("SystemThreaded member with Filters works unchanged inside a phase",
          "[ecs][SystemPhase]") {
	World world;
	EntityID const plain = world.create_entity(PhaseTagA { 0 });
	EntityID const filtered = world.create_entity(PhaseTagA { 0 }, PhaseTagC { 0 });

	world.register_system<PhaseSugarP1>();
	world.register_system<PhaseSugarP2>();
	world.register_system<SugarThreadedMember>();

	world.tick_systems(Date {});

	PhaseTagA const* plain_a = world.get_component<PhaseTagA>(plain);
	PhaseTagA const* filtered_a = world.get_component<PhaseTagA>(filtered);
	REQUIRE(plain_a != nullptr);
	REQUIRE(filtered_a != nullptr);
	CHECK(plain_a->v == 1);    // {A} — visited
	CHECK(filtered_a->v == 0); // {A, C} — excluded by Filters

	std::size_t const p1 = stage_of(world, system_type_id_of<PhaseSugarP1>());
	std::size_t const p2 = stage_of(world, system_type_id_of<PhaseSugarP2>());
	std::size_t const member = stage_of(world, system_type_id_of<SugarThreadedMember>());
	CHECK(p1 < member);
	CHECK(member < p2);
}

TEST_CASE("Same-phase write conflict still auto-serialises between the anchors",
          "[ecs][SystemPhase]") {
	World world;
	world.create_entity(PhaseTagA { 0 });
	world.register_system<PhaseSugarP1>();
	world.register_system<PhaseSugarP2>();
	world.register_system<SugarConflictSysA>();
	world.register_system<SugarConflictSysB>();

	// Both write PhaseTagA with no declared edge: anchors add ordering edges only, never
	// remove conflict edges, so the pair must still be auto-serialised into two stages —
	// both strictly inside the phase fence.
	std::size_t const p1 = stage_of(world, system_type_id_of<PhaseSugarP1>());
	std::size_t const p2 = stage_of(world, system_type_id_of<PhaseSugarP2>());
	std::size_t const sys_a = stage_of(world, system_type_id_of<SugarConflictSysA>());
	std::size_t const sys_b = stage_of(world, system_type_id_of<SugarConflictSysB>());
	CHECK(sys_a != sys_b);
	CHECK(p1 < sys_a); CHECK(sys_a < p2);
	CHECK(p1 < sys_b); CHECK(sys_b < p2);
}

TEST_CASE("Cross-phase writers of the same component occupy distinct stages",
          "[ecs][SystemPhase]") {
	World world;
	register_macro_set(world);

	// SugarP0SysA / SugarP1SysA / SugarP2SysA all write PhaseTagA from different phases —
	// the anchor path already orders them, and the conflict stays serialised.
	std::size_t const s0 = stage_of(world, system_type_id_of<SugarP0SysA>());
	std::size_t const s1 = stage_of(world, system_type_id_of<SugarP1SysA>());
	std::size_t const s2 = stage_of(world, system_type_id_of<SugarP2SysA>());
	CHECK(s0 < s1);
	CHECK(s1 < s2);
}

TEST_CASE("Anchors declare zero access and an empty iteration query", "[ecs][SystemPhase]") {
	CHECK(PhaseSugarP0::declared_access().size() == 0u);
	CHECK(PhaseSugarP1::compute_tick_query_require_ids().empty());
	CHECK(PhaseSugarP1::compute_tick_query_exclude_ids().empty());
	CHECK(PhaseSugarP0::declared_run_after().size() == 0u); // first phase: no predecessor
	REQUIRE(PhaseSugarP1::declared_run_after().size() == 1u);
	CHECK(PhaseSugarP1::declared_run_after()[0] == system_type_id_of<PhaseSugarP0>());

	// The membership macro generates exactly the hand-written arrays.
	REQUIRE(SugarP1SysB::declared_run_after().size() == 2u);
	CHECK(SugarP1SysB::declared_run_after()[0] == system_type_id_of<PhaseSugarP1>());
	CHECK(SugarP1SysB::declared_run_after()[1] == system_type_id_of<SugarP1SysA>());
	REQUIRE(SugarP1SysB::declared_run_before().size() == 1u);
	CHECK(SugarP1SysB::declared_run_before()[0] == system_type_id_of<PhaseSugarP2>());
}
