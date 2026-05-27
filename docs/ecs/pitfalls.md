# Pitfalls and rules

This is the consolidated list of rules you must internalize before writing game code against the ECS. Every rule here is enforced by a real mechanism — a runtime guard, a `static_assert`, a determinism test, or a performance cliff — and breaking one either no-ops (loudly or silently), desyncs lockstep multiplayer, or costs an order of magnitude at scale. Each rule states **what to do** in one line, the WHY grounded in how the ECS actually behaves, and the correct alternative. For the full API surface behind each rule, follow the cross-links to the topic docs.

Rules are grouped by theme:

- [Lifecycle](#lifecycle) — creating, mutating, destroying entities
- [Pointers and handles](#pointers-and-handles) — what stays valid for how long
- [Systems and scheduling](#systems-and-scheduling) — declarations the scheduler relies on
- [Threading](#threading) — what is safe in parallel
- [Determinism](#determinism) — lockstep-multiplayer rules
- [Toolchain](#toolchain) — build/compiler footguns

---

## Lifecycle

### **Pre-attach every component the entity will ever need at `create_entity` time.**

```cpp
template<typename... Cs>
EntityID create_entity(Cs&&... values);

template<typename C>
C* add_component(EntityID id, C&& value);
```

(`src/openvic-simulation/ecs/World.hpp`)

WHY: an entity's component set IS its archetype, and the archetype decides which chunk slabs the entity's data lives in (see [storage-model.md](storage-model.md)). `add_component` after creation is an **archetype migration**: the World builds the extended signature, finds or creates the target archetype, reserves a row there, move-constructs *every* existing component across, then swap-pop compacts the source archetype (relocating an unrelated entity into the hole) and bumps every column version. That is one full migration **per call, per entity**. At the scale this simulation targets (e.g. a million order entities per tick) it is lethal.

Instead:

- Pass the full component set to `create_entity(Cs&&... values)` — including zero-size tag components and output components a later system will fill in.
- For large batches, use the bulk API, which pays the signature sort, archetype lookup, and per-chunk version bump once per batch instead of once per entity:

```cpp
template<typename... Cs, typename... Spans>
bool create_entities(std::size_t count, std::span<EntityID> out_ids, Spans&&... spans);
```

  Contract: one span per **non-empty** component (matched positionally to the non-empty `Cs...` in pack order; tags take no span), every span length `== count`, `out_ids.size() == count`. Pass no spans at all to default-construct every component. See [entities.md](entities.md).
- If the entity genuinely already carries `C`, `add_component` is cheap: it replaces the value in place and returns the existing pointer — no migration. (It returns `nullptr` if the entity is dead, and `nullptr` for tag types, which have no data.)

### **Bulk-create input spans are moved-from — pass a copy if the source must survive.**

WHY: `create_entities` move-constructs values out of your spans straight into the column slabs (chosen over copying for the 300k-pop session-setup path). After the call your storage holds moved-from husks. Spans of `const` elements are rejected at compile time precisely so this contract cannot silently degrade into copies.

### **Never mutate World structure from inside a system tick — go through `ctx.cmd`.**

```cpp
// On TickContext (src/openvic-simulation/ecs/System.hpp):
struct TickContext {
	World& world;
	Date today;
	CommandBuffer& cmd;
};

// On CommandBuffer (src/openvic-simulation/ecs/CommandBuffer.hpp):
template<typename... Cs>
EntityID create_entity(World& world, Cs&&... values);

void destroy_entity(EntityID id);

template<typename C>
void add_component(EntityID id, C&& value);

template<typename C>
void remove_component(EntityID id);
```

WHY: the scheduler sets an in-tick flag around every system's tick, and the World's structural mutators — `create_entity`, `create_entities`, `destroy_entity`, `add_component`, `remove_component`, `restore_entity` — check it. A direct call from a tick body is **not** a crash: it is refused with an error log (loud-but-no-crash) and returns a no-op value (`EntityID {}`, `nullptr`, or `false`). Your code keeps running and the mutation never happened; watch the error log and the failure return value. The guard exists because a structural change mid-iteration would invalidate the very columns being walked, and because other systems in the same stage may be reading those archetypes concurrently.

`ctx.cmd` defers the op instead: the scheduler drains each system's `CommandBuffer` at the **stage barrier**, in the stage's deterministic order (ascending `system_type_id_t`, independent of registration order) — fixed and worker-count-independent. See [command-buffer.md](command-buffer.md).

Working example (adapted from `tests/src/ecs/InTickMutationGuard.cpp`, which asserts exactly these semantics):

```cpp
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct health {
		int32_t hp = 0;
	};
	struct wounded {}; // zero-size tag component
}
ECS_COMPONENT(health, "game::health")
ECS_COMPONENT(wounded, "game::wounded")

namespace {
	// WRONG — direct World mutation during a tick. The in-tick guard intercepts the
	// call: refused (error log), returns nullptr, the tag is never added.
	struct mark_wounded_wrong : System<mark_wounded_wrong> {
		void tick(TickContext const& ctx, EntityID eid, health const& h) {
			if (h.hp < 10) {
				ctx.world.add_component<wounded>(eid); // refused — error log, returns nullptr, no tag
			}
		}
	};

	// RIGHT — record the op on the system's CommandBuffer. It applies at the stage
	// barrier, after every system in the stage has finished ticking.
	struct mark_wounded_system : System<mark_wounded_system> {
		void tick(TickContext const& ctx, EntityID eid, health const& h) {
			if (h.hp < 10) {
				ctx.cmd.add_component<wounded>(eid);
			}
		}
	};
}
ECS_SYSTEM(mark_wounded_system)

void example() {
	World world;
	EntityID const eid = world.create_entity(health { 3 });
	world.register_system<mark_wounded_system>();
	world.tick_systems(Date {});
	// Past the stage barrier the deferred op has applied:
	// world.has_component<wounded>(eid) == true
}
```

Note the implication: an op queued via `ctx.cmd` is **not visible during the same stage**. `world.has_component<wounded>(eid)` inside the same stage still returns `false`. The op applies at that stage's barrier, so it **is** visible to systems in subsequent stages of the same tick.

### **`remove_component` will not strip an entity's last component — `destroy_entity` instead.**

```cpp
template<typename C>
bool remove_component(EntityID id);

void destroy_entity(EntityID id);
```

WHY: a zero-component entity is an invariant the model does not allow (it would belong to no archetype). `remove_component` returns `false` if the entity is dead, doesn't carry `C`, **or** removing `C` would leave it with zero components. When you mean "this entity is done", say so: `destroy_entity` (or `ctx.cmd.destroy_entity(id)` from inside a tick).

### **Don't destroy or create entities while iterating — collect, then act.**

WHY: `for_each` walks chunk slabs directly. Destroying a row mid-iteration swap-pop relocates the archetype's last row into the hole while the loop is reading it — corrupted iteration. The in-tick guard protects you inside systems, but `for_each` called *outside* a tick has no guard.

Instead: inside a system, queue `ctx.cmd.destroy_entity(eid)` (this is the normal path). Outside a tick, use `for_each_with_entity` to collect the `EntityID`s, then destroy after the loop:

```cpp
template<typename... Cs, typename Fn>
void for_each_with_entity(Fn&& fn); // fn(EntityID, Cs&...)
```

The header comment on this function states the rule verbatim: "Useful for collecting IDs to destroy later (you can't destroy during iteration without invalidating columns)." See [queries.md](queries.md).

### **Immutable entities never change archetype — and the type system enforces it.**

```cpp
template<typename... Cs>
ImmutableEntityID create_immutable_entity(Cs&&... values);

bool is_immutable(EntityID id) const;
```

WHY: `ImmutableEntityID` is a distinct type with **no implicit conversion** to `EntityID`, and there are deliberately no `add_component` / `remove_component` overloads for it — passing one to a structural mutator is a compile error. The entity's slot is also flagged at creation, so type-erased paths (e.g. a `CommandBuffer` op recorded against a plain `EntityID` for the same entity) are refused at runtime with an error log. Component **data** stays mutable (`get_component` returns a mutable `C*`), and `destroy_entity` is allowed.

Rules of thumb:

- Holding a plain `EntityID` (e.g. from `for_each_with_entity`) and unsure? Branch on `ctx.world.is_immutable(eid)` before queueing a structural op.
- The only bridge back to a mutable handle is `unsafe_mutable_id()` — intentionally verbose so every structural bypass is auditable by grepping `unsafe_mutable_id`. Don't reach for it casually.

See [entities.md](entities.md).

### **Explicit lifecycle over RAII — release handles at the callsite, not from a destructor.**

WHY: For pools and handle release — e.g. `DenseSlotAllocator`'s `release(uint32_t slot)` for singleton side-table rows — the call must be visible where the lifecycle decision is made, so end-of-session sweeps and ownership are auditable by reading the callsites. A destructor hiding the release defeats that. See [components.md](components.md) for `DenseSlotAllocator` side tables.

Corollary contract (`src/openvic-simulation/ecs/DenseSlotAllocator.hpp`): `release` must happen **exactly once per live slot**. A double release is a contract violation that is **not** detected per-release (a per-call scan would make mass end-of-session sweeps quadratic) — the slot re-enters the free list and the next two `allocate()` calls hand out the same row twice. Only `debug_validate()` catches it: a one-shot O(n log n) invariant check for tests and debug sweeps. (Releasing a slot `>= high_water()` *is* caught: logged and ignored.)

---

## Pointers and handles

### **Component pointers are short-lived — never cache a raw `C*` across structural changes or ticks.**

```cpp
template<typename C>
C* get_component(EntityID id);

template<typename C>
uint64_t component_version_in(EntityID id) const;
```

WHY: `get_component` returns a pointer into a chunk slab at the entity's current `(archetype, chunk, row)`. Rows move: `destroy_entity` of **any** entity in the same archetype swap-pops the last row into the vacated slot; `add_component` / `remove_component` migrate rows between archetypes; new rows are pushed on `create_entity`. The pointer you hold is valid only until the next `create_entity` / `destroy_entity` / `add_component` / `remove_component` touching that archetype — and you usually can't know which archetype another call touches.

The World exposes the invalidation signal directly: `component_version_in<C>(id)` returns the component-column version in the entity's current archetype (0 if dead or no longer carrying `C`). The version monotonically increases on every structural change to that column (push, swap-pop, relocate), so **a stable version implies cached pointers into the column are still valid**.

Across ticks, use `CachedRef<C>` (`src/openvic-simulation/ecs/CachedRef.hpp`), which packages exactly this check — id + version stamp + pointer; the fast path is one comparison:

```cpp
CachedRef<province_weather> ref = CachedRef<province_weather>::from(world, eid);

// ... later, possibly after arbitrary structural changes ...
province_weather* weather = ref.get(world); // re-resolves on version mismatch
if (weather != nullptr) {
	// safe to use until the next structural change
}
```

`get(world)` returns `nullptr` once the entity is dead or no longer carries `C`. A `CachedRef` is safe to copy and to hold across ticks. Or simply re-call `get_component` — it is a slot lookup, not a search. See [entities.md](entities.md).

### **Tag (zero-size) components have no data — `get_component<Tag>` returns `nullptr` by design.**

```cpp
template<typename C>
bool has_component(EntityID id) const;
```

WHY: tags occupy no column storage; their presence is encoded purely in archetype membership. There is no object for `get_component` to point at, so it returns `nullptr` — that is not an error state. Test presence with `has_component<Tag>(eid)`. Corollary: when a tag appears in a tick parameter pack or `for_each` lambda, the reference you receive is a shared static dummy instance — never store per-entity data in a tag type. If it needs data, it is not a tag. See [components.md](components.md).

### **Stale `EntityID`s fail safely — but only if you check.**

WHY: an `EntityID` is `{ index, generation }`; the slot's generation is bumped on reuse, so a stale id reliably fails `is_alive(id)` and makes `get_component` return `nullptr`. Nothing crashes — but nothing warns either. Check `is_alive` (or the `nullptr` return) whenever an id may have outlived its entity.

### **A deferred placeholder `EntityID` is not a real id — never persist it.**

WHY: inside a `SystemThreaded` tick, `ctx.cmd` is a per-chunk buffer in **parallel mode**: `cmd.create_entity` does not touch the World, it returns a placeholder `{ index = local_seq, generation = DEFERRED_GENERATION_BIT }`. The placeholder satisfies `is_valid()` and `is_deferred()`, fails `world.is_alive()`, and every public World accessor treats it as "not present". It is only meaningful as an argument to **other ops on the same buffer** during the same recording (e.g. an immediate `cmd.add_component(placeholder, ...)`). `apply()` resolves placeholders to real ids at the stage barrier — but values you already copied out (into a component, a container, anywhere) are **never rewritten**. A stored placeholder is a permanently dangling id.

```cpp
constexpr bool is_deferred() const; // on EntityID / ImmutableEntityID
```

Contrast: in a plain `System<>` (serial buffer), `cmd.create_entity` reserves a real slot synchronously and returns its real `EntityID` — `world.is_alive(eid)` is still `false` until `apply()` finalises it at the barrier, but the id itself is stable and safe to store. If a threaded system needs to remember entities it spawned, don't smuggle placeholders — re-discover them next tick via a query, or restructure so a serial system does the spawning. See [command-buffer.md](command-buffer.md) and [threading-and-reductions.md](threading-and-reductions.md).

The same contract carries over to the **deferred bulk API**:

```cpp
template<typename... Cs, typename... Spans>
bool create_entities(World& world, std::size_t count, std::span<EntityID> out_ids, Spans&&... spans);

template<typename... Cs, typename... Spans>
bool create_immutable_entities(
	World& world, std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
);
```

(`src/openvic-simulation/ecs/CommandBuffer.hpp`) — the deferred analogue of `World::create_entities`, same input contract (one span per non-empty component in pack order, length `== count`, tags take no span, or no spans to default-construct; input spans are moved-from at record time). In parallel mode `out_ids` receives `count` **sequential deferred placeholders** that are **never rewritten** to real ids — the exact never-persist contract as the single create's returned placeholder, just `count` of them. In serial mode `count` real slots are reserved up-front in creation order, usable for same-buffer `add_component` / `destroy_entity` like single creates. Either way, the bulk op yields the identical id assignment as the equivalent `create_entity` loop.

---

## Systems and scheduling

### **Your tick signature is your access contract — declare everything you touch.**

```cpp
// Read = `C const&`, Write = `C&` in the tick parameter pack:
void tick(TickContext const& ctx, position const& pos, velocity& vel);

// Anything accessed via ctx.world OUTSIDE the iterated rows:
static constexpr std::array<component_type_id_t, 0> extra_reads() { return {}; }
static constexpr std::array<component_type_id_t, 0> extra_writes() { return {}; }
```

WHY: the scheduler builds its stage layout from declared access only — tick parameters plus `extra_reads()` / `extra_writes()`. Systems with disjoint access run concurrently in one stage; two systems with conflicting declared access are serialised — **unless** the scheduler can prove their iterated archetypes are disjoint (one's tick query requires a component the other excludes via `Filters = Filter<Without<...>>`, and every conflicting component appears purely in both tick parameter packs, never in `extra_reads()` / `extra_writes()`). That is the disjoint-iteration conflict override: e.g. one system writing `C` and reading `B` shares a stage with another writing `C` `Without<B>`, despite the shared write on `C`. See [scheduling.md](scheduling.md). An access you perform through `ctx.world` but did not declare is **invisible** to that model: the scheduler may co-schedule your system against a conflicting writer, producing a data race whose outcome depends on thread timing. That is not a crash — it is a silent lockstep desync, and the worker-count gate (below) catches it only probabilistically. See [systems.md](systems.md) and [scheduling.md](scheduling.md).

The same rule covers `ChunkSystem<Derived, Cs...>` (`src/openvic-simulation/ecs/ChunkSystem.hpp`) — it has no per-row tick parameter pack, so the access set is declared by the `Cs...` template list instead, with identical inference: `C const` is Read, `C` is Write. Only where you write the declaration moves; the contract is the same.

### **Singleton access inside a tick MUST be declared, and every singleton must exist before the first `tick_systems`.**

```cpp
template<typename C>
C* set_singleton(C&& value);

template<typename C>
C* get_singleton();
```

WHY (declaration): singletons share `component_type_id_t` with components, but they appear in no tick signature — so a `get_singleton` read needs `extra_reads() = { component_type_id_of<C>() }`, and mutation through the returned pointer needs `extra_writes() = { component_type_id_of<C>() }` (serial systems only — see the next rule). Undeclared, a co-scheduled writer/reader pair silently breaks worker-count-invariant determinism.

WHY (create-before-tick): `set_singleton` on a missing id **inserts into the singleton hashmap**. Done mid-tick, that can rehash the map under a concurrent reader in another system — even when both sides declared their access correctly. Create every singleton during session setup, before the first `tick_systems` call.

```cpp
struct game_speed {
	int32_t days_per_tick = 1;
};
ECS_COMPONENT(game_speed, "game::game_speed")

struct movement_system : System<movement_system> {
	static constexpr std::array<component_type_id_t, 1> extra_reads() {
		return { component_type_id_of<game_speed>() };
	}

	void tick(TickContext const& ctx, position& pos, velocity const& vel) {
		game_speed const* speed = ctx.world.get_singleton<game_speed>();
		pos.x += vel.dx * speed->days_per_tick;
	}
};
```

See [world.md](world.md) for the singleton API and [components.md](components.md) for what belongs in a singleton.

### **`extra_writes()` is forbidden on `SystemThreaded` — the compiler will stop you.**

WHY: a `SystemThreaded`'s chunks run concurrently. A cross-archetype or singleton write target is shared by every chunk, so the system races **with itself** — no scheduler edge between systems can fix an intra-system race. `World::register_system` enforces this with a `static_assert`:

> "SystemThreaded must not declare extra_writes(): its chunks run concurrently, so a cross-archetype/singleton write races the system with ITSELF — no scheduler edge can fix that. Use a serial System<> (with Reductions::* for parallel folds) instead."

Instead: keep the shared-target write in a serial `System<>`, and use the `reductions::*` helpers if the fold itself should be parallel (see [Threading](#threading)).

### **`should_run` must be `static`, and pure over deterministic simulation state.**

```cpp
static bool should_run(TickContext const& ctx);
```

WHY: the scheduler evaluates `should_run` exactly once per tick, on the main thread, at the start of the system's stage. `false` skips dispatch for this tick only — the system still occupies its stage, its ordering/conflict edges still constrain co-staged systems, and `schedule_hash` is untouched. That makes it the lockstep-safe way to do cadence gating; registering/unregistering systems per tick would churn `schedule_hash` (the multiplayer handshake value) instead.

The determinism contract (stated in full in `src/openvic-simulation/ecs/System.hpp`): `should_run` must be a pure function of `ctx.today` and singletons read via `ctx.world`. Reading wall-clock, thread ids, RNG, or any per-machine state desyncs lockstep — and the scheduler cannot check purity. Do not write through `ctx.cmd` or `ctx.world`; the context is for reads only. Singleton reads inside `should_run` need **no** `extra_reads()` declaration: it runs on the main thread while no workers run, observing the previous stage's barrier state.

It must be `static` (systems are stateless — next rule); a member function, data member, wrong-signature variant, or overload set with a candidate callable with `TickContext const&` hard-fails registration via `static_assert` rather than being silently ignored. The one accepted hole (documented in `System.hpp`): an overload set in which **no** candidate is callable with `TickContext const&` fails both detection probes, reads as absent, and silently behaves as "always run".

```cpp
struct monthly_upkeep_system : System<monthly_upkeep_system> {
	static bool should_run(TickContext const& ctx) {
		return ctx.today.is_month_start(); // pure over deterministic state
	}

	void tick(TickContext const& ctx, treasury& t, upkeep const& u) {
		t.balance -= u.monthly_cost;
	}
};
```

### **Systems are stateless — all simulation state lives in components and singletons.**

WHY: system instances are not serialized. Anything you store on the system object (counters, caches with semantic meaning, accumulated totals) silently vanishes on save/load and is invisible to the full-state checksum — a desync and a save-corruption bug in one. This is why `should_run` is required to be `static`. Scratch buffers whose contents never affect results (e.g. a reused `reductions::KeyedSumScratch<T>`) are the only acceptable instance state.

### **Register every phase anchor — unmatched `run_after` / `run_before` ids are silently ignored.**

WHY (scheduler contract, documented in `src/openvic-simulation/ecs/SystemPhase.hpp`): `declared_run_after` / `declared_run_before` ids that match no **registered** system are dropped without error. A phase anchor you declared with `ECS_PHASE_ANCHOR` but forgot to `world.register_system<...>()` makes every edge through it vanish — systems that were supposed to be ordered now schedule freely. Register every anchor; order doesn't matter (the DAG sorts registration order out).

Three related hard rules from the same header:

- A hand-written anchor must derive from `PhaseAnchorSystem<Derived>` (or just use `ECS_PHASE_ANCHOR` / `ECS_PHASE_ANCHOR_FIRST`, which expand to exactly that). A bare `System<>` with an empty tick does **not** compile — `dispatch_serial_impl` `static_assert`s on an empty component pack — and an empty require set would otherwise match *every* archetype. `PhaseAnchorSystem` shadows `tick_all` with a no-op so no query is ever built.
- A system using `ECS_IN_PHASE(prev, next, extras...)` **cannot** also hand-write `declared_run_after` or `declared_run_before` — that is an in-class member redefinition and a hard compile error.
- A template-id extra dependency (e.g. `some_system<A, B>`) splits on the comma at the preprocessor level — use a type alias.

See [scheduling.md](scheduling.md).

---

## Threading

### **Never call `EcsThreadPool::parallel_for` from inside a system tick body.**

```cpp
template<typename Body>
void parallel_for(std::size_t chunk_count, Body&& body); // BLOCKING
```

(`src/openvic-simulation/ecs/EcsThreadPool.hpp` — hard invariant: "`parallel_for` is blocking — does not return until every chunk's body has run.")

WHY: the scheduler already dispatches the outer `parallel_for` around your stage — your tick body is (in general) *already running on a pool worker*. An inner `parallel_for` queues jobs and then blocks that worker waiting for them; the inner jobs sit unowned until **some other** worker picks them up. If every worker is in the same situation, nobody is left to run anything: deadlock.

Instead: the right tool inside a tick is straight-line code. If you need a deterministic parallel fold, use the `reductions::*` helpers from the top of a serial system — never from within a per-row/per-chunk tick body:

```cpp
namespace OpenVic::ecs::reductions {
	template<typename T, typename Body>
	T parallel_sum(EcsThreadPool& pool, std::size_t chunk_count, T init, Body&& body);
	// also: parallel_min, parallel_max, parallel_keyed_sum
}
```

See [threading-and-reductions.md](threading-and-reductions.md).

### **Inside a `SystemThreaded` tick, touch only your own row, `ctx.cmd`, and declared reads.**

WHY: chunks of a `SystemThreaded` run concurrently. Any shared mutable state — a member variable, a captured container, a singleton — is a data race between your own chunks. The safe surface is: the component references passed to your tick (rows are partitioned by chunk, so no two work items share one), read-only cross-archetype data declared via `extra_reads()`, and `ctx.cmd` (each chunk gets its own buffer). Writes to anything shared belong in a serial `System<>`.

### **Keep tick bodies noexcept — a throw terminates the process.**

WHY: pool invariant (`src/openvic-simulation/ecs/EcsThreadPool.hpp`): "No work is ever silently dropped; bodies that throw will std::terminate (we do not guarantee exception-safety from inside system bodies — they should be noexcept)." There is no recovery path; handle failure as data (error components, logged skips), not exceptions.

### **Never key results off `worker_id`.**

WHY: `parallel_for` passes a `worker_id` to each body, but which worker runs which chunk is scheduling noise — it varies run to run and with worker count. Anything observable keyed by `worker_id` (per-worker accumulators folded in worker order, worker-indexed output slots) is nondeterministic by construction. The ECS's own machinery is deliberately keyed by `chunk_idx` instead: per-chunk `CommandBuffer`s, chunk-indexed `reductions::*` buffers. `worker_id` is exposed for diagnostics and thread-local scratch *whose contents never affect results* — nothing else.

### **Guarantee (not a pitfall): stages mix `System<>` and `SystemThreaded` freely.**

The scheduler builds one flat work-item list across every system in a stage — N×chunks for each `SystemThreaded` plus one whole-tick item per plain `System<>` — and dispatches it via a single outer `parallel_for`. The historical "`SystemThreaded` inside a multi-system stage silently falls back to serial" footgun is gone, and the scheduler prewarms `World::query_cache` on the main thread before the parallel section, so workers only read the cache. You do not need to arrange your systems to avoid mixing. See [scheduling.md](scheduling.md).

---

## Determinism

The contract: lockstep multiplayer. Same starting `World` + same inputs must produce **bit-identical** state on every machine and at every worker count. `tests/src/ecs/WorkerCountInvariance.cpp` is the gate — a multi-tick digest at workers = 1, 2, 4, 8, 16 must match exactly. Full treatment in [determinism.md](determinism.md); the rules below are the ones you must not break.

### **No float math — `fixed_point_t` everywhere.**

WHY: IEEE float results vary with compiler, optimization level, FMA contraction, and architecture. Any `float`/`double` that feeds simulation state eventually diverges across peers. All simulation arithmetic uses `fixed_point_t`.

### **No non-deterministic globals — anywhere simulation state is computed.**

WHY: Wall-clock time, unseeded RNG, thread ids, pointer values, environment state — anything that varies per machine or per run — must never feed a tick body, a `should_run` predicate, or any other code that writes simulation state. The contract is the same one stated for `should_run` above, generalised: every value the simulation consumes must be a pure function of deterministic state (`ctx.today`, components, singletons). Nothing checks this — the worker-count gate catches violations only probabilistically, and a per-machine divergence surfaces as a lockstep desync ticks later.

### **Parallel folds go through `reductions::*` — no atomics, no thread-locals, no worker-order accumulation.**

WHY: the `reductions` helpers write per-chunk results into a `chunk_idx`-indexed buffer during the parallel section, then fold sequentially in `chunk_idx` ascending order after the join. The only operation order that affects the output is that sequential fold, which is independent of the pool — so the result is bit-identical across worker counts. An atomic accumulator or per-worker partial sums folded in completion/worker order encodes thread timing into your state.

### **`DenseSlotAllocator::allocate` / `release` are serial-only — never call them from a `SystemThreaded` tick body.**

```cpp
uint32_t allocate();
void release(uint32_t slot);
```

WHY (determinism contract, `src/openvic-simulation/ecs/DenseSlotAllocator.hpp`): the allocation order is a pure function of the alloc/release **call sequence** — LIFO reuse of released slots, then high-water growth. Inside a `SystemThreaded` tick the execution order is worker-count-dependent, so the call sequence (and therefore every slot assignment) differs per machine: a lockstep desync. Call `allocate` / `release` only from serial code — plain `System<>` tick bodies, `CommandBuffer`-apply-adjacent serial code, or outside ticks entirely. Same discipline as the deferred-create path: structural decisions funnel through a serial point.

### **Structural mutation order is deterministic because you used `ctx.cmd` — here is the order you can rely on.**

Per stage: each system's pending `CommandBuffer` is applied at the stage barrier in the stage's deterministic order (ascending `system_type_id_t`); within a `SystemThreaded`, per-chunk buffers are merged in `chunk_idx` ascending order before apply, and deferred placeholders are resolved to real `EntityID`s on a single thread in record order. Net effect: a threaded system spawning entities yields the identical end state — including identical id assignment — for any worker count.

Working example (adapted from the deferred-create cases in `tests/src/ecs/WorkerCountInvariance.cpp`):

```cpp
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct spawn_seed {
		int64_t seed = 0;
	};
	struct spawned_value {
		int64_t derived = 0;
	};
}
ECS_COMPONENT(spawn_seed, "game::spawn_seed")
ECS_COMPONENT(spawned_value, "game::spawned_value")

namespace {
	// Chunk-parallel spawner. ctx.cmd is a per-chunk buffer in parallel mode:
	// create_entity records the op and returns a deferred placeholder; real slots are
	// assigned at the stage barrier, on one thread, in chunk_idx-ascending merge order.
	struct spawner_system : SystemThreaded<spawner_system> {
		void tick(TickContext const& ctx, EntityID, spawn_seed const& s) {
			EntityID const placeholder =
				ctx.cmd.create_entity(ctx.world, spawned_value { s.seed * 31 + 7 });
			// placeholder.is_deferred() == true; only meaningful to further ops on
			// THIS buffer. Do not store it anywhere that outlives this tick call.
			(void) placeholder;
		}
	};
}
ECS_SYSTEM(spawner_system)

int64_t spawn_and_digest(uint32_t worker_count, std::size_t seed_count) {
	World world;
	world.set_ecs_worker_count(worker_count); // must precede the first tick_systems

	for (std::size_t i = 0; i < seed_count; ++i) {
		world.create_entity(spawn_seed { static_cast<int64_t>((i * 17) % 251 + 1) });
	}

	world.register_system<spawner_system>();
	world.tick_systems(Date {});

	int64_t digest = 0;
	world.for_each_with_entity<spawned_value>([&](EntityID e, spawned_value& s) {
		digest = digest * 1000003 + s.derived;
		digest ^= static_cast<int64_t>(e.to_uint64());
	});
	return digest;
}
// The determinism gate: spawn_and_digest(wc, 500) is identical for wc = 1, 2, 4, 8, 16.
```

For debugging a divergence, `World::set_serial_mode(bool enabled)` runs each stage's systems one at a time from the calling thread (pair it with `set_ecs_worker_count(1)` for fully single-threaded runs) — "parallel result == serial result" is the invariant to bisect against.

### **Anything sensitive to id assignment must iterate in id / dense-index order — never chunk order.**

WHY: the identity layer guarantees *same allocation-request sequence → same ids*. After a save/load round-trip, archetype **packing** may legitimately differ from the never-saved run (`restore_identity` documents this); chunk iteration order is packing order. So `cmd.create_entity` calls driven by chunk-order iteration will assign different ids post-load than the original run — a delayed desync that surfaces k ticks after loading. Iterate in `EntityID` or dense-index order wherever the iteration order feeds id assignment (or any other order-sensitive output). Related guarantee: `create_entities` (bulk) yields the *identical* end state — including identical `EntityID` assignment — as the equivalent `create_entity` loop, so switching to bulk creation never perturbs ids.

### **Snapshot identity only between ticks, and follow the loader contract to the letter.**

```cpp
bool snapshot_identity(WorldIdentitySnapshot& out) const;
bool restore_identity(WorldIdentitySnapshot const& snapshot);

template<typename... Cs>
bool restore_entity(EntityID eid, Cs&&... values);
```

WHY: `snapshot_identity` refuses (error log + `false`) when called mid-tick or while any reserved-but-unfinalised slot exists — i.e. a `CommandBuffer` holding un-applied creates. After `restore_identity` (which requires a fresh `World`), the **only** legal entity ops until the last live slot is finalised are `restore_entity` calls, one per live slot. In particular, `destroy_entity` on a not-yet-finalised id would push the slot onto the free list and silently corrupt the restored allocation order.

Separately from legality: recreate in **slot-index ascending order** — the canonical order that makes archetype packing reproducible across loads. Identity correctness is order-independent (an out-of-order `restore_entity` succeeds; the precheck validates the slot's reserved-but-unfinalised state and generation, not call order), but packing is not — and packing order is chunk iteration order, so anything chunk-order-sensitive (previous rule) inherits the divergence. See [world.md](world.md).

### **The name literal in `ECS_COMPONENT` / `ECS_SYSTEM` IS the persistent identity — globally unique, stable, namespace scope.**

```cpp
// src/openvic-simulation/ecs/ComponentTypeID.hpp:
ECS_COMPONENT(Type, NameLiteral) // component_type_id_of<Type>() == fnv1a_64(NameLiteral)

// src/openvic-simulation/ecs/SystemTypeID.hpp:
ECS_SYSTEM(Type) // system_type_id_of<Type>() == fnv1a_64(#Type) — the macro stringifies
                 // its argument, so the qualified type name you pass IS the literal
```

WHY: both ids are the FNV-1a 64-bit hash of the string literal, computed in pure `constexpr` — the same input string yields the same hash on every compiler and platform, so ids are byte-identical across builds. That hash is the identity everywhere ids persist or cross machines: saves, replays, the network protocol, and the `schedule_hash` handshake (next rule). The contracts stated in both headers:

- The literal must be **globally unique** within the simulation — it is the sole input to the id.
- The macro must be invoked at **namespace scope**, outside any other namespace: it opens `namespace OpenVic::ecs` itself. (The examples in this doc invoke it after closing their enclosing namespace — that is the pattern.)
- **Renaming the literal is a breaking change** to anything that persists the id: old saves and replays stop resolving, the network protocol mismatches, and a renamed system changes `schedule_hash`, so peers on mixed builds fail the join handshake. For `ECS_SYSTEM` the literal is the stringified type name, so renaming or re-qualifying the system type itself *is* the rename.

See [components.md](components.md) and [determinism.md](determinism.md).

### **`schedule_hash` is the multiplayer handshake — keep registrations identical across peers.**

```cpp
uint64_t schedule_hash();
```

WHY: peers compare this FNV-1a hash over the schedule's `(stage_index, system_type_id_t)` pairs at session-start; a mismatch rejects the join. The schedule is cross-machine deterministic *given identical registrations* — so every peer must register the same set of systems (including phase anchors). Per-tick cadence belongs in `should_run`, never in conditional registration.

---

## Toolchain

### **Forward-declare every type used in member-function signatures (MSVC parser cascade).**

WHY: if a header declares `void f(IncompleteType&)` and the `.cpp` includes it first, MSVC marks the function ill-formed and every following member access reports a bogus "static member functions do not have 'this' pointers". The fix is mechanical: forward-declare every type that appears in member-function signatures.
