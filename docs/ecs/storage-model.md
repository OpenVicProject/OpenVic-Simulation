# Storage model

This page explains how the `World` physically stores your components — archetypes, fixed-size chunks, and per-component columns — and the rules that storage imposes on game code. You almost never touch this machinery directly, but it is the reason behind three rules you *will* hit: pre-attach components at creation time, treat component pointers as short-lived, and use `ChunkView` only inside its callback. If you internalize the picture below, every performance rule in [pitfalls.md](pitfalls.md) follows from it.

## The mental model

- **One archetype per unique set of component types.** An archetype is identified by its sorted signature of `component_type_id_t`s. Every entity with exactly the components `{Position, Velocity}` lives in the same archetype; add a `Selected` tag and the entity lives in a *different* archetype, `{Position, Velocity, Selected}`. You never name archetypes in game code — they are created on demand by `create_entity` and by structural mutations, and queries match against them (see [queries.md](queries.md)).
- **An archetype stores rows in fixed 16 KB chunks.** `CHUNK_BLOCK_BYTES` is `16 * 1024`, aligned to `CHUNK_BLOCK_ALIGN` (64, cache-line). Each chunk holds up to `chunk_capacity` rows; the capacity is computed once at archetype creation and is constant for the archetype's life.
- **Within a chunk, components are stored in columns (slabs), not per-entity structs:**

  ```
  [entity_id slab: EntityID[chunk_capacity]]
  [component_0 slab: chunk_capacity * sizeof(C0), aligned]
  [component_1 slab: ...]
  ...
  ```

  All of one component type sits contiguously, which is what makes `for_each` / `for_each_chunk` iteration cache-friendly and SIMD-friendly.
- **Chunks are the unit of growth.** When the trailing chunk is full, a fresh 16 KB chunk is allocated; existing column data is **never relocated to grow**. That is the principal performance advantage over per-column `std::vector` storage (no reallocation-and-copy of a million-row column).
- **Tag (zero-size, `std::is_empty`) components have no slab.** Their presence is carried purely by the archetype signature. That is why `get_component<Tag>` and `ChunkView::array<Tag>()` return `nullptr` by design — use `has_component<Tag>(eid)` instead (see [components.md](components.md)).

Rows fill chunks left to right, and only the *trailing* chunk can ever be partial — removal always compacts against the global last row (next section).

## Chunk capacity and overflow

`chunk_capacity` is roughly `CHUNK_BLOCK_BYTES / (sizeof(EntityID) + sum of non-tag component sizes)`, minus alignment padding. A one-`int` component gives ~1365 rows per chunk (16 KB / 12 bytes per row); a 512-byte component gives ~31. There is no public accessor for the capacity — and you should not write code that depends on its exact value — but the overflow behavior is fully observable through chunk iteration:

```cpp
template<typename... Cs, typename Fn>
void for_each_chunk(Fn&& fn);

template<typename... Cs, typename Fn>
void for_each_chunk(Query const& query, Fn&& fn);
```

(`src/openvic-simulation/ecs/World.hpp`.) The callback receives one `ChunkView<Cs...>` per non-empty chunk of every matched archetype. Inserting `chunk_capacity + 1` entities observably produces two views — the first full, the second with one row (from `tests/src/ecs/ChunkOverflow.cpp`):

```cpp
struct Heavy {
	std::uint64_t pad[64] {};
	int marker = 0;
	std::int32_t _pad = 0;
};
ECS_COMPONENT(Heavy, "example::Heavy")

World world;
for (std::size_t i = 0; i < cap + 1; ++i) {
	world.create_entity(Heavy { .marker = static_cast<int>(i) });
}

std::vector<std::size_t> chunk_counts;
world.for_each_chunk<Heavy>([&](ChunkView<Heavy> view) {
	chunk_counts.push_back(view.count());
});
// chunk_counts == { cap, 1 }
```

Other observable overflow/compaction behavior:

- A chunk that becomes empty at the tail is dropped — destroy the lone entity in the second chunk and `for_each_chunk` visits only one chunk again (verified in `tests/src/ecs/ChunkOverflow.cpp`). The freed block goes back to the `ChunkPool` (see below).
- `for_each_chunk` never calls you with an empty view: `view.count() >= 1` inside the callback. This is a guarantee of `World::for_each_chunk` itself — zero-row chunks are skipped.
- A fully drained archetype holds zero chunks (`Archetype::drop_empty_trailing_chunk` keeps no spare), but the archetype itself persists for the `World`'s lifetime — it still participates in query matching, at zero per-chunk cost. The drain-and-refill cycle is exercised by the ping-pong tests in `tests/src/ecs/ChunkPool.cpp`.

## Removal compacts by swap-pop

`destroy_entity(id)`, and any migration that moves an entity *out* of an archetype, vacates a row. The storage refuses holes: the vacated row is filled by **moving the global last row** (last row of the last chunk — a cross-chunk move when needed) into the gap, then the last chunk's count drops. The relocated entity keeps its `EntityID` — its slot record is updated — but its components now live at a different (chunk, row).

Consequences you must design for:

- **Row order within an archetype is creation order only until the first removal.** After any destroy or migration, entities are reordered. Never encode meaning into row position.
- **Destroying entity A moves entity B's data.** This is the root cause of the pointer-invalidation rule below — a `get_component` pointer to an entity you did not touch can still be invalidated by an operation on its *neighbors*.

## Archetype migration and the pre-attach rule

```cpp
template<typename C>
C* add_component(EntityID id, C&& value);

template<typename C>
C* add_component(EntityID id);

template<typename C>
bool remove_component(EntityID id);
```

(`src/openvic-simulation/ecs/World.hpp`.) Because an archetype *is* its component set, adding or removing a component cannot happen in place — the entity must move to a different archetype. One `add_component` call does all of this:

1. Builds the target signature (`current ∪ {C}` or `current ∖ {C}`) and looks up — or **creates** — the target archetype. Creating a new archetype bumps the internal archetype epoch, invalidating every cached query result.
2. Reserves a row in the target (possibly allocating a fresh 16 KB chunk).
3. **Move-constructs every existing component of the entity**, column by column, from the source row into the target row.
4. Swap-pop compacts the source archetype — moving an *unrelated* entity's entire row.
5. Rewrites the entity's slot record.

That is roughly two full row moves per call, plus allocator and lookup traffic. Hence the most important rule in this codebase (from `ECS.md`):

> **Pre-attach output components at `create_entity` time.** `add_component` after the fact = one archetype migration per call. Lethal at scale.

If a system will write a result component for an entity, give the entity that component (default-constructed if need be) in its `create_entity` / `create_entities` call. Migration is for genuinely rare state transitions, not per-tick data flow. Also avoid ping-ponging a component on and off: every distinct signature you ever produce becomes a permanent archetype, and each *new* one invalidates the query caches.

Contracts (the first four are verified in `tests/src/ecs/Migration.cpp` and `tests/src/ecs/ChunkMigration.cpp`):

- `add_component` returns a pointer to the component in its new location, or `nullptr` if the entity is dead. If the entity **already has** `C`, the value is replaced in place — no migration — and the existing pointer is returned.
- `add_component<Tag>` returns `nullptr` (tags have no data slot) but does add the tag to the archetype; check with `has_component<Tag>`.
- `remove_component` returns `false` if the entity is dead, does not carry `C`, or `C` is its **only** component — a zero-component entity is not allowed; call `destroy_entity` instead.
- All component values, including non-trivial ones (e.g. `std::string` members), survive migration via move construction. The `EntityID` is unchanged by migration.
- **Not callable mid-tick.** Structural mutations (`create_entity`, `destroy_entity`, `add_component`, `remove_component`) are refused (no-op failure returns + an error log) while systems are ticking. From inside a system, queue them on `ctx.cmd` instead (see [command-buffer.md](command-buffer.md)). Verified in `tests/src/ecs/InTickMutationGuard.cpp`.
- **Immutable entities refuse migration.** Entities created via `create_immutable_entity` cannot have components added or removed — compile-time for `ImmutableEntityID`, runtime backstop for plain `EntityID` (see [entities.md](entities.md)). Component *data* stays mutable. Verified in `tests/src/ecs/ImmutableEntity.cpp`.
- Single-threaded by design: structural mutations happen only on the main tick thread, between ticks or in the command-buffer apply phase.

A realistic (rare, justified) migration, adapted from `tests/src/ecs/ChunkMigration.cpp`:

```cpp
EntityID const target = ids[cap + 1]; // an entity in the archetype's second chunk
world.add_component<CMigrB>(target, CMigrB { .marker = 999 });

// The entity moved to the {CMigrA, CMigrB} archetype; both values intact:
CHECK(world.get_component<CMigrA>(target)->marker == static_cast<int>(cap + 1));
CHECK(world.get_component<CMigrB>(target)->marker == 999);
```

## Component pointer invalidation

```cpp
template<typename C>
C* get_component(EntityID id);

template<typename C>
uint64_t component_version_in(EntityID id) const;
```

The rule (from `ECS.md`):

> **Component pointers are short-lived.** `world.get_component<C>(eid)` is valid only until the next `create_entity` / `destroy_entity` / `add_component` / `remove_component` on that archetype. Across ticks: use `CachedRef<C>` or re-look-up by id.

Why each operation invalidates:

- `destroy_entity` / migration *out*: swap-pop may move **your** entity's row to fill the gap, or move a neighbor over a slot you point at.
- `add_component` / `remove_component` on the entity itself: the whole row moves to another archetype.
- `create_entity` / migration *in*: existing chunk data is mechanically stable (chunks never relocate on growth), but the contract still counts it as invalidating — every push bumps the per-column version counters, and the version counter is the *only* supported validity signal. Code that "knows" pushes are safe is relying on an internals detail that the contract does not promise.

The supported revalidation primitive is `component_version_in<C>(eid)`: it returns the monotonically increasing version of `C`'s column in the entity's current archetype (0 if dead or no longer carrying `C`). A stable version implies cached pointers into that column are still valid. `CachedRef<C>` (see [entities.md](entities.md)) packages exactly this pattern for cross-tick references: id + generation + version, re-resolving only when the version moved.

`EntityID`s, by contrast, are never invalidated by storage operations — they are stable handles for the entity's whole life (and stable across save/load, see [world.md](world.md)).

## ChunkView — the user-facing window into a chunk

```cpp
template<typename... Cs>
struct ChunkView {
	std::size_t count() const;

	EntityID* OV_RESTRICT entities();
	EntityID const* OV_RESTRICT entities() const;

	template<typename C>
	C* OV_RESTRICT array();

	template<typename C>
	C const* OV_RESTRICT array() const;
};
```

(`src/openvic-simulation/ecs/ChunkView.hpp`.) A `ChunkView<Cs...>` wraps a single chunk's slabs: the `EntityID` array plus one typed component-array pointer per `Cs...`. All arrays share the same length, `count()`. You receive one in two places:

- `World::for_each_chunk<Cs...>(fn)` / `for_each_chunk<Cs...>(query, fn)` callbacks, and
- a `ChunkSystem<Derived, Cs...>`'s `tick_chunk(ChunkView<Cs...> view, TickContext const& ctx)` — the chunk-granular system base for tight inner loops (see [systems.md](systems.md)).

Rules:

- **The view is valid only inside the callback.** Do not store it, do not stash its pointers — the underlying chunk data may be relocated by any subsequent structural mutation of the `World`. Mutations *through* the view during the callback are fine and immediately visible to later iteration (verified in `tests/src/ecs/ChunkView.cpp`).
- `array<C>()` requires `C` to be exactly one of the view's `Cs...`; anything else is a compile error (`static_assert`).
- `array<Tag>()` returns `nullptr` for tag types — never dereference it; tags contribute presence (the archetype matched), not data.
- The accessors return `OV_RESTRICT`-qualified pointers — the compiler is told the entity array and each component array do not alias one another. For the strongest codegen, bind each slab into an `OV_RESTRICT` local at the top of your loop; restrict on locals is honored more reliably than on function returns.

A complete `ChunkSystem`, adapted from `tests/src/ecs/ChunkSystem.cpp`:

```cpp
struct CSPosition {
	int64_t x = 0;
	int64_t y = 0;
};
struct CSVelocity {
	int64_t dx = 0;
	int64_t dy = 0;
};
ECS_COMPONENT(CSPosition, "example::CSPosition")
ECS_COMPONENT(CSVelocity, "example::CSVelocity")

// `CSVelocity const` in the base's template list declares Read access;
// plain `CSPosition` declares Write. The view's type uses the plain types.
struct MoverChunkSystem : ChunkSystem<MoverChunkSystem, CSPosition, CSVelocity const> {
	void tick_chunk(ChunkView<CSPosition, CSVelocity> view, TickContext const&) {
		CSPosition* OV_RESTRICT pos = view.array<CSPosition>();
		CSVelocity const* OV_RESTRICT vel = view.array<CSVelocity>();
		for (std::size_t i = 0; i < view.count(); ++i) {
			pos[i].x += vel[i].dx;
			pos[i].y += vel[i].dy;
		}
	}
};
ECS_SYSTEM(MoverChunkSystem)
```

The same view shape works for ad-hoc iteration outside systems:

```cpp
world.for_each_chunk<CVA>([&](ChunkView<CVA> view) {
	CVA* arr = view.array<CVA>();
	for (std::size_t i = 0; i < view.count(); ++i) {
		arr[i].v *= 10;
	}
});
```

## Iteration order, locality, and determinism

What the storage guarantees about iteration order (`for_each`, `for_each_with_entity`, `for_each_chunk` — all drive the same walk):

- Matched archetypes are visited in archetype-creation order, chunks left to right, rows `0..count-1`. Given an identical history of operations, this order is bit-identical across runs and machines — chunk iteration is deterministic *within* a run lineage.
- **Row order is creation order until the first removal in that archetype** — swap-pop compaction then permutes it. Bulk creation (`create_entities`, see [entities.md](entities.md)) packs its batch into contiguous row ranges, identical to the equivalent `create_entity` loop.
- **Packing is not saved state.** After a save/load round-trip the identity layer (ids, generations, free-list) is reproduced exactly, but row/chunk packing may legitimately differ from the never-saved run. Therefore: anything *id-assignment-sensitive* (e.g. loops issuing `cmd.create_entity` calls where the resulting id assignment matters) must iterate in id / dense-index order, never chunk order. Per-row independent reads/writes are unaffected — for them chunk order genuinely doesn't matter. See [determinism.md](determinism.md) and [world.md](world.md).
- **Key locality is a heuristic you create, not an invariant the storage maintains.** Because consecutive creations occupy consecutive rows, creating entities grouped by some key at setup time yields chunks whose rows are grouped by that key. `reductions::parallel_keyed_sum` (namespace `OpenVic::ecs::reductions`, `src/openvic-simulation/ecs/Reductions.hpp`) is built to exploit exactly this grouping, but swap-pop removals erode it over time. Code must stay *correct* for arbitrary row order and merely *faster* when locality holds. See [threading-and-reductions.md](threading-and-reductions.md).

## Chunk memory reuse (`ChunkPool`)

Each `World` owns a `ChunkPool` of 16 KB blocks (`src/openvic-simulation/ecs/ChunkPool.hpp`). You never need to call it from game code — it exists so the storage rules above stay cheap:

- Dropped chunks go back to the pool and are handed out LIFO, so an archetype of transient per-tick entities that drains and refills every tick reuses warm memory with **zero** steady-state allocations — verified in `tests/src/ecs/ChunkPool.cpp` ("Ping-pong create/destroy reuses pooled chunks"). Blocks are interchangeable across archetypes.
- The cache is capped at `MAX_POOL_SIZE` (64) blocks, and blocks idle for more than `AGE_THRESHOLD_TICKS` (256) ticks are returned to the OS (`tick_systems` advances the aging clock). Aging affects memory residency only — never simulation results.
- `World::chunk_pool()` exposes the pool with diagnostic accessors (`pooled_count()`, `total_allocations()`, `total_deallocations()`, `current_tick()`) — useful in tests asserting allocation behavior; production code has no reason to call them.
- Single-threaded by design: structural mutations are serialized on the main tick thread, so the pool needs no synchronization.

## Source files

- `src/openvic-simulation/ecs/Archetype.hpp` — archetype, columns, row reservation, swap-pop
- `src/openvic-simulation/ecs/Chunk.hpp` — `DataChunk`, `CHUNK_BLOCK_BYTES`, `CHUNK_BLOCK_ALIGN`, `OV_RESTRICT`, block layout
- `src/openvic-simulation/ecs/ChunkPool.hpp` — block pool, cap, aging
- `src/openvic-simulation/ecs/ChunkView.hpp` — the user-facing chunk window
- `src/openvic-simulation/ecs/ChunkSystem.hpp` — chunk-granular system base (`tick_chunk`)
- `src/openvic-simulation/ecs/World.hpp` — `create_entity`, `add_component`, `remove_component`, `component_version_in`, `for_each_chunk`
- Tests: `tests/src/ecs/Archetype.cpp`, `tests/src/ecs/Chunk.cpp`, `tests/src/ecs/ChunkPool.cpp`, `tests/src/ecs/ChunkView.cpp`, `tests/src/ecs/ChunkMigration.cpp`, `tests/src/ecs/ChunkOverflow.cpp`, `tests/src/ecs/Migration.cpp`
