# Entities

An entity is a stable identity — an `EntityID` — that ties together a set of components stored in an archetype (see [storage-model.md](storage-model.md)). This page covers everything about creating, destroying, and referring to entities from game code: handle encoding and validity, single and bulk creation, immutable entities, save/load id stability, and `CachedRef` for references that must survive across ticks. Defining the components themselves is covered in [components.md](components.md); the rest of the `World` API in [world.md](world.md).

## EntityID

```cpp
struct EntityID {
	uint32_t index = 0;
	uint32_t generation = 0;

	constexpr bool operator==(EntityID const& rhs) const;
	constexpr bool operator!=(EntityID const& rhs) const;
	constexpr bool is_valid() const;
	constexpr bool is_deferred() const;
	constexpr uint64_t to_uint64() const;
	static constexpr EntityID from_uint64(uint64_t value);
};

inline constexpr EntityID INVALID_ENTITY_ID = {};
```

Defined in [src/openvic-simulation/ecs/EntityID.hpp](../../src/openvic-simulation/ecs/EntityID.hpp). There is no parallel id system — `EntityID` is the one handle type for cross-references between entities. Store it by value; it is two `uint32_t`s and trivially copyable.

### Encoding

- `index` — the entity's slot in the World's slot table. Slots are reused after destruction.
- `generation` — bumped every time a slot is reused, so a stale handle to a recycled slot reliably fails validity checks. Real generations live in `[1, 0x7FFFFFFF]`.
- `generation == 0` is the invalid sentinel. `INVALID_ENTITY_ID` is the default-constructed `{ 0, 0 }`.
- The high generation bit (`DEFERRED_GENERATION_BIT = 0x80000000u`) is reserved for deferred-create placeholders (below).

`to_uint64()` packs the handle as `(generation << 32) | index`; `from_uint64` round-trips it exactly. Use this pair when an id needs to cross a serialization boundary or be used as a map key:

```cpp
uint64_t const encoded = eid.to_uint64();
EntityID const decoded = EntityID::from_uint64(encoded); // decoded == eid
```

### Validity: `is_valid` vs `is_alive` vs `is_deferred`

These answer three different questions — do not conflate them:

| Check | Question | Where |
|---|---|---|
| `eid.is_valid()` | "Is this not the zero sentinel?" (`generation != 0`) | `EntityID` |
| `world.is_alive(eid)` | "Does this id name a live, finalised entity in this World right now?" | `World` |
| `eid.is_deferred()` | "Is this an unresolved placeholder from a parallel `CommandBuffer::create_entity`?" | `EntityID` |

`is_valid()` is a local property of the handle; only `is_alive` consults the World. In particular:

- A deferred placeholder satisfies `is_valid()` (its generation is non-zero) but is **not** alive — `is_deferred()` distinguishes it. Every public `World` accessor treats deferred ids as "not present" (`is_alive` → false, `get_component` → nullptr, `component_version_in` → 0), so a stray placeholder is safe to pass anywhere. Placeholders are only meaningful as arguments to other ops on the same `CommandBuffer` recording session — see [command-buffer.md](command-buffer.md).
- An id reserved by a serial `CommandBuffer::create_entity` is real (`is_valid()`, not deferred) but `is_alive` stays false until the buffer's `apply` finalises the slot.
- A stale id (slot since destroyed or reused) fails `is_alive` forever — the generation bump guarantees it, even after the slot is reoccupied:

```cpp
EntityID const a = world.create_entity(LifeA { 1 });
world.destroy_entity(a);

EntityID const b = world.create_entity(LifeA { 2 });
// b.index == a.index (LIFO free-list reuse), b.generation > a.generation
// world.is_alive(a) == false, world.is_alive(b) == true — forever.
```

### Determinism

EntityID assignment is a pure function of the create/destroy sequence: freed slots are reused LIFO, fresh slots grow in order, generations bump at allocate time. The same operation sequence on every machine yields the same ids — which makes **creation order simulation state**. Never drive entity creation from a non-deterministic iteration order (e.g. an `unordered_map` walk, or chunk order after a load — see the save/load section). See [determinism.md](determinism.md).

## Creating entities

```cpp
template<typename... Cs>
EntityID create_entity(Cs&&... values);
```

Constructs one entity whose archetype is exactly `Cs...` and returns its id. Each component is aggregate-initialised from the supplied value; component order in the call does not matter (the signature is sorted internally).

```cpp
EntityID const unit = world.create_entity(UnitHealth { 100 }, UnitMorale { 50 }, UnitTag {});
```

Contracts:

- **At least one component is required** (`static_assert`). Zero-component entities do not exist in this model.
- **Not callable mid-tick.** Inside a system tick the call is refused with an error log (loud-but-no-crash) and returns `EntityID {}` (invalid). Structural mutations from systems go through `ctx.cmd` — see [command-buffer.md](command-buffer.md) and [systems.md](systems.md).
- **Not thread-safe.** Direct `World` structural ops are for serial code between ticks (session setup, loaders, tests).

### Rule: pre-attach every component at creation time

Give the entity its **complete** component set up front, including output components a later system will fill in. Calling `add_component` afterwards migrates the entity to a different archetype: every existing component is move-constructed into the new archetype's columns, the old row is swap-popped, and every column version in both archetypes bumps. One migration is cheap; one migration *per entity per tick* is lethal at scale (think a million order entities per tick). The same applies to `remove_component` — prefer a tag/state field over structural churn. See [pitfalls.md](pitfalls.md) and [components.md](components.md).

## Immutable entities

```cpp
template<typename... Cs>
ImmutableEntityID create_immutable_entity(Cs&&... values);
```

Identical to `create_entity`, except the entity can **never change archetype after creation** and the returned handle is a distinct strong type:

```cpp
struct ImmutableEntityID {
	uint32_t index = 0;
	uint32_t generation = 0;

	constexpr bool operator==(ImmutableEntityID const& rhs) const;
	constexpr bool operator!=(ImmutableEntityID const& rhs) const;
	constexpr bool is_valid() const;
	constexpr bool is_deferred() const;
	constexpr uint64_t to_uint64() const;
	static constexpr ImmutableEntityID from_uint64(uint64_t value);
	constexpr EntityID unsafe_mutable_id() const;
};

inline constexpr ImmutableEntityID INVALID_IMMUTABLE_ENTITY_ID = {};
```

The guarantee is enforced twice:

1. **Compile time:** `ImmutableEntityID` has the same layout as `EntityID` but no implicit conversion to it. All structural-mutation APIs (`World::add_component`, `World::remove_component`, and the `CommandBuffer` equivalents) take `EntityID` by value, so passing an `ImmutableEntityID` simply does not compile. `tests/src/ecs/ImmutableEntity.cpp` pins this with a `static_assert` wall.
2. **Runtime backstop:** the entity's slot is flagged, so even if a plain `EntityID` for the same entity surfaces (from `for_each_with_entity`, or a laundered `unsafe_mutable_id()`), `add_component` / `remove_component` refuse with an error log and no migration — including through a `CommandBuffer` at apply time.

What "immutable" does **not** mean:

- **Component data stays mutable.** `get_component` returns a mutable `C*` through the immutable handle. Only the archetype is frozen.
- **Destruction is allowed.** Destroying a live entity is not an archetype mutation *of it*.
- **Address pinning is not promised.** Sibling churn in the same archetype can swap-pop the entity's row to a new address. Assert liveness and re-fetch pointers; never cache them across structural changes (see `CachedRef` below).

Read/destroy access goes through explicit overloads on `World`:

```cpp
template<typename C> C* get_component(ImmutableEntityID id);
template<typename C> C const* get_component(ImmutableEntityID id) const;
template<typename C> bool has_component(ImmutableEntityID id) const;
bool is_alive(ImmutableEntityID id) const;
void destroy_entity(ImmutableEntityID id);
bool is_immutable(ImmutableEntityID id) const;
template<typename C> uint64_t component_version_in(ImmutableEntityID id) const;
```

There is deliberately **no** `add_component` / `remove_component` overload for `ImmutableEntityID`.

### `unsafe_mutable_id` — the only escape hatch

`unsafe_mutable_id()` returns the underlying `EntityID`. It is intentionally verbose so every structural bypass is auditable with a single grep for `unsafe_mutable_id`. Do not use it casually; the runtime backstop will refuse structural ops anyway and log an error.

### `is_immutable` — branching on a plain EntityID

```cpp
bool is_immutable(EntityID id) const;
```

Returns true if and only if `id` refers to a *live* entity created immutable; false for dead, stale, or mutable entities. Use it when a system holds a plain `EntityID` (e.g. from `for_each_with_entity`, where the compile-time guarantee no longer applies) and wants to branch before queueing a structural op:

```cpp
if (!ctx.world.is_immutable(eid)) {
	ctx.cmd.add_component<SomeComponent>(eid, SomeComponent { /* ... */ });
}
```

System ticks can also receive the row's handle *as* `ImmutableEntityID` instead of `EntityID` — then a `ctx.cmd.add_component(eid, ...)` in the body would not compile at all. See [systems.md](systems.md).

A destroyed immutable entity's slot returns to the free list mutable: the next `create_entity` reusing that index produces a normal, mutable entity.

## Bulk creation

```cpp
template<typename... Cs, typename... Spans>
bool create_entities(std::size_t count, std::span<EntityID> out_ids, Spans&&... spans);

template<typename... Cs, typename... Spans>
bool create_immutable_entities(
	std::size_t count, std::span<ImmutableEntityID> out_ids, Spans&&... spans
);
```

Creates `count` entities sharing one signature `Cs...`, paying the per-entity costs of the equivalent `create_entity` loop **once per batch**: one signature sort, one archetype lookup, chunk-granular row reservation (the partial tail chunk fills first, then whole chunks), column-contiguous construction, and one column-version bump per touched chunk. Use it for session-setup-scale populations (hundreds of thousands of entities); use plain `create_entity` for onesies.

The new ids are written to `out_ids` in creation order. `create_immutable_entities` additionally stamps every slot immutable and hands back `ImmutableEntityID`s — same guarantees as the single-entity API.

### Input span contract

- Per-entity component values arrive as **one span per non-empty component**, matched positionally to the non-empty `Cs...` in pack order. Tag (zero-size) components are named in the pack but take **no** span.
- Each span's length must equal `count`, and `out_ids.size()` must equal `count`.
- Pass **no spans at all** to default-construct every component (tag-heavy / zeroed batches).
- **Input spans are MOVED-FROM**: values are move-constructed out of the caller's storage, which is left holding moved-from husks. Pass a copy if the source must survive. Spans of `const` elements are rejected at compile time so the contract cannot silently degrade to copies.
- `Cs...` must be plain types — no `const` / `volatile` / reference qualifiers (`static_assert`).

### Return value and failure modes

Returns `true` on success. Returns `false` when:

- called mid-tick (refused with an error log) — `out_ids` is then filled with `INVALID_ENTITY_ID`, matching the per-call `EntityID {}` the equivalent loop would have returned (inside a system, use `ctx.cmd.create_entities` instead — see [command-buffer.md](command-buffer.md));
- `out_ids` or any input span length mismatches `count` (error log) — `out_ids` is left untouched.

`count == 0` is a no-op returning `true`; like the empty loop, it does **not** create the archetype.

### Determinism guarantee

A bulk create yields the **identical end state — including identical EntityID assignment — as the equivalent `create_entity` loop**. Entity slots are allocated one-by-one in creation order (same free-list pops) and rows pack exactly as N single creates would. This equivalence is load-bearing: adopting bulk creation is never a simulation-behavior change, and `tests/src/ecs/BulkCreate.cpp` gates it (including worker-count invariance for the `CommandBuffer` path). The only divergence from the loop is the numeric values of column versions (bumped once per touched chunk instead of once per row) — versions only signal "changed", nothing may compare their values.

### Example: bulk session setup

Adapted from `tests/src/ecs/BulkCreate.cpp`:

```cpp
std::size_t const count = 100;

std::vector<BCA> as;
std::vector<BCB> bs;
as.reserve(count);
bs.reserve(count);
for (std::size_t i = 0; i < count; ++i) {
	as.push_back(BCA { static_cast<int>(i) });
	bs.push_back(BCB { static_cast<int>(i * 2) });
}

// One span per non-empty component, in pack order; BCTag takes no span.
// `as` and `bs` are moved-from after this call.
std::vector<EntityID> ids(count);
if (!world.create_entities<BCA, BCB, BCTag>(count, ids, as, bs)) {
	// length mismatch or mid-tick call — already logged
	return;
}

// ids[i] is the i-th created entity, bit-identical to what a create_entity loop
// would have assigned.
for (EntityID const eid : ids) {
	// alive, carries BCA + BCB + BCTag
}
```

## Destroying entities

```cpp
void destroy_entity(EntityID id);
bool is_alive(EntityID id) const;
```

`destroy_entity` destroys each component in place, swap-pops the row out of its archetype, and pushes the slot onto the free list (LIFO — the most recently freed slot is reused first, with a bumped generation).

Contracts and behavior:

- **Safe on anything.** Dead, stale, invalid, deferred, and out-of-range ids are silent no-ops; double-destroy is harmless. You do not need to pre-check `is_alive`.
- **Not callable mid-tick** (refused no-op + error log). Use `ctx.cmd.destroy_entity(eid)` from systems; the destroy applies after the stage — see [command-buffer.md](command-buffer.md).
- **Swap-pop relocates a neighbor.** Destroying a row moves the archetype's *last* row into the hole. That other entity stays alive and its id stays valid, but any raw component pointer you held to it is now wrong. This is one of the reasons component pointers are short-lived (see `CachedRef` below).
- **Don't destroy while iterating.** Collect ids during `for_each_with_entity`, destroy after the loop (or via a command buffer) — destroying mid-iteration would invalidate the columns being walked. See [queries.md](queries.md).
- On a reserved-but-unfinalised id (a serial `CommandBuffer` create that has not been applied yet), `destroy_entity` just drops the reservation.

### Example: full lifecycle

Adapted from `tests/src/ecs/EntityLifecycle.cpp`:

```cpp
World world;

// Pre-attach the full component set — never add_component afterwards.
EntityID const a = world.create_entity(LifeA { 100 });
EntityID const b = world.create_entity(LifeA { 200 });
EntityID const c = world.create_entity(LifeA { 300 });

// Destroying `a` swap-pops `c` (the last row) into a's slot. `c` stays valid;
// only raw pointers into the column are invalidated.
world.destroy_entity(a);

LifeA* lc = world.get_component<LifeA>(c); // re-fetch — fresh pointer, value intact
// lc->v == 300

// Stale handle: every accessor degrades safely, forever.
// world.is_alive(a) == false, world.get_component<LifeA>(a) == nullptr
world.destroy_entity(a); // no-op

// The freed slot is reused with a bumped generation.
EntityID const d = world.create_entity(LifeA { 400 });
// d.index == a.index, d.generation > a.generation, world.is_alive(a) still false
```

## EntityID stability across save/load

EntityIDs are save-stable: the identity layer (slot generations, immutability flags, free-list order) can be snapshotted and rebuilt so that every saved id is alive at its original `(index, generation)` after load, stale ids stay dead, and **subsequent allocations continue exactly as in the never-saved run**. Saved `EntityID`s in component data therefore remain valid cross-references after a load.

```cpp
bool snapshot_identity(WorldIdentitySnapshot& out) const;
bool restore_identity(WorldIdentitySnapshot const& snapshot);

template<typename... Cs>
bool restore_entity(EntityID eid, Cs&&... values);
```

`WorldIdentitySnapshot` is a plain serializable struct (`slots` + `free_list`); the ECS imposes no IO format — serialize it however the save system likes. It deliberately captures identity **only**: archetypes, chunks, rows, packing, singletons, and systems are rebuilt by the loader. See [world.md](world.md) for the broader snapshot story.

The flow:

1. **Save:** call `snapshot_identity` between ticks. It refuses (error log + `false`) mid-tick, while any reserved-but-unfinalised slot exists (i.e. a `CommandBuffer` holds un-applied creates — apply every buffer first), or if free-chain corruption is detected. Serialize the snapshot plus, per live entity, its component data.
2. **Load:** on a **fresh** `World` (no entity slot ever allocated, outside any tick), call `restore_identity(snapshot)`. The snapshot is validated fully before any mutation; on failure the World is untouched. Afterwards every live slot is addressable at its original `(index, generation)` but `is_alive` is still false — each must be finalised.
3. **Recreate:** call `restore_entity(eid, values...)` once per live slot, **in slot-index ascending order**. Same component rules as `create_entity` (at least one component, values aggregate-initialised). It validates that `eid` names a reserved-but-unfinalised slot with a matching generation and returns `false` + error log on any mismatch (deferred id, out of range, dead slot, stale generation, already finalised). Not callable mid-tick.

Loader rules (each one protects determinism):

- **Between `restore_identity` and the last `restore_entity`, the only legal entity ops are `restore_entity` calls.** In particular `destroy_entity` on a not-yet-finalised id drops the reservation and PUSHES the slot onto the free list, silently corrupting the restored allocation order.
- **Recreate in slot-index ascending order.** Identity correctness is order-independent, but archetype packing is not — the canonical order is what makes packing reproducible across loads.
- **Same ids require the same allocation-request sequence post-load.** The identity layer guarantees "same request sequence → same ids"; producing that sequence is the caller's job. Packing may legitimately differ from the saved run, so anything id-assignment-sensitive must iterate in id / dense-index order, **never chunk order**.
- **Immutability is restored as data**, not by how `restore_entity` is called — there is deliberately no `ImmutableEntityID` overload. Rebuild strong handles as `ImmutableEntityID { index, generation }` after finalisation; the restored flag still gates structural ops at runtime.

Adapted from `tests/src/ecs/IdentitySnapshot.cpp`:

```cpp
// --- save (between ticks, all command buffers applied) ---
WorldIdentitySnapshot snap;
if (!world.snapshot_identity(snap)) {
	return; // refused — mid-tick, un-applied creates, or corruption (already logged)
}
// ... serialize snap + per-entity component data ...

// --- load (fresh World) ---
World restored;
if (!restored.restore_identity(snap)) {
	return;
}
// One restore_entity per saved live entity, slot-index ascending:
for (SavedEntity const& saved : saved_entities_in_slot_index_order) {
	restored.restore_entity(
		EntityID { saved.index, saved.generation }, IdsA { saved.a }, IdsB { saved.b }
	);
}
// Saved handles are now live at their original (index, generation); destroyed
// handles stay dead; the next create_entity continues the never-saved sequence.
```

## Cross-tick references: `CachedRef`

`world.get_component<C>(eid)` returns a raw pointer that is valid **only until the next structural change touching that archetype** — any `create_entity`, `destroy_entity`, `add_component`, or `remove_component` that pushes, swap-pops, or relocates rows in it. Across ticks, raw component pointers are never safe. Store the `EntityID` and re-look-up — or use `CachedRef<C>`, which automates the re-look-up and makes the common "nothing changed" case nearly free.

```cpp
template<typename C>
struct CachedRef {
	EntityID entity_id = INVALID_ENTITY_ID;
	uint64_t cached_version = 0;
	C* cached_pointer = nullptr;

	static CachedRef from(World& world, EntityID id);
	EntityID entity() const;
	bool is_valid(World const& world) const;
	void invalidate();
	C* get(World& world);
};
```

Defined in [src/openvic-simulation/ecs/CachedRef.hpp](../../src/openvic-simulation/ecs/CachedRef.hpp).

- `CachedRef<C>::from(world, id)` builds a ref and resolves it immediately.
- `get(world)` returns the current component pointer, refreshing the cache if the column has mutated since the last successful resolve or the entity changed archetype. Returns `nullptr` if the entity is dead or no longer carries `C`. The fast path is one version comparison and an indirection — cheaper than calling `World::get_component<C>` every time.
- `is_valid(world)` — true when the last resolve succeeded *and* the entity is still alive. `get` is the authoritative call; `is_valid` is a cheap pre-check.
- `invalidate()` clears the cache; the next `get` re-resolves from scratch.

Lifetime and threading: a `CachedRef` may be stored across ticks and is safe to copy (copies cache independently). Holding one after the entity is destroyed is fine — `get` just returns `nullptr`. `get` mutates the ref's cache, so a single `CachedRef` instance must not be shared between concurrently running threads; give each owner its own.

The staleness detection is built on column versions:

```cpp
template<typename C>
uint64_t component_version_in(EntityID id) const;
```

Returns the component-column version for `C` in the entity's current archetype, or `0` if the entity is dead or no longer carries `C`. The version monotonically increases on every structural change to that column (push, swap-pop, relocate) — a stable version implies cached pointers into the column are still valid. In-place field writes do **not** bump it. Version *values* carry no meaning beyond "changed"; never compare them across runs (bulk creation legitimately produces different values than the equivalent loop).

### Example: a reference that survives structural churn

Adapted from `tests/src/ecs/CachedRef.cpp`:

```cpp
World world;
EntityID const a = world.create_entity(VA { 1 });
EntityID const b = world.create_entity(VA { 2 });

CachedRef<VA> ref_b = CachedRef<VA>::from(world, b);

// Destroying `a` swap-pops `b` into a's row — any raw VA* to `b` is now wrong.
world.destroy_entity(a);

VA* fresh_b = ref_b.get(world); // version mismatch detected, re-resolves
// fresh_b != nullptr, fresh_b->v == 2

// Even an archetype migration is survived: the ref follows the entity.
world.add_component<VB>(b, VB { 0 });
fresh_b = ref_b.get(world); // re-resolves into the new archetype's column
// fresh_b->v == 2

world.destroy_entity(b);
// ref_b.get(world) == nullptr, ref_b.is_valid(world) == false — dead is dead.
```

Note for tag (zero-size) components: `get_component<Tag>` returns `nullptr` by design, so a `CachedRef<Tag>` can never resolve — use `has_component<Tag>(eid)` instead (see [components.md](components.md)).

## Quick rules recap

- Pre-attach **every** component at `create_entity` time; post-hoc `add_component` is an archetype migration per call.
- Never call `create_entity` / `create_entities` / `destroy_entity` (or `add_component` / `remove_component`) from inside a system tick — they no-op with an error log. Use `ctx.cmd` ([command-buffer.md](command-buffer.md)).
- Component pointers are short-lived; across structural changes or ticks, hold the `EntityID` and re-resolve, or use `CachedRef<C>`.
- `is_valid()` is not `is_alive()`. Liveness is a World question.
- Entity creation order is deterministic simulation state; bulk creation is guaranteed id-identical to the loop.
- Immutable entities freeze the archetype, not the data. `unsafe_mutable_id()` is the single grep-able bypass, and the runtime backstop refuses structural ops regardless.
- Save/load: snapshot between ticks with all buffers applied; restore on a fresh World; `restore_entity` in slot-index ascending order and nothing else in between.

## Source files

- [src/openvic-simulation/ecs/EntityID.hpp](../../src/openvic-simulation/ecs/EntityID.hpp) — `EntityID`, `ImmutableEntityID`, sentinels, `DEFERRED_GENERATION_BIT`
- [src/openvic-simulation/ecs/World.hpp](../../src/openvic-simulation/ecs/World.hpp) — creation/destruction/bulk APIs, `WorldIdentitySnapshot`, `restore_entity`, `component_version_in`
- [src/openvic-simulation/ecs/World.cpp](../../src/openvic-simulation/ecs/World.cpp) — slot allocation, `is_alive` / `destroy_entity` / identity snapshot bodies
- [src/openvic-simulation/ecs/CachedRef.hpp](../../src/openvic-simulation/ecs/CachedRef.hpp) — `CachedRef<C>`
- Tests with observable semantics: `tests/src/ecs/EntityID.cpp`, `tests/src/ecs/EntityLifecycle.cpp`, `tests/src/ecs/BulkCreate.cpp`, `tests/src/ecs/ImmutableEntity.cpp`, `tests/src/ecs/CachedRef.cpp`, `tests/src/ecs/IdentitySnapshot.cpp`
