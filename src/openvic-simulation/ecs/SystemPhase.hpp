#pragma once

#include <array>

#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"

namespace OpenVic::ecs {

	// === Phase anchors — sugar over the one-DAG tick pipeline (ECS_SIM_ARCHITECTURE §7). ===
	//
	// The daily tick is ONE schedule: phases are zero-access "anchor" systems chained with
	// run_after, and a domain system joins a phase by declaring run_after(phase_anchor) +
	// run_before(next_phase_anchor). Anchors add ORDERING EDGES ONLY — their access set is
	// empty, so they never create (or remove) conflict edges, and independent domains still
	// overlap across phase lines when their access sets are disjoint. Everything here is
	// pure compile-time sugar: the expansion is exactly equivalent to hand-written
	// declared_run_after / declared_run_before statics — no scheduler involvement, no
	// runtime state, schedule_hash identical to the hand-written form.
	//
	// The core defines the MECHANISM only; phase names (p0_anchor … p10_anchor) belong to
	// game code.
	//
	// Pitfall (scheduler contract, see SystemScheduler::rebuild phase 1): run_after /
	// run_before ids that match no REGISTERED system are silently ignored. An anchor you
	// declared but forgot to world.register_system() makes every edge through it vanish —
	// register every anchor, in any order (the DAG sorts registration order out).

	// CRTP base for phase anchors: an empty System<> with zero component access and a
	// no-op execution. This — not the bare `System<Derived>` + empty tick — is the
	// canonical hand-written anchor form, for two load-bearing reasons:
	//
	//   - tick() with an empty component pack feeds the access-set machinery (empty access
	//     set, empty tick_query_require_ids) but would NOT compile through the default
	//     dispatch path: dispatch_serial_impl static_asserts on an empty pack, and
	//     register_system instantiates tick_all unconditionally.
	//   - tick_all() here SHADOWS System<Derived>::tick_all (the register_system thunk
	//     resolves tick_all on the derived type), so dispatch_serial is never instantiated
	//     and an anchor's "execution" is a no-op — no query is built (an empty require set
	//     would otherwise match EVERY archetype), no rows are touched.
	//
	// Derived declares its chain edge (or none, for the first phase):
	//
	//   struct p4_anchor : PhaseAnchorSystem<p4_anchor> {
	//       static constexpr std::array<system_type_id_t, 1> declared_run_after() {
	//           return { system_type_id_of<p3_anchor>() };
	//       }
	//   };
	//   ECS_SYSTEM(p4_anchor)
	//
	// — which is exactly what ECS_PHASE_ANCHOR expands to.
	template<typename Derived>
	struct PhaseAnchorSystem : System<Derived> {
		// Never invoked; exists so declared_access() derives an EMPTY access set.
		void tick(TickContext const&) {}
		// Shadows System<Derived>::tick_all — see the class comment.
		void tick_all(World&, TickContext const&) {}
	};

	namespace detail {
		// Variadic split the preprocessor can't do: ECS_IN_PHASE passes
		// <prev_anchor, __VA_ARGS__> where __VA_ARGS__ = next_anchor, extras... .
		// run_after takes prev + extras (NextAnchor is carried but unused);
		// run_before takes next (Extras are carried but unused).
		template<typename PrevAnchor, typename NextAnchor, typename... Extras>
		constexpr std::array<system_type_id_t, 1 + sizeof...(Extras)> phase_run_after_array() {
			return { system_type_id_of<PrevAnchor>(), system_type_id_of<Extras>()... };
		}

		template<typename NextAnchor, typename... Extras>
		constexpr std::array<system_type_id_t, 1> phase_run_before_array() {
			return { system_type_id_of<NextAnchor>() };
		}
	}
}

// Declares the FIRST phase anchor (no predecessor) and registers its ECS_SYSTEM identity.
// Must be invoked at global namespace scope (it expands ECS_SYSTEM, which opens
// `namespace OpenVic::ecs`); the anchor type lands in the enclosing (global) namespace
// and its stringified name must be globally unique — anchors are singular by nature.
#define ECS_PHASE_ANCHOR_FIRST(anchor_name) \
	struct anchor_name : ::OpenVic::ecs::PhaseAnchorSystem<anchor_name> {}; \
	ECS_SYSTEM(anchor_name)

// Declares a phase anchor chained after `prev_anchor` and registers its ECS_SYSTEM
// identity. Same global-namespace-scope constraint as ECS_PHASE_ANCHOR_FIRST.
#define ECS_PHASE_ANCHOR(anchor_name, prev_anchor) \
	struct anchor_name : ::OpenVic::ecs::PhaseAnchorSystem<anchor_name> { \
		static constexpr std::array<::OpenVic::ecs::system_type_id_t, 1> declared_run_after() { \
			return { ::OpenVic::ecs::system_type_id_of<prev_anchor>() }; \
		} \
	}; \
	ECS_SYSTEM(anchor_name)

// Phase membership, written INSIDE a system's class body:
//
//   struct rgo_produce_system : SystemThreaded<rgo_produce_system> {
//       ECS_IN_PHASE(p4_anchor, p5_anchor)               // after p4, before p5
//       void tick(TickContext const& ctx, ...);
//   };
//
// Expands to the declared_run_after / declared_run_before pair: run_after(prev_anchor),
// run_before(next_anchor). Extra intra-phase ordering composes variadically — every
// additional system type is APPENDED to the generated declared_run_after:
//
//       ECS_IN_PHASE(p4_anchor, p5_anchor, rgo_hire_system)
//
// Notes:
//   - Works identically on System<>, SystemThreaded<> and ChunkSystem<> (each base
//     carries its own empty-default statics; the generated members shadow them the same
//     way). Composes freely with `Filters`, extra_reads/extra_writes and should_run.
//   - A system using this macro CANNOT also hand-write declared_run_after or
//     declared_run_before: that is an in-class member redefinition — a hard compile
//     error (MSVC C2556/C2371: "overloaded function differs only by return type" /
//     "redefinition; different basic types"), never silent shadowing. Verified manually;
//     not SFINAE-probeable, so there is no automated test.
//   - A template-id extra dep (e.g. some_system<A, B>) would split on the comma at the
//     preprocessor level — use a type alias.
#define ECS_IN_PHASE(prev_anchor, /* next_anchor, extra_run_after_systems... */ ...) \
	static constexpr auto declared_run_after() { \
		return ::OpenVic::ecs::detail::phase_run_after_array<prev_anchor, __VA_ARGS__>(); \
	} \
	static constexpr auto declared_run_before() { \
		return ::OpenVic::ecs::detail::phase_run_before_array<__VA_ARGS__>(); \
	}
