# CommandBuffer

`CommandBuffer` is how game code performs structural changes — creating and destroying entities, adding and removing components — from inside a system tick. Recording an operation is cheap and immediate; the actual World mutation is deferred to the **stage barrier**, after every system in the current stage has finished iterating. Inside a system you never mutate the World structurally yourself: you call `ctx.cmd.*`, and the scheduler plays your buffer back at a safe, deterministic point.

Defined in [src/openvic-simulation/ecs/CommandBuffer.hpp](../../src/openvic-simulation/ecs/CommandBuffer.hpp) (recording) and [src/openvic-simulation/ecs/CommandBuffer.cpp](../../src/openvic-simulation/ecs/CommandBuffer.cpp) (playback).

## Where you get one

Every system tick receives a `TickContext` (see [systems.md](systems.md)):

```cpp
struct TickContext {
	World& world;
	Date today;
	CommandBuffer& cmd;
};
```

- For a plain `System<Derived>`, `ctx.cmd` is the system's own **pending buffer**, owned by the World.
- For a `SystemThreaded<Derived>`, `ctx.cmd` is a **per-chunk buffer** in parallel mode — each chunk being processed in parallel records into its own buffer, so recording is race-free without locks. See [Spawning from SystemThreaded](#spawning-from-systemthreaded) below.

You can also construct a `CommandBuffer` directly outside any tick (loaders, tests, tooling) and call `apply()` on it yourself — see [Standalone use](#standalone-use-outside-a-tick).

## Why direct `ctx.world` structural mutation is forbidden

Structural operations move entity rows between archetype chunks and swap-pop neighbours to keep columns dense (see [storage-model.md](storage-model.md)). Doing that while a `for_each` is walking those same columns invalidates the iteration in place; doing it while another system in the same stage iterates in parallel is a data race. So the rule is absolute:

> **Inside a tick, structural mutations go through `ctx.cmd`, never `ctx.world`.**

### The in-tick mutation guard

The World enforces this at runtime. While `tick_systems` is running, the four structural mutators — `World::create_entity` (and `create_entities`), `World::destroy_entity`, `World::add_component`, `World::remove_component` — check an internal in-tick flag. If called mid-tick they are **rejected as a no-op with an error log** (loud-but-no-crash): `create_entity` returns an invalid `EntityID {}`, `add_component` returns `nullptr`, `remove_component` returns `false`, `create_entities` returns `false` and fills `out_ids` with invalid ids. Your simulation keeps running, but the mutation did not happen — watch the error log and the failure return values.

At the stage barrier the scheduler flips an apply-phase flag so the guard yields, then drains each system's pending buffer. That is the *only* window in which structural ops execute during a tick. From `tests/src/ecs/InTickMutationGuard.cpp`:

```cpp
// Forbidden — the in-tick guard rejects this; entity never gets GuardTagB.
struct MisbehavingSystem : System<MisbehavingSystem> {
	void tick(TickContext const& ctx, EntityID eid, GuardTag&) {
		ctx.world.add_component<GuardTagB>(eid); // rejected by the guard — error log, no-op
	}
};

// Correct — deferred via the CommandBuffer, applied at the stage barrier.
struct WellBehavedSystem : System<WellBehavedSystem> {
	void tick(TickContext const& ctx, EntityID eid, GuardTag&) {
		ctx.cmd.add_component<GuardTagB>(eid);
	}
};
```

Reads (`get_component`, `has_component`, `is_alive`, `for_each`, singleton reads) are not blocked by the guard — only structural mutation is. Singleton and cross-archetype access still has to be *declared* via `extra_reads()` / `extra_writes()` so the scheduler can order you correctly; see [systems.md](systems.md) and [scheduling.md](scheduling.md).

## Execution timing and ordering guarantees

1. **Record order is replay order.** `apply()` drains the buffer front-to-back in the exact order operations were recorded.
2. **Stage barrier.** The scheduler applies buffers once per stage, after every system in the stage has finished ticking. Effects are therefore visible to the *next* stage, never within the stage that recorded them (and never within your own tick — `world.is_alive(eid)` stays `false` for an entity you just queued).
3. **Cross-system order within a stage.** At the barrier, each system's pending buffer is applied in the stage's deterministic topological emit order — ascending `system_type_id_t` within the stage. This is fixed, machine-independent, and independent of registration order (see [scheduling.md](scheduling.md)).
4. **SystemThreaded merge order.** Per-chunk buffers are spliced into the system's pending buffer in chunk-index ascending order — the deterministic `(archetype_idx, chunk_idx)` order, *not* the order workers happened to finish. Worker scheduling never affects replay order.

Together these make playback fully deterministic: the same recorded operations against the same World state produce the same archetype contents **and the same `EntityID` assignment**, regardless of worker count. Entity slots are popped from the free list one at a time in replay order, so two runs that record the same ops get bit-identical ids (verified by `tests/src/ecs/CommandBuffer.cpp` "Buffer applied to fresh world reproduces same EntityIDs in same order" and the worker-count sweep in `tests/src/ecs/SystemThreadedSpawn.cpp`). See [determinism.md](determinism.md) for the broader rules this feeds into.

## Recording operations

All recording functions are cheap: they capture the component value(s) into a type-erased payload (moved if you pass an rvalue) and push one op. Nothing touches archetypes until `apply()`.

### `create_entity`

```cpp
template<typename... Cs>
EntityID create_entity(World& world, Cs&&... values);
```

Queues creation of an entity with components `Cs...` (at least one — enforced by `static_assert`). Component values are moved/copied into the buffer **at record time**; move-only components (e.g. holding a `std::unique_ptr`) are supported.

The returned `EntityID` depends on the buffer's mode:

- **Serial mode** (plain `System<>`, standalone buffers — the default): a real entity slot is reserved in the World immediately, so the id is genuine and stable. However `world.is_alive(eid)` returns `false` — and `get_component` returns `nullptr` — until `apply()` finalises the entity.
- **Parallel mode** (per-chunk buffers inside `SystemThreaded`): no World mutation happens. You get a *deferred placeholder* — `EntityID { index = local_seq, generation = DEFERRED_GENERATION_BIT }`. The placeholder satisfies `is_valid()` and `is_deferred()`, fails `world.is_alive()`, and every World accessor treats it as "not present" (returns `nullptr` / `false` / `0` — safe, never out-of-bounds). It is resolved to a real `EntityID` at `apply()` time, single-threaded, in record order.

In **both** modes the returned id is accepted by other operations *on the same buffer* — `add_component`, `remove_component`, `destroy_entity` — so you can build the entity up across several calls in one recording. Do **not** store a parallel-mode placeholder anywhere that outlives the tick: it is never rewritten to the real id, and it is meaningless outside its own buffer. If you need a durable handle to a freshly spawned entity, either spawn from a serial system (real id immediately) or look the entity up next tick via a query.

```cpp
CommandBuffer cmd;
EntityID const eid = cmd.create_entity(world, CBA { 7 });
CHECK(eid.is_valid());
CHECK_FALSE(world.is_alive(eid)); // not alive until apply

cmd.apply(world);
CHECK(world.is_alive(eid));
CHECK(world.get_component<CBA>(eid)->v == 7);
```

> **Pre-attach every component the entity will need at `create_entity` time.** Adding a component afterwards — even via the same buffer — is one full archetype migration per `add_component` op at replay: every existing component of the entity is moved to a row in a different archetype. At spawn-heavy scales (hundreds of thousands of entities per tick) that is lethal. List all components in the `create_entity` call instead; that costs one row construction in the final archetype, total.

### `create_immutable_entity`

```cpp
template<typename... Cs>
ImmutableEntityID create_immutable_entity(World& world, Cs&&... values);
```

Deferred analogue of `World::create_immutable_entity` (see [entities.md](entities.md)). Identical mechanics to `create_entity`, but returns an `ImmutableEntityID`, which `add_component` / `remove_component` will not accept (compile-time guarantee — there are no overloads for it), and `apply()` stamps the finalised entity's slot immutable (runtime backstop: type-erased structural paths refuse to migrate it). Component *data* stays mutable through `get_component`. In parallel mode the returned handle is a deferred placeholder, same rules as above.

If a system only holds a plain `EntityID` (e.g. from `for_each_with_entity`) and the entity might be immutable, branch before queueing:

```cpp
if (!ctx.world.is_immutable(eid)) {
	ctx.cmd.add_component<C>(eid, ...);
}
```

Otherwise `apply()` refuses the op with an error log (`"CommandBuffer::apply refused add_component: entity ... is immutable"`) and skips it.

### `create_entities` / `create_immutable_entities` (bulk)

```cpp
template<typename... Cs, typename... Spans>
bool create_entities(
	World& world, std::size_t count, std::span<EntityID> out_ids, Spans&&... spans
);

template<typename... Cs, typename... Spans>
bool create_immutable_entities(
	World& world, std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
);
```

Deferred analogues of `World::create_entities` (see [entities.md](entities.md)): record **one** batch op instead of `count` individual creates — one contiguous heap-allocated block per non-tag component column per batch (plus one boxed batch payload), rather than one allocation per component per entity.

Contract (same as the World API):

- Pass one `std::span` per **non-empty** component, matched positionally to the non-empty `Cs...` in pack order; each span's length must equal `count`. Tag components are named in the pack but take no span. Pass *no* spans at all to default-construct every component.
- **Input spans are moved-from at record time.** The caller's storage is left holding moved-from husks; pass a copy if the source must survive. Spans of `const` elements are rejected at compile time so the move contract cannot silently degrade to a copy.
- `out_ids.size()` must equal `count`. On any length mismatch the call logs an error, records nothing, and returns `false`.
- `count == 0` records nothing and returns `true` (loop-equivalent: the archetype is not created either).

Handles written to `out_ids` follow the single-create rules per mode: serial mode reserves `count` real slots up-front in creation order (usable for same-buffer `add_component` / `destroy_entity`); parallel mode hands out `count` *sequential* deferred placeholders.

**Determinism guarantee:** a bulk create yields the identical end state — including identical `EntityID` assignment — as the equivalent `create_entity` loop. Real slots are assigned one-by-one in record order at replay, so you can freely convert a spawn loop to a bulk create without changing simulation results.

### `destroy_entity`

```cpp
inline void destroy_entity(EntityID id);
```

Queues destruction. At replay, `World::destroy_entity` is a no-op on already-dead entities and on stale generations, so double-destroys and races between two systems destroying the same entity are benign. Accepts:

- a real, live `EntityID`;
- an id returned by a same-buffer `create_entity` — a create followed by a destroy in the same buffer is a clean net no-op;
- a deferred placeholder from the same buffer (parallel mode).

To destroy entities found during iteration, record the destroy inside the loop — that is exactly what the buffer is for; never call `ctx.world.destroy_entity` mid-tick.

### `add_component`

```cpp
template<typename C>
void add_component(EntityID id, C&& value);

template<typename C>
void add_component(EntityID id); // default-construct
```

Queues adding component `C` to `id`. The value (or a default-constructed `C`) is captured into the buffer **at record time**. At replay:

- If the entity is dead, has a stale generation, or is an unresolved placeholder, the op is **silently dropped**.
- If the entity already carries `C`, the existing value is replaced in place (no migration).
- Otherwise the entity migrates to the archetype extended with `C` — the expensive path; see the pre-attach rule above.
- If the entity is immutable, the op is refused with an error log.

Tag (zero-size) components are first-class: `cmd.add_component<MyTag>(eid)` works, and adding a tag that is already present is safe.

Ordering note: only record an `add_component` against an entity whose creation is **earlier in the same buffer** (the natural pattern — `create_entity` returns the id you then add to) or already finalised in the World. An add that replays before the entity's create op has been applied (e.g. the create sits in a *different* system's buffer that applies later) is dropped — adding to a reserved-but-unfinalised slot is undefined-by-policy and ignored.

### `remove_component`

```cpp
template<typename C>
void remove_component(EntityID id);
```

Queues removal of `C` from `id`. At replay:

- Dead / stale / unresolved-placeholder ids: silently dropped.
- Entity doesn't carry `C`: silent no-op.
- `C` is the entity's **sole** component: refused (entities with zero components are not allowed — record `destroy_entity` instead), mirroring `World::remove_component`.
- Immutable entity: refused with an error log.

Removal is also an archetype migration (every remaining component moves to the shrunken archetype) — same cost caveat as `add_component`. If a flag flips frequently, prefer a data field over add/remove churn of a tag component; see [components.md](components.md).

### What is *not* on the CommandBuffer

There is **no singleton operation** on `CommandBuffer`. Singletons are created via `World::set_singleton` *before the first `tick_systems` call* (creating one mid-tick rehashes the singleton map under concurrent readers), and mutated in place through `world.get_singleton<C>()` from a **serial** system that declares the write in `extra_writes()`. See [world.md](world.md) and [systems.md](systems.md).

## Playback and buffer management

These are the non-recording members. Inside a system you never call them — the scheduler does. You call them yourself only on standalone buffers.

```cpp
void apply(World& world);
```

Drains the buffer onto the World in record order, then resets it (ops cleared, `deferred_count()` back to 0). Resolves any deferred placeholders to real ids first-come in record order, on the calling thread. The scheduler invokes this once per system per stage, in the stage's deterministic order (ascending `system_type_id_t`); for standalone buffers you call it yourself, **outside any tick**.

```cpp
void clear();
```

Resets the buffer *without* applying. Every queued payload is destroyed correctly via its column vtable (move-only component values are not leaked). After `clear()`, `op_count() == 0` and `empty() == true`.

```cpp
std::size_t op_count() const;
bool empty() const;
```

Introspection: number of queued ops / whether the buffer is empty.

### Driver-level members (do not call from game code)

```cpp
void set_parallel_mode(bool enabled);
bool parallel_mode() const;
void merge_from(CommandBuffer&& other);
uint32_t deferred_count() const;
```

`set_parallel_mode(true)` switches `create_entity` to placeholder-returning deferred mode; `SystemThreaded` sets it on every per-chunk buffer and clears it on the merged buffer before `apply()`. `merge_from` splices another buffer's ops onto this one's tail (rebasing placeholder indices so they stay unique; `other` is left empty); the scheduler uses it to combine per-chunk buffers in chunk-index ascending order. `deferred_count()` reports queued placeholder creates pending resolution. These exist for the scheduler and for tests that simulate it — game systems just use `ctx.cmd` and let the driver handle modes and merging.

## Spawning from SystemThreaded

A `SystemThreaded` tick body may call `ctx.cmd.create_entity` freely — `ctx.cmd` is that chunk's private buffer in parallel mode, so parallel recording needs no synchronisation. The pipeline is: parallel per-chunk record → chunk-index-ascending merge → single-threaded resolution and apply at the stage barrier. The resulting entity finalisation order — and therefore id assignment and chunk packing — is **worker-count-invariant**.

Complete pattern, adapted from `tests/src/ecs/SystemThreadedSpawn.cpp` — a threaded spawner plus a downstream serial system that observes the spawned entities one stage later:

```cpp
struct STSSource {
	int32_t id = 0;
};
struct STSSpawnCount {
	int32_t count = 0;
};
struct STSSpawned {
	int32_t source_id = 0;
};
struct STSSpawnedTotal {
	int64_t value = 0;
};

ECS_COMPONENT(STSSource, "game::Source")
ECS_COMPONENT(STSSpawnCount, "game::SpawnCount")
ECS_COMPONENT(STSSpawned, "game::Spawned")
ECS_COMPONENT(STSSpawnedTotal, "game::SpawnedTotal")

// Chunk-parallel: each row queues `count` deferred creates into its chunk's buffer.
struct STSSpawnSystem : SystemThreaded<STSSpawnSystem> {
	void tick(TickContext const& ctx, EntityID, STSSource const& src, STSSpawnCount const& sc) {
		for (int i = 0; i < sc.count; ++i) {
			ctx.cmd.create_entity(ctx.world, STSSpawned { src.id });
		}
	}
};

// Serial, sequenced after the spawner: by the time this stage runs, the stage
// barrier has applied the spawner's buffer, so the new entities are queryable.
struct STSSpawnedCounter : System<STSSpawnedCounter> {
	void tick(TickContext const& ctx, STSSpawned const&) {
		STSSpawnedTotal* total = ctx.world.get_singleton<STSSpawnedTotal>();
		if (total != nullptr) {
			total->value += 1;
		}
	}

	static constexpr std::array<system_type_id_t, 1> declared_run_after() {
		return { system_type_id_of<STSSpawnSystem>() };
	}
};

ECS_SYSTEM(STSSpawnSystem)
ECS_SYSTEM(STSSpawnedCounter)
```

(The counter also needs its singleton write declared via `extra_writes()` in real game code, and the singleton created before the first tick — see [systems.md](systems.md).)

Rules specific to threaded spawning:

- The `EntityID` returned inside a `SystemThreaded` tick is a deferred placeholder. Use it only for follow-up ops on the same `ctx.cmd` within the same row's processing; never store it.
- Pre-attach all components in the `create_entity` call (the pitfall above applies doubly here — a spawn loop is exactly where per-entity migrations explode).
- The spawned entities become visible to queries in the next stage, not later in the same stage.
- `extra_writes()` is forbidden on `SystemThreaded` (self-race); the buffer is the only mutation channel a threaded system has besides its declared per-row writes. See [threading-and-reductions.md](threading-and-reductions.md).

## Standalone use outside a tick

Outside `tick_systems` you may construct and apply a buffer directly — useful when you want to iterate and mutate without invalidating the iteration, e.g. tagging every match of a query. Adapted from `tests/src/ecs/CommandBuffer.cpp`:

```cpp
World world;
world.create_entity(CBA { 1 });
world.create_entity(CBA { 2 });
world.create_entity(CBA { 3 });

CommandBuffer cmd;
world.for_each_with_entity<CBA>([&](EntityID e, CBA&) {
	cmd.add_component<CBB>(e, CBB { 100 });
});
// Nothing has changed yet — iteration stayed valid throughout.

cmd.apply(world);
// Now every CBA entity also carries CBB { 100 }.
```

You could call `world.add_component` directly between ticks instead (the in-tick guard only blocks mid-tick calls), but not from inside the `for_each` — the buffer is what makes mutate-while-iterating safe anywhere.

## Pitfall summary

- **`ctx.cmd`, never `ctx.world`, for structural ops inside a tick.** The in-tick guard turns direct calls into logged no-ops; the mutation does not happen — watch the error log and check the failure return values.
- **Pre-attach components at `create_entity` time.** Each deferred `add_component` that actually adds (rather than replaces) is a full archetype migration at replay.
- **Effects land at the stage barrier.** Your own tick — and every other system in the same stage — sees the pre-mutation World. Sequence an observer after the mutator (`declared_run_after` / phases, see [scheduling.md](scheduling.md)).
- **Deferred placeholders don't outlive the buffer.** Inside `SystemThreaded`, the id from `create_entity` is a placeholder; it is never rewritten and is meaningless after the tick. Re-find spawned entities via queries, or spawn from a serial system if you need the real id immediately.
- **`apply()` is a structural mutation.** Component pointers cached before the stage barrier follow the usual invalidation rule from [components.md](components.md): any create/destroy/add/remove on that archetype invalidates them.
- **Silent-drop semantics are deliberate.** Ops against dead or stale entities are skipped quietly (so cross-system destroy races are benign); in-tick direct-World refusals, immutable-entity refusals and size mismatches log errors. Watch the error log during development.

## Source files

- [src/openvic-simulation/ecs/CommandBuffer.hpp](../../src/openvic-simulation/ecs/CommandBuffer.hpp) — recording API, op storage, payload holders
- [src/openvic-simulation/ecs/CommandBuffer.cpp](../../src/openvic-simulation/ecs/CommandBuffer.cpp) — `apply` / `clear` / `merge_from` playback
- [src/openvic-simulation/ecs/World.hpp](../../src/openvic-simulation/ecs/World.hpp) — in-tick mutation guard, reserved-slot lifecycle, `is_immutable`
- [src/openvic-simulation/ecs/System.hpp](../../src/openvic-simulation/ecs/System.hpp) — `TickContext` (the `cmd` member), per-chunk buffer pool on `SystemThreaded`
- [src/openvic-simulation/ecs/EntityID.hpp](../../src/openvic-simulation/ecs/EntityID.hpp) — `is_deferred()`, `DEFERRED_GENERATION_BIT`, `ImmutableEntityID`
- Tests: [tests/src/ecs/CommandBuffer.cpp](../../tests/src/ecs/CommandBuffer.cpp), [tests/src/ecs/InTickMutationGuard.cpp](../../tests/src/ecs/InTickMutationGuard.cpp), [tests/src/ecs/SystemThreadedSpawn.cpp](../../tests/src/ecs/SystemThreadedSpawn.cpp)
