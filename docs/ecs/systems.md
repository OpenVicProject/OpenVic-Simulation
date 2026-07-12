# Writing systems

Systems are where per-tick simulation logic lives. You write a small CRTP struct with a `tick` (or `tick_chunk`) member function, the ECS derives the system's component access from that function's signature at compile time, and the `SystemScheduler` uses those declarations to build a deterministic, automatically-parallelised tick pipeline. You never iterate entities by hand inside a system and you never call iteration drivers yourself — you declare *what* you touch, and the scheduler decides *when and where* your code runs.

This page covers the three system bases, the access-declaration model, ordering, the optional `should_run` cadence gate, query filters, `TickContext`, and registration. For how stages are formed and what the scheduler guarantees, see [scheduling.md](scheduling.md); for the iteration/query machinery itself, see [queries.md](queries.md).

## Choosing a base

| Base | Body you implement | Execution | Pick it when |
|---|---|---|---|
| `System<Derived>` | `void tick(TickContext const&, [EntityID,] Cs&...)` — called once per matching row | Serial within the system (the stage around it may still run other systems concurrently) | Default choice. Pick it for per-row logic that must observe earlier rows' writes, or when you need `extra_writes()` (singleton/cross-archetype mutation) together with per-row dispatch — `extra_writes()` is legal on any serial base (`System<>` or `ChunkSystem`) and forbidden only on `SystemThreaded`. |
| `SystemThreaded<Derived>` | Same `tick` shape as `System<Derived>` | Chunk-parallel: matched chunks are distributed across the `EcsThreadPool` workers | Large row counts, per-row work that only touches that row's components. Forbidden: `extra_writes()` (see below), any shared mutable state. |
| `ChunkSystem<Derived, Cs...>` | `void tick_chunk(ChunkView<Cs...> view, TickContext const&)` — called once per matching chunk | Serial | Tight inner loops over big archetypes: you get raw contiguous component slabs and write the per-row loop yourself (SIMD-friendly, no per-row call overhead). |

All three share the same metadata surface: `declared_run_after()` / `declared_run_before()`, `extra_reads()` / `extra_writes()`, the optional `Filters` alias, and the optional `static bool should_run(TickContext const&)` gate. All three are registered the same way via `world.register_system<S>()`.

Multi-system stages mix `System<>` and `SystemThreaded` freely — the scheduler builds one flat work-item list across every system in a stage and dispatches it via a single outer `parallel_for`. There is no "threaded system silently falls back to serial inside a shared stage" footgun.

**Include rule:** a translation unit that defines a concrete system and registers it must include `openvic-simulation/ecs/SystemImpl.hpp` (which pulls in `World.hpp` and `System.hpp`) — the templated iteration drivers are defined there and must be visible at the point `register_system` instantiates them. `ChunkSystem.hpp` already includes it. Do not rely on or document anything *inside* `SystemImpl.hpp`; it is internal.

## The tick signature is the access declaration

`System<Derived>` (and `SystemThreaded<Derived>`) extract the component pack from `&Derived::tick` at compile time. Three shapes are accepted:

```cpp
// Per-row, components only:
void tick(TickContext const& ctx, Cs&... components);

// Per-row with the row's entity handle:
void tick(TickContext const& ctx, EntityID eid, Cs&... components);

// Per-row with an immutable handle — add_component / remove_component on it
// will not compile inside the tick body (see entities.md):
void tick(TickContext const& ctx, ImmutableEntityID eid, Cs&... components);
```

Each `Cs` parameter declares both a *requirement* (the system iterates only archetypes carrying all of them) and an *access mode*:

- `C const& c` → `AccessMode::Read`
- `C& c` → `AccessMode::Write`

That access set is what the scheduler uses to detect conflicts between systems: two systems overlap when at least one of them writes a component the other reads or writes (Read/Read never conflicts). Conflicting systems are serialised into different stages — unless their iteration is provably disjoint via `Filters` (e.g. one writes `C` requiring `B`, the other writes `C` `Without<B>`), in which case the scheduler co-stages them; see [scheduling.md](scheduling.md). Non-conflicting systems may run concurrently. **Declare the weakest access that is true** — taking `C&` when you only read `C` costs you parallelism for no benefit.

Tag (zero-size) components may appear in the pack to narrow the iteration; the reference you receive carries no data (it aliases a shared static instance). Take tags as `Tag const&` so they don't register spurious Write access.

`tick` is non-virtual — there is no base-class override. The CRTP base finds it by name and signature; a typo in the name is a compile error at registration, not a silent no-op.

### A complete serial system

Adapted from `tests/src/ecs/SystemSchedulerSingletonWrites.cpp` and `tests/src/ecs/SystemShouldRun.cpp`:

```cpp
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"

namespace OpenVic {
	struct Treasury {
		int64_t balance = 0;
	};
	// Singleton — global per-session total, never attached to an entity.
	struct UpkeepTotal {
		int64_t value = 0;
	};
}
ECS_COMPONENT(OpenVic::Treasury, "OpenVic::Treasury")
ECS_COMPONENT(OpenVic::UpkeepTotal, "OpenVic::UpkeepTotal")

namespace OpenVic {
	// Deducts monthly upkeep from every Treasury row and folds it into the
	// UpkeepTotal singleton.
	struct MonthlyUpkeepSystem : ecs::System<MonthlyUpkeepSystem> {
		// The singleton is invisible to the tick signature — mutation through
		// ctx.world MUST be declared so the scheduler serialises this system
		// against every other reader/writer of UpkeepTotal.
		static constexpr std::array<ecs::component_type_id_t, 1> extra_writes() {
			return { ecs::component_type_id_of<UpkeepTotal>() };
		}

		// Dispatch-time cadence gate: the tick body only runs on month starts.
		static bool should_run(ecs::TickContext const& ctx) {
			return ctx.today.is_month_start();
		}

		void tick(ecs::TickContext const& ctx, Treasury& treasury) {
			UpkeepTotal* total = ctx.world.get_singleton<UpkeepTotal>();
			int64_t const upkeep = treasury.balance / 100;
			treasury.balance -= upkeep;
			total->value += upkeep;
		}
	};
}
ECS_SYSTEM(OpenVic::MonthlyUpkeepSystem)
```

And driving it:

```cpp
ecs::World world;
world.set_singleton<OpenVic::UpkeepTotal>(); // create singletons BEFORE the first tick_systems
// ... create entities (see entities.md) ...
world.register_system<OpenVic::MonthlyUpkeepSystem>();

Date today { 1836, 1, 1 };
world.tick_systems(today);
```

## System identity: `ECS_SYSTEM`

Every system type used with `World` needs a stable string identity, declared once at **global namespace scope** (the macro opens `namespace OpenVic::ecs` itself):

```cpp
#define ECS_SYSTEM(Type)
```

```cpp
namespace OpenVic { struct UnitGroupTotalsSystem { /* ... */ }; }
ECS_SYSTEM(OpenVic::UnitGroupTotalsSystem)
```

The macro stringifies its argument, so the literal is the qualified name you pass in (`"OpenVic::UnitGroupTotalsSystem"`). Forgetting it is a clear compile error ("incomplete type `SystemName<X>`") the first time the type meets `World`.

The id derived from it:

```cpp
using system_type_id_t = uint64_t;

template<typename S>
constexpr system_type_id_t system_type_id_of();
```

is an FNV-1a hash of that string. You use `system_type_id_of<S>()` in exactly one place as a system author: `declared_run_after()` / `declared_run_before()` entries. Everything else about `system_type_id_t` is scheduler plumbing — but the *stability* contract matters to you: the id feeds `schedule_hash()` (the multiplayer session-start handshake) and anything that persists system ids, so **renaming a system's `ECS_SYSTEM` literal is a breaking change** to saves, replays, and cross-version multiplayer.

## Registering with the World

```cpp
template<typename S, typename... Args>
SystemHandle register_system(Args&&... args);

void unregister_system(SystemHandle handle);
void tick_systems(Date today);
// Legacy source-compatibility shim: the supplied context — including ctx.today —
// is ignored entirely, and the tick runs with a default Date. Always use
// tick_systems(Date today).
void tick_systems(TickContext const& ctx);
void clear_systems();
uint64_t schedule_hash();
```

`world.register_system<S>()` constructs the system instance (forwarding any `args` to `S`'s constructor), extracts all static metadata (`declared_access()`, `declared_run_after()`, `declared_run_before()`, `extra_reads()`, `extra_writes()`, `Filters`, `should_run`), and marks the schedule dirty; the scheduler rebuilds its DAG and stage layout lazily on the next `tick_systems` call. The schedule is registration-order-independent — register the same *set* of systems on every machine in a lockstep session (`schedule_hash()` mismatches reject the join); see [scheduling.md](scheduling.md).

Two `static_assert`s fire at registration:

- `SystemThreaded` with a non-empty `extra_writes()` is rejected (see below).
- A present-but-malformed `should_run` is rejected (see below) instead of being silently ignored.

The returned handle:

```cpp
struct SystemHandle {
	uint32_t index = 0;
	uint32_t generation = 0;
	constexpr bool is_valid() const;
};

inline constexpr SystemHandle INVALID_SYSTEM_HANDLE = {};
```

is only needed if you intend to `unregister_system` later. Generation is bumped on unregister, so passing a stale handle to `unregister_system` again is a safe no-op — the slot's generation no longer matches the handle's — rather than touching the wrong slot. Note that `is_valid()` does **not** detect staleness: it only distinguishes a real handle from the default-constructed `INVALID_SYSTEM_HANDLE` sentinel (generation 0), so a once-valid handle still reports `is_valid()` after its system is unregistered. Most game systems are registered once per session and never unregistered — for periodic systems, use `should_run`, **not** register/unregister churn (which changes `schedule_hash` every time).

**Systems are stateless by project rule.** All simulation state lives in components and singletons — never in system instance members. System instances are not serialized, so any state you stash on one silently diverges across save/load. (This is also why `should_run` is required to be `static`.) Constructor arguments are for wiring test instrumentation, not simulation state.

`tick_systems(Date today)` is the normal entry: it builds the `TickContext` and runs every stage. Drive it once per simulation day from your session loop.

## Cross-archetype and singleton access: `extra_reads()` / `extra_writes()`

```cpp
static constexpr std::array<component_type_id_t, N> extra_reads();
static constexpr std::array<component_type_id_t, N> extra_writes();
```

The tick signature only declares access to components **on the rows you iterate**. Anything else your tick body touches through `ctx.world` is invisible to the scheduler unless you declare it:

- `extra_reads()` — components you read on rows you do *not* iterate (`ctx.world.get_component<C>(other_eid)`), and singletons you read (`ctx.world.get_singleton<C>()`). Folded into the access set as Read.
- `extra_writes()` — components or singletons you *mutate* on non-iterated rows. Folded in as Write, so the scheduler serialises you against every other reader/writer of that id. Singletons share `component_type_id_t` with components, so `component_type_id_of<MySingleton>()` is the right entry for both.

Override them as `static constexpr` members on your derived class (the defaults on the base return empty arrays). The why: the scheduler's conflict detection runs entirely on these declared sets. An **undeclared** singleton or cross-archetype access means a conflicting pair of systems can be co-scheduled into the same parallel stage — a silent data race that breaks worker-count-invariant determinism, and the determinism gate (`tests/src/ecs/WorkerCountInvariance.cpp`-style digests) catches it only probabilistically. Declare everything; see [determinism.md](determinism.md).

Related rule: **create every singleton before the first `tick_systems` call.** Creating a new singleton mid-tick (`set_singleton` on a missing id) rehashes the World's singleton hashmap, which can race with a concurrent declared *reader* even when your access declarations are correct. See [components.md](components.md).

### Why `extra_writes()` is forbidden on `SystemThreaded`

`register_system` hard-errors on it:

```
SystemThreaded must not declare extra_writes(): its chunks run concurrently, so a
cross-archetype/singleton write races the system with ITSELF — no scheduler edge
can fix that. Use a serial System<> (with Reductions::* for parallel folds) instead.
```

Scheduler edges order *systems* relative to each other; they cannot order one system's chunks against each other, because those chunks are dispatched concurrently by design. Two worker threads running two chunks of the same `SystemThreaded` would both write the shared target. `extra_reads()` **is** legal on `SystemThreaded` — concurrent reads of a stable pointer are safe. If you need a parallel fold into shared state, run a serial `System<>` and use `Reductions` (see [threading-and-reductions.md](threading-and-reductions.md)).

## Ordering: `declared_run_after()` / `declared_run_before()`

```cpp
static constexpr std::array<system_type_id_t, N> declared_run_after();
static constexpr std::array<system_type_id_t, N> declared_run_before();
```

Access conflicts already force an ordering between overlapping systems (deterministically auto-oriented by the scheduler), so you only need explicit edges when the *semantic* order matters beyond data-flow — e.g. "the counter must observe entities the spawner created this tick":

```cpp
struct STSSpawnedCounter : System<STSSpawnedCounter> {
	void tick(TickContext const& ctx, STSSpawned const&) { /* ... */ }

	static constexpr std::array<system_type_id_t, 1> declared_run_after() {
		return { system_type_id_of<STSSpawnSystem>() };
	}
};
```

An edge from A to B guarantees B runs in a later stage than A, *after* the stage barrier — meaning A's `CommandBuffer` has been applied, so entities A spawned via `ctx.cmd` are real, finalised, and visible to B's query. Phase anchors (`ECS_PHASE_ANCHOR` / `ECS_IN_PHASE`) are sugar over exactly these edges; see [scheduling.md](scheduling.md).

## The cadence gate: `static bool should_run(TickContext const&)`

A system may declare:

```cpp
static bool should_run(TickContext const& ctx);
```

(`noexcept` allowed; nothing else is — it must be a single, non-overloaded, **static** function with exactly this signature, or registration hard-fails with a `static_assert` rather than silently ignoring it. Absent means "always run".)

Semantics, all verified by `tests/src/ecs/SystemShouldRun.cpp`:

- Evaluated **exactly once per tick, on the main thread**, at the start of the system's stage — before any work items are built and before any system in the stage executes. Never per chunk, never per entity, never on a worker.
- `false` skips the system's dispatch for this tick: `tick_all` is not called, no chunks are collected, the query-cache prewarm is skipped.
- The skip is **dispatch-time only**. The system still occupies its stage, its conflict/ordering edges still constrain co-staged systems, and `schedule_hash()` is untouched. Stage layout is identical to a predicate-free twin. Skipping can therefore never perturb the schedule — it is lockstep-safe by construction, and gating via `should_run` is checksum-identical to an in-body early-out on the same condition.

**Purity contract (lockstep multiplayer):** `should_run` must be a pure function of deterministic simulation state — `ctx.today` and singletons read via `ctx.world`. Reading wall-clock, thread ids, RNG, or any per-machine state desyncs lockstep. Do not write through `ctx.cmd` or `ctx.world`; the context is passed for reads only. The scheduler cannot check purity — violations are caught only probabilistically by the worker-count gate. Singleton reads inside `should_run` need **no** `extra_reads()` declaration: evaluation happens on the main thread while no workers run, and always observes the previous stage's barrier state.

Use this for monthly/periodic systems:

```cpp
struct SrThreadedGated : SystemThreaded<SrThreadedGated> {
	static bool should_run(TickContext const& ctx) {
		return ctx.today.is_month_start();
	}
	void tick(TickContext const&, SrA& a) { a.v = a.v * 31 + 7; }
};
```

Do **not** implement cadence by registering/unregistering systems per tick — that churns `schedule_hash` and rebuilds the schedule.

## Query filters: the `Filters` alias

A system narrows its iteration with an optional member alias:

```cpp
struct SfExcludeDeadSystem : System<SfExcludeDeadSystem> {
	using Filters = Filter<Without<SfDead>>;
	void tick(TickContext const& /*ctx*/, SfCore& c) {
		c.v += 1;
	}
};
```

`Without<C>` makes the iteration query **skip every archetype carrying C**. The require set is still the tick parameter pack (or the `ChunkSystem` template list); `Filters` only adds the exclude set. Properties you can rely on (pinned by `tests/src/ecs/SystemFilters.cpp`):

- The excluded component adds **no access** — a system excluding `C` does not conflict with a writer of `C`, so they can share a stage.
- Duplicates collapse and ids are sorted; excluding a component no archetype carries simply matches everything.
- The filter applies identically on the serial, threaded, and chunk dispatch paths, and the filtered query re-resolves when new archetypes appear between ticks.
- Only `Without<C>` exists today; any other marker inside `Filter<...>` is a compile error, not a silent no-op.

This is the cheap way to model "logically dead/frozen" entities: tag them, and let every per-tick system exclude the tag — no archetype migration per row, just one tag added via `ctx.cmd.add_component`. See [queries.md](queries.md) for the underlying `Query` semantics.

## `TickContext`

```cpp
struct TickContext {
	World& world;
	Date today;
	CommandBuffer& cmd;
};
```

- `today` — the simulation date for this tick. The only clock a system may consult (wall-clock reads break determinism).
- `cmd` — the system's `CommandBuffer` for **all structural mutations**: `ctx.cmd.create_entity(ctx.world, Cs {...}...)`, `ctx.cmd.destroy_entity(eid)`, `ctx.cmd.add_component<C>(eid, ...)`, `ctx.cmd.remove_component<C>(eid)`. Ops are deferred and applied at the stage barrier, in the stage's deterministic order across its systems (ascending `system_type_id_t` — see [scheduling.md](scheduling.md)). For `SystemThreaded`, `cmd` refers to a *per-chunk* buffer; the per-chunk buffers are merged in `chunk_idx` ascending order before apply, which is what makes spawn order deterministic and identical across all worker counts (pinned by `tests/src/ecs/SystemThreadedSpawn.cpp`). Calling `ctx.cmd.*` requires including `openvic-simulation/ecs/CommandBuffer.hpp` in your system's TU; details in [command-buffer.md](command-buffer.md).
- `world` — read access to everything: `get_component` / `has_component` / `get_singleton` / `is_alive` / `is_immutable`. Two hard rules:
	1. **Never structurally mutate `ctx.world` from inside a tick.** `World::create_entity`, `destroy_entity`, `add_component` and `remove_component` are guarded during `tick_systems` — they are refused as no-ops (returning a null/false result) with an error log rather than corrupting concurrent iteration. `ctx.cmd` is the only mutation path.
	2. **Every non-iterated read or write through `ctx.world` must be declared** via `extra_reads()` / `extra_writes()` as described above. Writing through a `get_component` pointer on a row you iterate is fine only if that component is in your tick pack as `C&`.

Pointer lifetime still applies inside ticks: a `ctx.world.get_component<C>(eid)` pointer is valid only until the next structural mutation of that archetype — which inside a tick means it is safe for the duration of your tick body (structural ops are deferred), but never cache it across ticks. Use `CachedRef` for that (see [entities.md](entities.md)).

## `SystemThreaded` specifics

`SystemThreaded<Derived>` keeps the exact `tick` shapes of `System<Derived>`; what changes is execution: matched chunks are dispatched in parallel across the `EcsThreadPool`. Guarantees and rules:

- One chunk is processed by at most one worker at a time; rows within a chunk run serially in row order.
- Your tick body may only touch (a) the row's own components from the pack, (b) declared `extra_reads()` targets (read-only), (c) `ctx.cmd`. Anything else — member variables, globals, statics, undeclared world state — is a data race.
- `extra_writes()` does not compile (self-race, see above). Parallel folds go through `Reductions` from a serial system, or a downstream serial system ordered with `declared_run_after()`.
- **Never call `EcsThreadPool::parallel_for` from inside a tick body.** The scheduler already owns the outer `parallel_for` around your stage; a nested one blocks the worker running your tick while its inner jobs wait for *other* workers — if every worker does this, the pool deadlocks. Straight-line code inside ticks.
- Determinism holds across worker counts: chunk assignment to workers varies, but per-chunk command buffers merge in `chunk_idx` ascending order and all component writes are row-local, so the end state is bit-identical at 1, 2, 4, 8, or 16 workers.

A complete threaded spawn pipeline, adapted from `tests/src/ecs/SystemThreadedSpawn.cpp`:

```cpp
// Chunk-parallel: for each (Source, SpawnCount) row, queue deferred creates.
// Placeholders resolve at the stage barrier in chunk_idx ascending order.
struct STSSpawnSystem : SystemThreaded<STSSpawnSystem> {
	void tick(TickContext const& ctx, EntityID, STSSource const& src, STSSpawnCount const& sc) {
		for (int i = 0; i < sc.count; ++i) {
			ctx.cmd.create_entity(ctx.world, STSSpawned { src.id });
		}
	}
};

// Serial; ordered after the spawner, so it observes the freshly-finalised
// entities. Folds the spawned population into a declared singleton.
struct STSSpawnedCounter : System<STSSpawnedCounter> {
	static constexpr std::array<system_type_id_t, 1> declared_run_after() {
		return { system_type_id_of<STSSpawnSystem>() };
	}
	static constexpr std::array<component_type_id_t, 1> extra_writes() {
		return { component_type_id_of<STSSpawnedTotal>() };
	}

	void tick(TickContext const& ctx, STSSpawned const&) {
		STSSpawnedTotal* total = ctx.world.get_singleton<STSSpawnedTotal>();
		total->value += 1;
	}
};
```

```cpp
ECS_SYSTEM(STSSpawnSystem)
ECS_SYSTEM(STSSpawnedCounter)
```

(The original test version of `STSSpawnedCounter` omits `extra_writes` and nullptr-checks instead; declare the singleton write in real game code, as shown, so the scheduler can see it.)

For debugging, `world.set_serial_mode(true)` disables stage-level (inter-system) parallelism and `world.set_ecs_worker_count(n)` pins the pool size — combine both with `n = 1` for fully single-threaded runs (see [scheduling.md](scheduling.md)). Parallel results must equal serial results, and the tests assert exactly that.

## `ChunkSystem<Derived, Cs...>`

```cpp
template<typename Derived, typename... Cs>
struct ChunkSystem;

// Derived implements:
void tick_chunk(ChunkView<Cs...> view, TickContext const& ctx);
```

Here the component list moves from the tick signature to the base's template arguments, with the same access convention: `C const` in the list declares Read, plain `C` declares Write. Note the `ChunkView` parameter names the **plain** types — constness lives only in the base's list:

```cpp
struct MoverChunkSystem : ChunkSystem<MoverChunkSystem, CSPosition, CSVelocity const> {
	void tick_chunk(ChunkView<CSPosition, CSVelocity> view, TickContext const&) {
		CSPosition* pos = view.template array<CSPosition>();
		CSVelocity* vel = view.template array<CSVelocity>();
		for (std::size_t i = 0; i < view.count(); ++i) {
			pos[i].x += vel[i].dx;
			pos[i].y += vel[i].dy;
		}
	}
};
ECS_SYSTEM(MoverChunkSystem)
```

`ChunkSystem` runs serially (`is_threaded` is `false`), once per matched chunk per tick. Its value is the inner loop: the slabs are contiguous and aligned, so the compiler can vectorise. The view surface:

```cpp
template<typename... Cs>
struct ChunkView {
	std::size_t count() const;            // rows in this chunk; all arrays share this length
	EntityID* OV_RESTRICT entities();     // per-row entity ids
	template<typename C>
	C* OV_RESTRICT array();               // component slab; C must be one of Cs...
};
```

Rules:

- **Tag (zero-size) components map to `nullptr` slabs** — never dereference `array<Tag>()`. A tag in the `Cs...` list still narrows iteration; its presence is conveyed by archetype membership, not data. (Same root cause as `get_component<Tag>` returning nullptr — see [components.md](components.md).)
- The view is valid **only inside `tick_chunk`** — chunk data may be relocated by any later structural mutation. Don't stash slab pointers anywhere.
- `entities()` and `array<C>()` return `OV_RESTRICT` pointers (the slabs never alias). For the strongest codegen, bind each slab into a local at the top of `tick_chunk` as shown — return-type restrict qualifiers are honored less reliably than restrict-qualified locals.
- Everything else from the shared metadata surface works unchanged: `Filters`, `extra_reads()` / `extra_writes()` (legal here — serial), `declared_run_after()` / `declared_run_before()`, `should_run`, and `ctx.cmd` for structural ops.

For the storage model behind chunks and why this layout is fast, see [storage-model.md](storage-model.md).

## Rules recap

- Structural mutation inside a tick goes through `ctx.cmd`, never `ctx.world.*` — the World guard turns direct calls into refused no-ops (null/false results) with an error log; write the code right the first time.
- Declare every singleton / cross-archetype access with `extra_reads()` / `extra_writes()`; create all singletons before the first `tick_systems`.
- `extra_writes()` + `SystemThreaded` does not compile; use a serial `System<>` and `Reductions` for parallel folds.
- `should_run` must be `static`, pure over deterministic state (`ctx.today`, singletons), and read-only — it is your cadence tool precisely because it cannot perturb the schedule.
- Systems are stateless: no simulation state in members; instances are not serialized.
- No `EcsThreadPool::parallel_for` inside tick bodies — deadlock risk.
- No floats in tick math — `fixed_point_t` everywhere ([determinism.md](determinism.md)).
- Renaming an `ECS_SYSTEM` literal breaks saves/replays/multiplayer handshakes.
- Need entities pre-shaped for your system? Attach output components at `create_entity` time — per-row `add_component` later is an archetype migration per call ([entities.md](entities.md), [pitfalls.md](pitfalls.md)).

## Source files

- `src/openvic-simulation/ecs/System.hpp` — `System<Derived>`, `SystemThreaded<Derived>`, `TickContext`, `SystemHandle`, `should_run` traits and contract
- `src/openvic-simulation/ecs/SystemAccess.hpp` — `AccessMode`, `ComponentAccess`, access-set merge helpers
- `src/openvic-simulation/ecs/ChunkSystem.hpp` — `ChunkSystem<Derived, Cs...>`
- `src/openvic-simulation/ecs/ChunkView.hpp` — `ChunkView<Cs...>`
- `src/openvic-simulation/ecs/QueryFilter.hpp` — `Filter`, `Without`, the `Filters` alias machinery
- `src/openvic-simulation/ecs/SystemTypeID.hpp` — `system_type_id_t`, `system_type_id_of`, `ECS_SYSTEM`
- `src/openvic-simulation/ecs/World.hpp` — `register_system`, `unregister_system`, `tick_systems`, `clear_systems`, `schedule_hash`
- Tests with working examples: `tests/src/ecs/System.cpp`, `tests/src/ecs/SystemAccess.cpp`, `tests/src/ecs/ChunkSystem.cpp`, `tests/src/ecs/SystemShouldRun.cpp`, `tests/src/ecs/SystemFilters.cpp`, `tests/src/ecs/SystemThreadedSpawn.cpp`, `tests/src/ecs/SystemSchedulerSingletonWrites.cpp`
