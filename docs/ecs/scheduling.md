# Scheduling and phases

The scheduler turns the set of systems you register on a `World` into one deterministic execution plan for the daily tick. You never order systems imperatively — you declare *what data each system touches* (its access set) and, where data alone is not enough, *explicit edges* (`declared_run_after` / `declared_run_before`, or the phase-anchor sugar). The scheduler builds a DAG from those declarations, serialises everything that conflicts, runs everything that provably cannot conflict in parallel, and produces a `schedule_hash` that multiplayer peers compare at session start.

This page covers how to control ordering and what the scheduler guarantees. For writing the systems themselves (tick signatures, `SystemAccess`, `should_run`) see systems.md; for what happens to `ctx.cmd` operations see command-buffer.md; for the parallelism rules inside a tick body see threading-and-reductions.md.

## The one-DAG tick pipeline

There is exactly **one schedule per `World`** — not one per "phase" or per subsystem. Every registered system is a node in a single DAG; phases (see below) are just zero-access anchor nodes inside that same DAG.

```cpp
template<typename S, typename... Args>
SystemHandle register_system(Args&&... args);

void unregister_system(SystemHandle handle);
void tick_systems(Date today);
void clear_systems();
```

(All on `World` — see src/openvic-simulation/ecs/World.hpp and world.md.)

Each `world.tick_systems(today)` call:

1. **Rebuilds the schedule if dirty.** `register_system` / `unregister_system` / `clear_systems` mark the schedule dirty; the rebuild happens lazily on the next `tick_systems` (or `schedule_hash()`) call. Register everything up front — re-registering per tick churns the schedule and `schedule_hash`. For cadence gating ("run only on the 1st of the month"), use `should_run` instead (below).
2. **Runs the stages in order.** A *stage* is a set of systems the scheduler has proven conflict-free; systems within a stage may execute concurrently on the `EcsThreadPool`.
3. **Applies command buffers at each stage barrier.** After a stage joins, every system's pending `CommandBuffer` is applied in the stage's deterministic topological emit order — within a stage, ascending `system_type_id_t` — a pure function of the registered system types, independent of registration order. The next stage observes all of the previous stage's deferred structural mutations (see command-buffer.md).

The schedule is **cross-machine deterministic given identical registrations**: stage layout depends only on the registered system *types* (their access sets and declared edges), never on registration order, entity contents, or live archetypes. Command-buffer apply order at the barrier is registration-order-independent too (ascending `system_type_id_t` within the stage), so the entire tick — stage layout and barrier apply order — is a pure function of the registered system types.

A declared dependency cycle (or a conflict pair that cannot be oriented without creating a cycle) is a hard error: the rebuild fails **silently** — `tick_systems` runs nothing until the cycle is fixed, and the observable signals are `schedule_hash() == 0` and `debug_stage_count() == 0` (no error is logged). Cycles are a bug in your declarations, not something to handle at runtime.

## Declaring explicit order: `declared_run_after` / `declared_run_before`

Override these statics on your system class (defaults on `System<Derived>` are empty):

```cpp
static constexpr std::array<system_type_id_t, N> declared_run_after();
static constexpr std::array<system_type_id_t, N> declared_run_before();
```

Each entry is a `system_type_id_of<OtherSystem>()`. `declared_run_after` makes this system run in a strictly later stage than each listed system; `declared_run_before` is the mirror image. Both directions exist so a system can insert itself relative to systems it does not own.

```cpp
#include "openvic-simulation/ecs/SystemImpl.hpp" // required in the TU that calls register_system
#include "openvic-simulation/ecs/World.hpp"

using namespace OpenVic::ecs;

namespace {
	struct ProductionOutput { int64_t goods = 0; };
	struct Stockpile { int64_t goods = 0; };
}
ECS_COMPONENT(ProductionOutput, "example::ProductionOutput")
ECS_COMPONENT(Stockpile, "example::Stockpile")

namespace {
	struct ProduceSystem : System<ProduceSystem> {
		void tick(TickContext const& /*ctx*/, ProductionOutput& out) {
			out.goods += 10;
		}
	};

	// Always runs in a later stage than ProduceSystem, even though the two
	// touch disjoint components and would otherwise co-stage.
	struct StockpileSystem : System<StockpileSystem> {
		static constexpr std::array<system_type_id_t, 1> declared_run_after() {
			return { system_type_id_of<ProduceSystem>() };
		}
		void tick(TickContext const& /*ctx*/, Stockpile& s) {
			s.goods += 1;
		}
	};
}
ECS_SYSTEM(ProduceSystem)
ECS_SYSTEM(StockpileSystem)

void register_tick(World& world) {
	// Registration order is irrelevant to the schedule — the DAG sorts it out.
	world.register_system<StockpileSystem>();
	world.register_system<ProduceSystem>();
}
```

Rules and contracts:

- **Edges to unregistered systems are silently ignored.** `run_after` / `run_before` ids that match no *registered* system simply vanish — there is no error. The classic footgun is a phase anchor you declared but forgot to `world.register_system()`: every edge through it disappears and your "ordered" pipeline quietly flattens. Register every system you reference, in any order.
- The statics must be `constexpr` and the listed types need `ECS_SYSTEM` identities (see src/openvic-simulation/ecs/SystemTypeID.hpp). A missing `ECS_SYSTEM(Type)` is a clear compile error ("incomplete type `SystemName<X>`").
- Edges only constrain *stage order*; they never weaken conflict detection. Two systems can be both edge-ordered and conflicting — that is fine and common.

## Automatic conflict resolution from declared access

You usually do not need explicit edges at all. The scheduler derives each system's access set from:

- the **tick parameter pack** — `C const&` is a Read, `C&` is a Write (see systems.md);
- **`extra_reads()`** — components/singletons read via `ctx.world` outside the iterated rows;
- **`extra_writes()`** — components/singletons written via `ctx.world` outside the iterated rows (forbidden on `SystemThreaded` — it would race with itself).

Two systems **conflict** when their access sets overlap on a component id with at least one Write (W/W or R/W). Read/Read never conflicts. Then:

- **Non-conflicting systems run stage-parallel.** No declaration needed — disjoint access sets co-stage automatically.
- **Conflicting systems are serialised** into different stages. If your declared edges (or transitive paths) already order the pair, that order is used. Otherwise the scheduler **auto-orients** the conflict edge deterministically: conflict pairs are processed in a fixed `(min type_id, max type_id)` order, the orientation that increases DAG depth least wins, and ties break toward the lower `system_type_id_t` running first. The result is identical on every machine and independent of registration order — but it is *arbitrary* from your point of view. **If you care which of two conflicting systems runs first, declare it** (an explicit edge or a phase); do not rely on the auto-orienter picking the direction you happened to observe.

### The disjoint-iteration override

One refinement: two systems that write the **same component** are still co-staged when the scheduler can *prove* they never iterate the same entity. Both conditions must hold:

1. **Disjoint iteration**: one system *requires* a component the other *excludes* via its `Filters` alias (see queries.md). No archetype can match both queries.
2. **The conflicting access is purely archetype-iterated**: the conflicting component is in *both* systems' tick parameter packs and in *neither* system's `extra_reads()` / `extra_writes()`. An extra-list access reaches rows outside the iteration, so disjoint iteration proves nothing and the edge is kept.

```cpp
namespace {
	// Writes Wage on pops that have an Employer.
	struct EmployedWageSystem : System<EmployedWageSystem> {
		void tick(TickContext const& /*ctx*/, Wage& w, Employer const& /*e*/) {
			w.amount += 1;
		}
	};

	// Writes Wage on pops WITHOUT an Employer — provably disjoint rows, so the
	// scheduler co-stages this with EmployedWageSystem despite the shared Write.
	struct UnemployedWageSystem : System<UnemployedWageSystem> {
		using Filters = Filter<Without<Employer>>;
		void tick(TickContext const& /*ctx*/, Wage& w) {
			w.amount += 100;
		}
	};
}
```

A one-sided filter is not enough (an unfiltered writer still overlaps the `Without` writer's rows), and the proof is type-level only — it never inspects live archetypes, so the schedule (and `schedule_hash`) stays a pure function of the registered types. The full behaviour is pinned in tests/src/ecs/SystemSchedulerDisjointWriters.cpp.

### Singletons: declare or desync

Singletons share `component_type_id_t` with components but never appear in a tick signature, so **undeclared singleton access is invisible to the scheduler**. A writer/reader pair on the same singleton with no `extra_writes()` / `extra_reads()` declarations shares no visible component — the scheduler happily co-stages them, and you get silent, probabilistic corruption that the worker-count determinism gate only sometimes catches. This is the single most dangerous scheduling mistake:

- `ctx.world.get_singleton<C>()` read → declare `extra_reads() = { component_type_id_of<C>() }`.
- Mutation through the returned pointer → declare `extra_writes()` — and that system must be a plain `System<>` (the `register_system` `static_assert` rejects `extra_writes` on `SystemThreaded`).
- Declared singleton W/W and R/W pairs serialise; declared R/R pairs co-stage — exactly like components.

See tests/src/ecs/SystemSchedulerSingletonWrites.cpp for the pinned hazard and the correct declared pipeline.

## What happens inside a stage

A stage may freely mix `System<>` and `SystemThreaded` (and `ChunkSystem`). The scheduler builds one flat work-item list for the whole stage — one item *per matched chunk* for every `SystemThreaded`, one *whole-tick* item for every plain `System<>` — and dispatches it through a single outer `parallel_for` on the `EcsThreadPool`. Consequences you should design around:

- **A plain `System<>` in a multi-system stage runs its entire tick on one worker thread.** Correctness is unaffected (the access model already guarantees no conflicts), but one heavy serial system can dominate a wide stage's wall time. Prefer `SystemThreaded` for per-row-heavy work; see threading-and-reductions.md.
- **Never call `EcsThreadPool::parallel_for` from inside a tick body.** The scheduler already owns the outer `parallel_for`; a nested one blocks the worker running your tick while the inner jobs sit unowned — with every worker in the same state, that is deadlock. Tick bodies are straight-line code; deterministic parallel folds go through `Reductions::*` (threading-and-reductions.md).
- **Stage barrier ordering is deterministic.** A `SystemThreaded`'s per-chunk command buffers merge in chunk-index ascending order, and each system's pending buffer applies at the barrier in the stage's deterministic order (ascending `system_type_id_t`) — identical at every worker count and independent of registration order.
- `world.set_serial_mode(true)` disables **stage-level (inter-system) parallelism only**: each stage's systems run one at a time, dispatched from the calling thread, bypassing the combined work-item `parallel_for`. A plain `System<>` then runs entirely on the calling thread — but a `SystemThreaded` **still parallelises over its own chunks** via `EcsThreadPool::parallel_for` (the pool keeps its default worker count regardless of serial mode). For fully single-threaded execution — debugging, unsynchronized test instrumentation — additionally call `set_ecs_worker_count(1)`; `parallel_for` runs inline on the calling thread when the pool has one worker. Serial mode exists to validate "parallel result == serial result" in tests; results must be bit-identical either way.

```cpp
void set_serial_mode(bool enabled);
void set_ecs_worker_count(uint32_t count); // call before the first tick_systems; 0 = hw default, capped at 16
```

### `should_run` and the schedule

A system may declare `static bool should_run(TickContext const&)` (full contract in systems.md). For scheduling purposes the rule is: **the skip is dispatch-time only**. A skipped system still occupies its stage, its conflict and ordering edges still constrain everything around it, and `schedule_hash` is untouched. The predicate is evaluated exactly once per tick, on the main thread, at the start of the system's stage — it always observes the previous stage's barrier state, never a co-staged system's mid-stage writes. This is why cadence gating belongs in `should_run` and not in per-tick `register_system` / `unregister_system` churn: skipping never perturbs the schedule, so it is lockstep-safe by construction.

## `schedule_hash`

```cpp
uint64_t schedule_hash(); // on World
```

An FNV-1a hash over the `(stage_index, system_type_id_t)` pairs of the built schedule (rebuilding first if dirty). **Multiplayer peers compute this at session-start handshake; a mismatch rejects the join** — it proves both machines will run the same systems in the same stage layout.

What it depends on (and only this):

- the set of registered system types (`ECS_SYSTEM` name strings — renaming a system changes its `system_type_id_t` and therefore the hash: a breaking change for anything that persists system ids);
- their declared access sets and `Filters` (which drive conflicts and the disjoint-iteration override);
- their declared edges (including phase membership).

What it does **not** depend on: registration order, `should_run` outcomes, worker count, serial mode, entity or archetype contents. Registration-order invariance is pinned in tests (tests/src/ecs/SystemScheduler_DAG.cpp, tests/src/ecs/SystemPhaseAnchors.cpp) and `should_run` invariance in tests/src/ecs/SystemShouldRun.cpp; the rest follows from the rebuild reading only registry metadata, never live world state. The hash proves matching *schedules*; matching *simulation results* additionally needs the determinism rules in determinism.md (fixed-point math, worker-count invariance, deterministic reductions).

For test assertions about stage layout, `World` also exposes introspection-only helpers — do not use them in game logic:

```cpp
std::size_t debug_stage_count();
std::size_t debug_stage_index_of(system_type_id_t type_id); // SIZE_MAX if not scheduled
```

## Phases

Phases give the tick a readable coarse structure ("all production before all consumption") without a second scheduling mechanism. A *phase anchor* is an ordinary registered system with an **empty access set and a no-op execution**: anchors are chained with `run_after`, and a domain system joins a phase by declaring `run_after(phase_anchor)` + `run_before(next_phase_anchor)`. Everything is compile-time sugar over the same one DAG — no scheduler involvement, no runtime state.

Two properties follow directly and are worth internalising:

- **Anchors add ordering edges only.** They never create or remove conflict edges. Two systems in the *same* phase that write the same component still auto-serialise into sub-stages between the anchors; two systems in *different* phases with disjoint access are still only ordered because the anchor chain orders them — independent domains overlap across phase lines exactly as far as their access sets allow.
- **The sugar is hash-equivalent to hand-written edges.** A schedule built from the macros below hashes identically to one built from hand-written `PhaseAnchorSystem` subclasses and hand-written `declared_run_after` / `declared_run_before` arrays carrying the same `ECS_SYSTEM` name strings (pinned in tests/src/ecs/SystemPhaseAnchors.cpp).

The core defines the *mechanism* only — phase names belong to game code.

### `PhaseAnchorSystem<Derived>`

```cpp
template<typename Derived>
struct PhaseAnchorSystem : System<Derived> {
	void tick(TickContext const&) {}
	void tick_all(World&, TickContext const&) {}
};
```

The canonical base for a hand-written anchor (src/openvic-simulation/ecs/SystemPhase.hpp). Do **not** try to write an anchor as a bare `System<Derived>` with an empty tick: an empty component pack does not compile through the default dispatch path, and even if it did, an empty require set would match *every* archetype. `PhaseAnchorSystem` shadows `tick_all` so an anchor's "execution" is a true no-op — no query is built, no rows are touched, and `declared_access()` derives an empty access set.

### `ECS_PHASE_ANCHOR_FIRST` / `ECS_PHASE_ANCHOR`

```cpp
ECS_PHASE_ANCHOR_FIRST(anchor_name)            // first phase: no predecessor
ECS_PHASE_ANCHOR(anchor_name, prev_anchor)     // chained after prev_anchor
```

Each expands to a `PhaseAnchorSystem` subclass (with the chain's `declared_run_after` for the non-first form) plus its `ECS_SYSTEM(anchor_name)` identity.

**Both must be invoked at global namespace scope.** They expand `ECS_SYSTEM`, which opens `namespace OpenVic::ecs`; the anchor type lands in the enclosing (global) namespace, and its stringified name must be globally unique — anchors are singular by nature.

**Register every anchor.** An anchor is just a system; the macros declare it, they do not register it. Per the silently-ignored-edges rule above, an unregistered anchor makes every edge through it vanish without an error — the phase fence evaporates. `world.register_system<anchor_name>()` for each one, in any order.

### `ECS_IN_PHASE`

```cpp
ECS_IN_PHASE(prev_anchor, next_anchor, extra_run_after_systems...)
```

Written **inside a system's class body**, this declares phase membership: it expands to exactly the `declared_run_after` / `declared_run_before` pair — `run_after(prev_anchor, extras...)`, `run_before(next_anchor)`. The variadic extras append intra-phase ordering: each extra system type is added to the generated `declared_run_after`, so the system runs after those co-phase systems too.

Rules:

- Works identically on `System<>`, `SystemThreaded<>`, and `ChunkSystem<>`, and composes freely with `Filters`, `extra_reads` / `extra_writes`, and `should_run`.
- A system using `ECS_IN_PHASE` **cannot also hand-write** `declared_run_after` or `declared_run_before` — that is an in-class member redefinition, a hard compile error (MSVC C2556/C2371), never silent shadowing. Pick one form per system.
- A template-id extra (e.g. `some_system<A, B>`) splits on the comma at the preprocessor level — use a type alias.
- Members land *strictly between* their anchors in the stage layout, but anchors do not get dedicated stages — a system with no path to an anchor may legally share the anchor's stage; only the relative order (anchor before member before next anchor) is guaranteed.

### Complete phase example

Adapted from tests/src/ecs/SystemPhaseAnchors.cpp:

```cpp
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemPhase.hpp"
#include "openvic-simulation/ecs/World.hpp"

using namespace OpenVic::ecs;

// Anchors: GLOBAL namespace scope, names globally unique.
ECS_PHASE_ANCHOR_FIRST(TickPhaseProduction)
ECS_PHASE_ANCHOR(TickPhaseTrade, TickPhaseProduction)
ECS_PHASE_ANCHOR(TickPhaseConsumption, TickPhaseTrade)

namespace {
	struct FactoryOutput { int64_t v = 0; };
	struct MarketOffer { int64_t v = 0; };
}
ECS_COMPONENT(FactoryOutput, "example::FactoryOutput")
ECS_COMPONENT(MarketOffer, "example::MarketOffer")

namespace {
	// Production phase (between TickPhaseProduction and TickPhaseTrade).
	struct FactoryProduceSystem : SystemThreaded<FactoryProduceSystem> {
		ECS_IN_PHASE(TickPhaseProduction, TickPhaseTrade)
		void tick(TickContext const& /*ctx*/, FactoryOutput& out) {
			out.v += 1;
		}
	};

	// Trade phase. Disjoint components — these two would co-stage, but the
	// variadic extra forces OfferMatchSystem after OfferPostSystem.
	struct OfferPostSystem : System<OfferPostSystem> {
		ECS_IN_PHASE(TickPhaseTrade, TickPhaseConsumption)
		void tick(TickContext const& /*ctx*/, MarketOffer& o) {
			o.v += 1;
		}
	};
	struct OfferMatchSystem : System<OfferMatchSystem> {
		ECS_IN_PHASE(TickPhaseTrade, TickPhaseConsumption, OfferPostSystem)
		void tick(TickContext const& /*ctx*/, FactoryOutput& out) {
			out.v *= 2;
		}
	};
}
ECS_SYSTEM(FactoryProduceSystem)
ECS_SYSTEM(OfferPostSystem)
ECS_SYSTEM(OfferMatchSystem)

void register_tick(World& world) {
	// Anchors are systems too — register them or every edge through them
	// silently disappears. Order does not matter.
	world.register_system<TickPhaseProduction>();
	world.register_system<TickPhaseTrade>();
	world.register_system<TickPhaseConsumption>();
	world.register_system<FactoryProduceSystem>();
	world.register_system<OfferPostSystem>();
	world.register_system<OfferMatchSystem>();
}
```

Note that `FactoryProduceSystem` (writes `FactoryOutput`) and `OfferMatchSystem` (also writes `FactoryOutput`) are in *different* phases — the anchor chain already orders them, so the conflict is satisfied by the existing path and no auto-orientation is needed. Had they been in the same phase with no extra edge, the auto-orienter would have serialised them between the anchors, deterministically but in an order you did not choose.

## Rules of thumb

(The consolidated all-topics footgun list lives in pitfalls.md.)

- Let data declarations do the ordering; reach for explicit edges or phases only when two systems must be ordered for *semantic* reasons the access model cannot see (or when the auto-orienter's arbitrary-but-deterministic choice is not the one you need).
- Declare every singleton access (`extra_reads` / `extra_writes`) — undeclared singleton access co-schedules and silently breaks lockstep determinism.
- Register every system you reference in an edge — especially anchors. Unmatched edge ids are silently dropped.
- Register once at startup; gate cadence with `should_run`, never with re-registration.
- Never nest `parallel_for` inside a tick body.
- Renaming a system's `ECS_SYSTEM` string changes its identity and the `schedule_hash` — a breaking change for saves, replays, and the multiplayer handshake.
- The TU that calls `register_system` must include `openvic-simulation/ecs/SystemImpl.hpp` (registration instantiates the iteration drivers defined there); do not otherwise depend on its contents.

## Source files

- src/openvic-simulation/ecs/SystemScheduler.hpp — `SystemScheduler`, `ScheduledStage`, `schedule_hash`
- src/openvic-simulation/ecs/SystemScheduler.cpp — DAG build, auto-orientation, disjoint-iteration override, stage execution
- src/openvic-simulation/ecs/SystemPhase.hpp — `PhaseAnchorSystem`, `ECS_PHASE_ANCHOR_FIRST`, `ECS_PHASE_ANCHOR`, `ECS_IN_PHASE`
- src/openvic-simulation/ecs/System.hpp — `System`, `SystemThreaded`, `declared_run_after` / `declared_run_before`, `extra_reads` / `extra_writes`, `should_run` contract
- src/openvic-simulation/ecs/SystemAccess.hpp — `AccessMode`, `ComponentAccess`, conflict definition
- src/openvic-simulation/ecs/SystemTypeID.hpp — `system_type_id_of`, `ECS_SYSTEM`
- src/openvic-simulation/ecs/World.hpp — `register_system`, `tick_systems`, `schedule_hash`, `set_serial_mode`, `set_ecs_worker_count`
- Tests: tests/src/ecs/SystemScheduler_DAG.cpp, tests/src/ecs/SystemScheduler_Conflicts.cpp, tests/src/ecs/SystemSchedulerDisjointWriters.cpp, tests/src/ecs/SystemSchedulerSingletonWrites.cpp, tests/src/ecs/SystemPhaseAnchors.cpp, tests/src/ecs/MultiSystemMixedStage.cpp
