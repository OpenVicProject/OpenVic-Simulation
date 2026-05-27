# World

`World` is the single entry point to the ECS: it owns every entity, archetype, chunk, singleton, registered system and the scheduler that drives them. Game code creates one `World` per session, populates it with entities and singletons, registers systems once, and then calls `tick_systems` once per simulation day. Everything else in the ECS — queries, command buffers, systems — operates *through* a `World&`.

This page is the API reference for what you call on a `World` and the contracts you get back. For the overall mental model start at [storage-model.md](storage-model.md) (how the World physically stores data) and [pitfalls.md](pitfalls.md) (the consolidated rules); deep dives live in the sibling docs: [entities.md](entities.md), [components.md](components.md), [queries.md](queries.md), [systems.md](systems.md), [command-buffer.md](command-buffer.md), [determinism.md](determinism.md).

---

## Constructing a World

```cpp
World();
~World();
World(World const&) = delete;
World& operator=(World const&) = delete;
World(World&&) = delete;
World& operator=(World&&) = delete;
```

A `World` is default-constructed, non-copyable and non-movable. Create it once and pass it by reference (`World&` / `World const&`). The destructor tears everything down in order: registered system instances, all live component data (running component destructors), singletons, then the chunk pool — you never free ECS storage manually. See the test `"World destructor reclaims entities and singletons"` in tests/src/ecs/Coverage.cpp.

```cpp
void set_ecs_worker_count(uint32_t count);
```

Overrides the worker count used by the scheduler's thread pool. **Call it before the first `tick_systems` invocation.** `0` (the default) means hardware concurrency capped at 16. Determinism does not depend on this value — the worker-count-invariance gate (see [determinism.md](determinism.md)) requires identical results at every worker count — but changing it mid-session resets the pool.

```cpp
EcsThreadPool& ecs_thread_pool();
```

Returns the World-owned thread pool, lazily constructed on first access. You rarely need it directly; never call `EcsThreadPool::parallel_for` from inside a system tick body (see [threading-and-reductions.md](threading-and-reductions.md) for why this deadlocks).

---

## Entity operations (summary)

Full coverage, including `EntityID` semantics, immutable entities and bulk creation, is in [entities.md](entities.md).

```cpp
template<typename... Cs>
EntityID create_entity(Cs&&... values);

template<typename... Cs>
ImmutableEntityID create_immutable_entity(Cs&&... values);

template<typename... Cs, typename... Spans>
bool create_entities(std::size_t count, std::span<EntityID> out_ids, Spans&&... spans);

template<typename... Cs, typename... Spans>
bool create_immutable_entities(
	std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
);

void destroy_entity(EntityID id);
void destroy_entity(ImmutableEntityID id);
bool is_alive(EntityID id) const;
bool is_alive(ImmutableEntityID id) const;
bool is_immutable(EntityID id) const;
bool is_immutable(ImmutableEntityID id) const;
```

Key contracts:

- **`create_entity` requires at least one component** (compile error otherwise) and there is no zero-component entity state — `remove_component` refuses to strip the last component; `destroy_entity` instead.
- **Pre-attach every component the entity will ever need at creation time.** Each `Cs...` becomes a column in the entity's archetype. Adding a component later (`add_component`) migrates the entity to a different archetype: every existing component is move-constructed into the new archetype's chunks and the vacated row is back-filled by swap-pop. One call is fine; per-entity-per-tick calls are lethal at scale.
- Component order at the call site does not matter — the signature is sorted internally, so `create_entity(A {}, B {})` and `create_entity(B {}, A {})` land in the same archetype.
- `destroy_entity` on a dead or stale id is a safe no-op. `is_alive` is the universal validity check; a destroyed slot's generation bumps on reuse, so stale `EntityID`s reliably fail it.
- `is_immutable(id)` is true only for a *live* entity created via `create_immutable_entity` / `CommandBuffer::create_immutable_entity`. Use it when a plain `EntityID` surfaces for an entity that might be immutable (e.g. from `for_each_with_entity`) before queueing a structural op.
- **Determinism:** `EntityID` assignment is purely a function of the create/destroy sequence (free-list pops, then fresh growth). `create_entities` is guaranteed to yield the *identical* end state — including identical `EntityID` assignment and row packing — as the equivalent `create_entity` loop, with one exception: column versions' numeric values are bumped once per touched chunk instead of once per row. Versions only signal change, so never compare `component_version_in` values across the two paths. This equivalence is load-bearing for save/load and checksums; see the identity-snapshot section below.
- Bulk-create input spans are **moved-from** (the caller's storage is left holding moved-from husks); spans of `const` elements are rejected at compile time. `count == 0` is a no-op returning `true`. Size mismatches return `false` with an error log.

`World` also exposes `reserve_entity_slot`, `finalize_reserved_entity`, `finalize_reserved_entities_bulk` and `drop_reserved_slot`. These are plumbing for `CommandBuffer`'s deferred creates ([command-buffer.md](command-buffer.md)); game code never calls them directly.

---

## Component operations (summary)

Full coverage in [components.md](components.md).

```cpp
template<typename C>
C* get_component(EntityID id);

template<typename C>
C const* get_component(EntityID id) const;

template<typename C>
bool has_component(EntityID id) const;

template<typename C>
C* add_component(EntityID id, C&& value);

template<typename C>
C* add_component(EntityID id); // default-construct

template<typename C>
bool remove_component(EntityID id);

template<typename C>
uint64_t component_version_in(EntityID id) const;
```

`get_component`, `has_component`, `is_alive`, `destroy_entity`, `is_immutable` and `component_version_in` all have `ImmutableEntityID` overloads. There is deliberately **no** `add_component` / `remove_component` overload for `ImmutableEntityID` — that absence is the compile-time immutability guarantee ([entities.md](entities.md)).

Key contracts:

- `get_component` returns `nullptr` for a dead entity or an entity whose archetype lacks `C`. For **tag (zero-size) components it returns `nullptr` by design** — tags have no column storage; archetype membership *is* the data. Use `has_component<Tag>(eid)`.
- **Component pointers are short-lived.** The pointer is valid only until the next structural change touching that archetype: `create_entity`, `destroy_entity`, `add_component` or `remove_component` can swap-pop another row into yours or relocate your row to a different chunk. Across ticks, hold an `EntityID` (or `CachedRef<C>`, see [entities.md](entities.md)) and re-resolve. `component_version_in<C>(id)` returns the monotonically increasing version of `C`'s column in the entity's current archetype (0 if dead / not carried) — a stable version implies cached pointers into that column are still valid.
- `add_component` on an entity that already carries `C` replaces the value **in place** and returns the existing pointer — no migration. Otherwise it migrates the entity (expensive, see above) and returns the new component's address, or `nullptr` if the entity is dead or immutable. For tag types the migration happens but the returned pointer is `nullptr` (no data to point at).
- `remove_component` returns `false` if the entity is dead, immutable, doesn't carry `C`, or `C` is its only component.
- All four structural mutators (`create_entity`, `destroy_entity`, `add_component`, `remove_component`) are **refused during a tick** — see "The in-tick mutation guard" below. Inside systems, use `ctx.cmd` instead.

---

## Iteration

```cpp
// Visit every entity whose archetype contains all of Cs..., calling fn(C&...) per row.
template<typename... Cs, typename Fn>
void for_each(Fn&& fn);

// Same, but the function also receives the EntityID.
template<typename... Cs, typename Fn>
void for_each_with_entity(Fn&& fn);

// Query overloads — require Query::require_ids, reject archetypes overlapping
// Query::exclude_ids.
template<typename... Cs, typename Fn>
void for_each(Query const& query, Fn&& fn);

template<typename... Cs, typename Fn>
void for_each_with_entity(Query const& query, Fn&& fn);

// Chunk-granularity iteration: fn receives a ChunkView<Cs...>.
template<typename... Cs, typename Fn>
void for_each_chunk(Fn&& fn);

template<typename... Cs, typename Fn>
void for_each_chunk(Query const& query, Fn&& fn);
```

All six require at least one component type (`static_assert`). The non-`Query` forms are sugar: they build a `Query` from `Cs...` (`q.with<Cs...>().build()`) and forward to the `Query` overload.

Rules and contracts:

- **The lambda signature must match the `Cs...` template arguments**: `fn(C&...)` for `for_each`, `fn(EntityID, C&...)` for `for_each_with_entity`. With an explicit `Query`, the exclude-set is *not* reflected in the call signature — you pass `for_each<IA>(q, ...)` even if `q` excludes other components.
- The `Query` must have had `build()` called before being passed in ([queries.md](queries.md)). Match results are cached per `(require, exclude)` key and invalidated automatically whenever a new archetype is created, so reusing a `Query` across calls and ticks is cheap and always sees newly created archetypes.
- **No structural mutation during iteration.** Destroying or migrating an entity mid-loop swap-pops rows under the iterator and invalidates the hoisted column pointers. Collect `EntityID`s with `for_each_with_entity` and apply the changes after the loop (example below). Nested *read-only* `for_each` calls are fine.
- Tag components may appear in `Cs...` for matching, but the `C&` your lambda receives for a tag refers to a shared static dummy instance — never write through it, and never take its address.
- `for_each_chunk` hands you a `ChunkView<Cs...>` exposing the raw `EntityID` slab (`view.entities()`) and one contiguous component slab per `Cs...` (`view.array<C>()`), all of length `view.count()`. Tag slabs are `nullptr` — don't dereference. The view is valid **only inside the callback**. Use it when an inner loop wants tight, function-call-free, SIMD-friendly access ([storage-model.md](storage-model.md) explains why slabs are contiguous).
- Determinism: iteration visits matched archetypes in creation order and rows in chunk order. Chunk order (packing) is scrambled by swap-pop churn and is **not** save-stable — anything whose *output depends on visit order* (notably id assignment via `ctx.cmd.create_entity`) must iterate in id / dense-index order instead. See the loader contract below and [determinism.md](determinism.md).

### Example: query-filtered iteration, then deferred destroys

Adapted from tests/src/ecs/Iteration.cpp and tests/src/ecs/Integration.cpp.

```cpp
struct Wealth {
	int32_t cents = 0;
};
struct Bankrupt {}; // tag

ECS_COMPONENT(Wealth, "game::Wealth")
ECS_COMPONENT(Bankrupt, "game::Bankrupt")

// Visit every Wealth entity NOT tagged Bankrupt. Build the Query once, reuse it.
Query solvent;
solvent.with<Wealth>().exclude<Bankrupt>().build();

world.for_each<Wealth>(solvent, [](Wealth& w) {
	w.cents += 10;
});

// Destroying during iteration is forbidden (swap-pop invalidates the columns
// being walked) — collect ids first, destroy after the loop.
std::vector<EntityID> to_destroy;
world.for_each_with_entity<Wealth>([&](EntityID eid, Wealth& w) {
	if (w.cents < 0) {
		to_destroy.push_back(eid);
	}
});
for (EntityID eid : to_destroy) {
	world.destroy_entity(eid);
}
```

### Example: chunk-granular iteration

```cpp
world.for_each_chunk<Position, Velocity>([](ChunkView<Position, Velocity> view) {
	Position* OV_RESTRICT pos = view.array<Position>();
	Velocity const* OV_RESTRICT vel = view.array<Velocity>();
	std::size_t const n = view.count();
	for (std::size_t row = 0; row < n; ++row) {
		pos[row].x += vel[row].dx;
		pos[row].y += vel[row].dy;
	}
});
```

---

## Singletons

A singleton is a per-type unique value owned by the World — the right home for global simulation state that doesn't belong on any particular entity (a clock, a registry, a per-session config blob). Singletons are *not* entities: they live outside all archetypes and are never visited by `for_each`.

```cpp
template<typename C>
C* set_singleton(C&& value);

template<typename C>
C* set_singleton(); // default-construct

template<typename C>
C* get_singleton();

template<typename C>
C const* get_singleton() const;

template<typename C>
bool clear_singleton();
```

Contracts:

- `set_singleton` creates the singleton if absent; if it already exists, it **assigns into the existing object and returns the existing pointer** — pointer identity is preserved across re-sets.
- Unlike component pointers, a singleton pointer is **stable**: valid until `clear_singleton<C>()` or World destruction. Singletons survive `clear_systems` and live for the World's lifetime.
- `get_singleton` returns `nullptr` if the singleton was never set. `clear_singleton` returns `false` if absent.
- **Singleton access inside a system tick MUST be declared.** Singletons share `component_type_id_t` space with components but are invisible in any tick signature. A system that calls `get_singleton<C>()` in its tick must declare `extra_reads() = { component_type_id_of<C>() }`; one that mutates through the returned pointer must declare `extra_writes()` (serial `System<>` only — `register_system` rejects `extra_writes` on `SystemThreaded` at compile time, because its chunks run concurrently and would race the system with itself). Undeclared access lets the scheduler co-schedule a conflicting reader/writer pair and silently break worker-count-invariant determinism — the determinism gate catches it only probabilistically. See [systems.md](systems.md).
- **Create every singleton before the first `tick_systems` call.** `set_singleton` on a missing type inserts into the World's singleton map; doing that mid-tick can rehash the map under a concurrent reader even when access is correctly declared.

---

## Systems and ticking

Writing systems (tick signatures, `SystemAccess`, `should_run`, `SystemThreaded`, `ChunkSystem`) is covered in [systems.md](systems.md); ordering and phases in [scheduling.md](scheduling.md). This section covers the World-side registry.

### register_system

```cpp
template<typename S, typename... Args>
SystemHandle register_system(Args&&... args);
```

Constructs an instance of the concrete system type `S` from `Args...` and stores it type-erased in the World's registry. The World owns the instance (destroyed by `unregister_system`, `clear_systems` or `~World`) along with a dedicated pending `CommandBuffer` for it. Registration marks the schedule dirty; the `SystemScheduler` rebuilds its DAG and stage layout lazily on the next `tick_systems` (or `schedule_hash`) call.

Compile-time checks enforced here:

- `SystemThreaded` must not declare `extra_writes()` (self-race, see above).
- If `S` declares `should_run`, it must be a single, non-overloaded **static** function with exactly the signature `static bool should_run(TickContext const&)` (a `noexcept` qualifier is additionally allowed). Anything else — a member function, a data member, an overload set, wrong parameters or return type — is a hard compile error rather than being silently ignored.

Determinism: given the same set of registered systems, the built schedule is identical cross-machine — registration order does not affect it (see [scheduling.md](scheduling.md)).

### SystemHandle

```cpp
struct SystemHandle {
	uint32_t index = 0;
	uint32_t generation = 0;

	constexpr bool operator==(SystemHandle const& rhs) const;
	constexpr bool operator!=(SystemHandle const& rhs) const;
	// Generation 0 is the invalid sentinel — valid handles always have generation >= 1.
	constexpr bool is_valid() const;
};

inline constexpr SystemHandle INVALID_SYSTEM_HANDLE = {};
```

Defined in src/openvic-simulation/ecs/System.hpp. `is_valid()` only distinguishes a default-constructed handle (`INVALID_SYSTEM_HANDLE`, generation 0) from one actually returned by `register_system` — it does **not** detect staleness, so a handle whose system was since unregistered still returns `true`. Staleness is caught by `unregister_system` itself: destroying a system bumps the registry slot's generation, and an incoming handle is checked against the slot's alive flag and current generation, so a second `unregister_system` with a stale handle is a safe no-op instead of touching the wrong slot. Don't use `handle.is_valid()` to ask "is this system still registered?" — there is no API for that; just hand the handle to `unregister_system` when you're done with it.

```cpp
void unregister_system(SystemHandle handle);
void clear_systems();
```

`unregister_system` validates the handle (invalid/stale handles are safe no-ops), destroys the system instance and marks the schedule dirty. `clear_systems` destroys every registered system and its pending command buffer. Neither touches entities or singletons.

### tick_systems and the TickContext flow

```cpp
void tick_systems(Date today);
void tick_systems(TickContext const& ctx);
```

**Use the `Date` overload.** The `TickContext const&` overload exists only for source compatibility: it ignores the supplied context entirely (including its date) and forwards to `tick_systems(Date {})` — every system now gets its own World-owned pending `CommandBuffer`, so an externally constructed context has nothing to contribute.

One `tick_systems(today)` call is one simulation tick. The observable flow:

1. If any `register_system` / `unregister_system` / `clear_systems` happened since the last tick, the scheduler rebuilds its stage schedule first (declared `run_after`/`run_before` edges plus auto-resolved access conflicts — [scheduling.md](scheduling.md)).
2. Stages run in order. Within a stage, systems may run in parallel; each system's per-row `tick` receives a `TickContext`:

   ```cpp
   struct TickContext {
   	World& world;
   	Date today;
   	CommandBuffer& cmd;
   };
   ```

   `ctx.world` is for reads (declared in your access set); `ctx.cmd` is the **only** legal route for structural mutations from inside a tick ([command-buffer.md](command-buffer.md)). Systems with a `should_run` gate that returns `false` for this tick are skipped at dispatch time (the schedule itself is untouched).
3. At the end of each stage, the buffered command-buffer ops recorded by that stage's systems are applied to the World — later stages observe them.
4. After the last stage, the chunk pool's aging clock advances (memory residency only; never affects simulation results).

`tick_systems` is **not reentrant**: the whole run is "in-tick" for the mutation guard below, so don't call any World lifecycle API from inside a tick body.

### The in-tick mutation guard

While `tick_systems` is running, direct structural mutation through `World` is refused so that the only mutation path is the deterministic, stage-boundary `CommandBuffer` apply. A refused call logs an error and early-returns its failure value:

| call | mid-tick result |
|---|---|
| `create_entity` / `create_immutable_entity` | returns `EntityID {}` / `ImmutableEntityID {}` (invalid) |
| `create_entities` / `create_immutable_entities` | returns `false`, `out_ids` filled with invalid ids |
| `destroy_entity` | no-op |
| `add_component` | returns `nullptr` |
| `remove_component` | returns `false` |
| `restore_entity` | returns `false` |
| `snapshot_identity` / `restore_identity` | returns `false` + error log |

Reads (`get_component`, `has_component`, `is_alive`, `get_singleton`, `for_each` over your declared access) remain available inside a tick.

### Schedule introspection

```cpp
uint64_t schedule_hash();
std::size_t debug_stage_count();
std::size_t debug_stage_index_of(system_type_id_t type_id);
void set_serial_mode(bool enabled);
ChunkPool& chunk_pool();
```

- `schedule_hash` — FNV-1a hash over the `(stage_index, system_type_id_t)` pairs of the current schedule (rebuilding it first if dirty). Production use: multiplayer peers compare it at session-start handshake; a mismatch rejects the join.
- `debug_stage_count` / `debug_stage_index_of` — test/introspection only; `debug_stage_index_of` returns `SIZE_MAX` for an unscheduled system. Tests use these to assert two systems do (or don't) share a stage.
- `set_serial_mode(true)` disables stage-level (inter-system) parallelism — each stage's systems run one at a time from the calling thread, though a `SystemThreaded` still parallelises over its own chunks unless the pool has one worker (see [scheduling.md](scheduling.md)). Used by tests to validate "parallel result == serial result". Default `false`.
- `chunk_pool()` — test/introspection access to the per-World `ChunkPool`; production code never needs it.

### Example: a complete session skeleton

Adapted from tests/src/ecs/Integration.cpp.

```cpp
struct Position {
	int32_t x = 0;
	int32_t y = 0;
};
ECS_CHECKSUM_BYTES(Position)

struct Velocity {
	int32_t dx = 0;
	int32_t dy = 0;
};
ECS_CHECKSUM_BYTES(Velocity)

struct GameClock {
	bool paused = false;
};

ECS_COMPONENT(Position, "game::Position")
ECS_COMPONENT(Velocity, "game::Velocity")
ECS_COMPONENT(GameClock, "game::GameClock")

struct MovementSystem : System<MovementSystem> {
	// The tick body reads the GameClock singleton. That access is invisible in the
	// tick signature, so it MUST be declared or the scheduler may co-schedule a
	// conflicting writer (see systems.md).
	static constexpr std::array<component_type_id_t, 1> extra_reads() {
		return { component_type_id_of<GameClock>() };
	}

	void tick(TickContext const& ctx, Position& p, Velocity const& v) {
		GameClock const* clock = ctx.world.get_singleton<GameClock>();
		if (clock->paused) {
			return;
		}
		p.x += v.dx;
		p.y += v.dy;
	}
};
ECS_SYSTEM(MovementSystem)

void run_session() {
	World world;

	// Singletons first — never create one mid-tick.
	world.set_singleton<GameClock>();

	// Pre-attach every component at creation; add_component later means a migration.
	EntityID const mover = world.create_entity(Position { 0, 0 }, Velocity { 1, 2 });
	EntityID const idle = world.create_entity(Position { 50, 50 });

	world.register_system<MovementSystem>();

	Date const today { 1836, 1, 1 };
	for (int day = 0; day < 5; ++day) {
		world.tick_systems(today);
	}

	Position const* p = world.get_component<Position>(mover);
	// p->x == 5, p->y == 10; `idle` has no Velocity and was never visited.
	(void) p;
	(void) idle;
}
```

---

## Session cleanup

There is no `World::end_game_session`. The project's migration convention is that **every runtime archetype's owning module contributes an `end_game_session` sweep**: collect its entities with `for_each_with_entity`, then `destroy_entity` each (and `reset()` any `DenseSlotAllocator` side tables — [components.md](components.md)). `clear_systems` handles the system half; singletons either persist by design or are explicitly `clear_singleton`-ed. Destroying the `World` itself is always a complete, leak-free teardown — the sweeps exist so a *reused* World starts the next session from a deterministic, empty state.

---

## Identity snapshots — EntityID-stable save/load

The save/load contract is split in two: the **identity layer** (which `EntityID` means what) is captured and restored by the World; the **data layer** (each entity's component values, singleton values) is serialized by the caller in whatever format it likes. This split exists because chunk packing legitimately differs between a saved run and a restored run — only identity must be preserved bit-exactly.

```cpp
struct WorldIdentitySnapshot {
	struct SlotRecord {
		uint32_t generation = 0;
		bool immutable = false;

		bool operator==(SlotRecord const&) const = default;
	};

	// Indexed by slot index; size == entity_slots.size() at snapshot time.
	std::vector<SlotRecord> slots;
	// Free slots in pop order: free_list.front() is the slot the next allocation reuses.
	std::vector<uint32_t> free_list;

	bool operator==(WorldIdentitySnapshot const&) const = default;
};
```

A plain serializable struct — no IO or format attached. Equality is defaulted, so round-trip tests can compare snapshots directly.

### What is preserved, and how EntityIDs stay stable

Captured: per-slot generations, per-slot immutability, and the free-list order. That is exactly enough to guarantee:

- Every live entity is addressable at its **original `(index, generation)`** after restore.
- Every stale/destroyed `EntityID` from before the save **stays dead** after restore (generations are preserved, including slots that were reused and re-freed).
- Subsequent allocations (direct `create_entity` or deferred `cmd.create_entity`) **continue exactly as in the never-saved run** — same free-list pops in the same order, same generation bumps (the bump happens at allocate time, identically in both runs), then identical fresh growth. The tests `"restored allocator continues exactly as the never-saved run"` (tests/src/ecs/IdentitySnapshot.cpp) and the digest gate in tests/src/ecs/IdentitySnapshotInvariance.cpp pin this end to end.

Deliberately **not** captured: archetypes, chunks, rows, packing, component data, singletons, systems, query caches. The loader rebuilds component data via `restore_entity`; singletons and systems are re-established by normal session setup.

### snapshot_identity

```cpp
bool snapshot_identity(WorldIdentitySnapshot& out) const;
```

Captures the identity layer. Refuses (error log + `false`, `out` left cleared) when:

- called mid-tick — snapshot only between ticks;
- any reserved-but-unfinalised slot exists, i.e. a `CommandBuffer` holds un-applied creates — apply every buffer before saving (see the test `"snapshot refuses while a CommandBuffer holds un-applied creates"`);
- the free chain is corrupt (cycle, live node, unreachable dead slot) — it is better to refuse a save than to diverge k ticks after load.

### restore_identity

```cpp
bool restore_identity(WorldIdentitySnapshot const& snapshot);
```

Rebuilds the identity layer. Precondition: a **fresh** `World` (no entity slot ever allocated), outside any tick. The snapshot is validated fully before any mutation, so on failure (error log + `false`) the World is untouched. Postcondition: every live slot is reserved-but-unfinalised — addressable at its original `(index, generation)` but `is_alive == false` until `restore_entity` finalises it; free slots carry their restored generations and the exact saved chain order.

### restore_entity

```cpp
template<typename... Cs>
bool restore_entity(EntityID eid, Cs&&... values);
```

Finalises one restored slot with the given components — same component rules as `create_entity` (at least one component, order-insensitive). Validates that `eid` names a reserved-but-unfinalised slot with a matching generation; returns `false` + error log on any mismatch (deferred id, out of range, dead slot, stale generation, already finalised). Not callable mid-tick.

The slot's restored `immutable` flag is preserved — finalization *is* creation here, and immutability means "no archetype change after creation". There is deliberately no `ImmutableEntityID` overload: immutability is restored as data by `restore_identity`. Rebuild strong handles afterwards as `ImmutableEntityID { index, generation }`.

### Loader contract

1. Between `restore_identity` and the last `restore_entity` call, the **only** legal entity ops are `restore_entity` calls, one per live slot. In particular, `destroy_entity` on a not-yet-finalised id would push the slot onto the free list and silently corrupt the restored order.
2. **Canonical recreation order is slot-index ascending.** Identity correctness is order-independent, but chunk packing is not — the canonical order is what makes packing reproducible across loads.
3. The identity layer guarantees *same allocation-request sequence → same ids*. Producing the same request sequence post-load is **your** obligation: a system that does chunk-order-driven `cmd.create_entity` over an archetype whose packing differs from the saved run WILL assign different ids. Anything id-assignment-sensitive must iterate in id / dense-index order, never chunk order ([determinism.md](determinism.md)).
4. Query caches and the chunk pool need no attention — caches rebuild lazily as `restore_entity` recreates archetypes, and pool aging affects memory residency only.

### Example: save and load

Adapted from tests/src/ecs/IdentitySnapshot.cpp and tests/src/ecs/IdentitySnapshotInvariance.cpp.

```cpp
struct Treasury {
	int64_t cents = 0;
};
ECS_CHECKSUM_BYTES(Treasury)
ECS_COMPONENT(Treasury, "game::Treasury")

struct SavedEntity {
	EntityID eid;
	int64_t cents = 0;
};

// === Save: between ticks, after every CommandBuffer has been applied ===
bool save_session(World& world, WorldIdentitySnapshot& snap, std::vector<SavedEntity>& data) {
	if (!world.snapshot_identity(snap)) {
		return false; // mid-tick, un-applied creates, or corrupt free chain
	}
	// Data layer is yours: capture component values keyed by EntityID. Sort by slot
	// index so the load below recreates in canonical order.
	world.for_each_with_entity<Treasury>([&](EntityID e, Treasury& t) {
		data.push_back(SavedEntity { e, t.cents });
	});
	std::sort(data.begin(), data.end(), [](SavedEntity const& a, SavedEntity const& b) {
		return a.eid.index < b.eid.index;
	});
	return true;
}

// === Load: into a FRESH World ===
bool load_session(World& restored, WorldIdentitySnapshot const& snap, std::vector<SavedEntity> const& data) {
	if (!restored.restore_identity(snap)) {
		return false;
	}
	// Finalise every live slot, slot-index ascending (data was sorted at save time).
	for (SavedEntity const& entry : data) {
		if (!restored.restore_entity(entry.eid, Treasury { entry.cents })) {
			return false;
		}
	}
	// Singletons and systems are NOT part of the snapshot — re-establish them.
	restored.set_singleton<GameClock>();
	restored.register_system<MovementSystem>();
	return true;
}
```

After a correct restore, `digest(tick^k(restore(snapshot(s)))) == digest(tick^k(s))` at every worker count — that equality is enforced by tests/src/ecs/IdentitySnapshotInvariance.cpp. Full-state checksums (`world_checksum` in src/openvic-simulation/ecs/Checksum.hpp) are covered in [determinism.md](determinism.md).

---

## Source files

- src/openvic-simulation/ecs/World.hpp — `World`, `WorldIdentitySnapshot`, all signatures quoted above
- src/openvic-simulation/ecs/World.cpp — out-of-line definitions (guards, snapshot/restore, tick driving)
- src/openvic-simulation/ecs/System.hpp — `TickContext`, `SystemHandle`, `INVALID_SYSTEM_HANDLE`
- src/openvic-simulation/ecs/Query.hpp — `Query` builder ([queries.md](queries.md))
- src/openvic-simulation/ecs/ChunkView.hpp — `ChunkView<Cs...>` passed to `for_each_chunk`
- src/openvic-simulation/ecs/EntityID.hpp — `EntityID`, `ImmutableEntityID` ([entities.md](entities.md))
- tests/src/ecs/Integration.cpp, tests/src/ecs/Iteration.cpp, tests/src/ecs/Coverage.cpp — usage examples
- tests/src/ecs/IdentitySnapshot.cpp, tests/src/ecs/IdentitySnapshotInvariance.cpp — save/load semantics and the invariance gate
