# Components

Components are the plain structs that hold all per-entity simulation data. You define a struct, register it once with `ECS_COMPONENT`, attach it at `create_entity` time, and read/write it through `World` accessors or system iteration. This page covers the rules a component type must satisfy, tag (zero-size) components, the add/get/has/remove API and its pointer-lifetime rules, singleton components, and `DenseSlotAllocator` for the side tables that singletons own.

For how components are physically stored (archetypes, chunks, columns) see [storage-model.md](storage-model.md). For iterating components in bulk see [queries.md](queries.md) and [systems.md](systems.md).

## Defining a component

A component is an ordinary struct plus one registration line:

```cpp
struct Health {
	int hp = 0;
	int max_hp = 0;
};

ECS_COMPONENT(Health, "OpenVic::Health")
```

`ECS_COMPONENT(Type, NameLiteral)` must be expanded at namespace scope, **outside any other namespace** (it opens `namespace OpenVic::ecs` itself). It specialises the `ComponentName<C>` trait with a stable string literal that becomes the type's identity across all builds (see [ComponentTypeID](#componenttypeid--how-types-get-ids) below). The literal must be globally unique within the simulation, and renaming it is a breaking change to anything that persists component IDs — saves, replays, the network protocol.

Forgetting to register is a clear compile error, not a silent misbehaviour: the primary `ComponentName` template is intentionally undefined, so the first use of an unregistered type with a `World` fails with "incomplete type `ComponentName<X>`".

### Requirements on component types

What the type system actually enforces:

- **Move-constructible and destructible.** Storage moves components between rows during archetype migrations and swap-pop compaction, and destroys them on `destroy_entity` / `remove_component`. Non-trivial members (e.g. a `std::string`) are handled correctly — destructors run, nothing leaks (see `tests/src/ecs/Component.cpp`, "Components with non-trivial dtor are destroyed properly").
- **Checksummable.** The first use of a component type with a `World` instantiates its column vtable, which `static_assert`s the universal hashing rule from `src/openvic-simulation/ecs/ChecksumTraits.hpp` (`is_checksummable_v<C>`). Every component (and singleton) type must hash one of two ways:
  1. **Raw bytes** — allowed only if `std::has_unique_object_representations_v<C>`, i.e. the compiler inserted no padding and there are no `float`/`double` members. Padding bytes are indeterminate garbage; fill gaps with explicit `_pad` members, zeroed at construction.
  2. **A custom hash** — a free function `uint64_t ecs_checksum(C const&, uint64_t seed)` declared at `C`'s scope (found by ADL), before `C`'s first `World`/`CommandBuffer` use in the translation unit. Mandatory for anything holding heap data (`std::string`, vector members, ...): walk sizes + elements in index order, never capacities or addresses.

  Tag (empty) types are exempt — their contribution is presence only. For trivially copyable types that the unique-representation check rejects conservatively (float members), `ECS_CHECKSUM_BYTES(Type)` expands an author-asserted byte hash; by writing it you assert no raw-pointer members and no implicit padding. A type satisfying neither path is a compile error, never a silent skip. See [determinism.md](determinism.md) for why the checksum exists at all.
- **No raw pointers.** This is the one rule the compiler cannot check: raw pointers *are* uniquely representable, so a byte-hashed pointer member would hash addresses — nondeterministic across runs, silently breaking every determinism gate. Cross-references between entities are dense indices or `EntityID` (see [entities.md](entities.md)).
- **Plain types in bulk creation.** `create_entities` additionally `static_assert`s that each component type is a plain type — "no const/volatile/reference".

Beyond the hard checks, the project convention is flat POD-style components: simulation math uses `fixed_point_t`, never floats (see [determinism.md](determinism.md)), and OOP classes migrate to flat data plus sibling components rather than components with behaviour.

A realistic definition block, adapted from `tests/src/ecs/Component.cpp`:

```cpp
namespace {
	struct Health {
		int hp = 0;
		int max_hp = 0;
	};
	struct Name {
		std::string value;
	};
	// Heap-holding type — custom checksum is mandatory: walk size then bytes in index order.
	inline uint64_t ecs_checksum(Name const& name, uint64_t seed) {
		uint64_t h = fold_uint64(name.value.size(), seed);
		return fnv1a_64_bytes(name.value.data(), name.value.size(), h);
	}
	struct Velocity {
		float dx = 0;
		float dy = 0;
	};
	// Two floats, no padding — author-asserted byte hash.
	ECS_CHECKSUM_BYTES(Velocity)
}

ECS_COMPONENT(Health, "test_Component::Health")
ECS_COMPONENT(Name, "test_Component::Name")
ECS_COMPONENT(Velocity, "test_Component::Velocity")
```

## ComponentTypeID — how types get IDs

You will mostly never touch this machinery directly; it is what `ECS_COMPONENT` plugs into. From `src/openvic-simulation/ecs/ComponentTypeID.hpp`:

```cpp
using component_type_id_t = uint64_t;

constexpr component_type_id_t fnv1a_64(std::string_view s);

template<typename C>
struct ComponentName; // primary template intentionally undefined

template<typename C>
constexpr component_type_id_t component_type_id_of();
```

`component_type_id_of<C>()` is the FNV-1a 64-bit hash of `ComponentName<C>::value` — the string you passed to `ECS_COMPONENT`. It is pure `constexpr`: the same string yields the same id on every compiler and platform, so component IDs are byte-identical across builds. This is the foundation for cross-platform deterministic protocols (multiplayer handshakes, save sharing, replay logs), and it is why the *string*, not the C++ type, is the durable identity.

The one place game code spells these ids out by hand is system access declarations — `extra_reads()` / `extra_writes()` return arrays of `component_type_id_of<C>()` values (see [systems.md](systems.md) and [Singleton components](#singleton-components) below). `component_type_id_of` is usable in `static_assert` (verified in `tests/src/ecs/FNVHash.cpp`).

## Attaching components: prefer `create_entity` time

Entity creation is covered in [entities.md](entities.md); the rule that belongs here:

> **Pre-attach output components at `create_entity` time.** `add_component` after the fact = one archetype migration per call. Lethal at scale (e.g. 1M order entities/tick).

Why: an entity's component set determines its archetype. Adding or removing a component moves the entity to a *different* archetype — every existing component is move-constructed into the new archetype's columns, the old row is destroyed, and the source archetype is compacted by swap-popping another entity into the hole. That is real per-entity work, and it invalidates pointers (see below). Attaching everything up front in one `create_entity(...)` call constructs each component exactly once, directly into its final column.

If an entity needs a component only "sometimes", it is usually still cheaper to attach it always (possibly defaulted, or as a [tag](#tag-zero-size-components)) than to migrate entities in and out of archetypes every tick.

## Reading and writing: `get_component` / `has_component`

```cpp
template<typename C>
C* get_component(EntityID id);

template<typename C>
C const* get_component(EntityID id) const;

template<typename C>
bool has_component(EntityID id) const;
```

- `get_component<C>(id)` returns a pointer to the entity's `C`, or `nullptr` if the entity is dead, the id is stale/invalid, the entity's archetype does not carry `C`, or `C` is a tag type (tags have no data slot — see below). Mutating through the returned pointer is the normal way to write a single entity's data outside iteration.
- `has_component<C>(id)` answers archetype membership: `true` iff the entity is alive and its archetype carries `C`. It works for tags and non-tags alike. Returns `false` for dead or invalid ids.

Both have `ImmutableEntityID` overloads with identical semantics — reads (and data mutation) are always allowed on immutable entities; only the *component set* is frozen (see [entities.md](entities.md)).

There is no error distinction between "dead entity" and "doesn't have C": both are `nullptr`/`false`. Branch on `is_alive(id)` first if you need to tell them apart.

### Pointer lifetime — the most important rule on this page

**Component pointers are short-lived.** A `C*` from `get_component` (or a reference received inside `for_each`) is valid only until the next structural change touching that archetype:

- `create_entity` of an entity with the same signature (may allocate, and fills rows),
- `destroy_entity` of any entity in the archetype (swap-pop relocates the archetype's last row into the hole),
- `add_component` / `remove_component` on any entity entering or leaving the archetype (migration + swap-pop compaction).

The pointed-to object can be moved out from under you without any fault at the old address — the classic symptom is stale data, not a crash. Never store a raw component pointer across structural mutations, and never across ticks. For a cross-tick reference, store the `EntityID` and re-resolve, or use `CachedRef<C>` ([entities.md](entities.md)), which revalidates against `component_version_in` automatically.

### `component_version_in` — cheap revalidation

```cpp
template<typename C>
uint64_t component_version_in(EntityID id) const;
```

Returns the component-column version for `C` in the entity's current archetype, or `0` if the entity is dead or no longer carries `C`. The version monotonically increases on every structural change to that column (push, swap-pop, relocate), so **a stable version implies cached pointers into the column are still valid**. A live entity that carries `C` always reports a version `> 0`, so `0` is unambiguous.

Notes:

- The version is per *column in the archetype*, not per entity — an unrelated entity joining or leaving the same archetype bumps it. That is exactly the conservatism pointer caching needs.
- Only the *numeric ordering for one column within one run* is meaningful. Bulk creation bumps versions once per touched chunk instead of once per row, so never compare version values across runs or use them as data — they only signal "this column changed".
- An `ImmutableEntityID` overload exists with identical behaviour.

This is the primitive `CachedRef<C>::get(World&)` is built on (`src/openvic-simulation/ecs/CachedRef.hpp`): it re-resolves the pointer only when the live version differs from the cached one.

### Example: reads, writes, and what `nullptr` means

```cpp
World world;
EntityID const a = world.create_entity(Health { 10, 100 });
EntityID const b = world.create_entity(Health { 20, 200 }, Velocity { 1.0f, 0.0f });

// Mutate through the pointer — valid because nothing structural happens in between.
Health* h = world.get_component<Health>(a);
h->hp = 999;

// Membership checks.
bool const b_moves = world.has_component<Velocity>(b);  // true
bool const a_moves = world.has_component<Velocity>(a);  // false
Velocity* v = world.get_component<Velocity>(a);          // nullptr — not in a's archetype

// Dead entities read as "nothing".
world.destroy_entity(b);
Health* gone = world.get_component<Health>(b);           // nullptr
bool const still = world.has_component<Health>(b);       // false

// NOTE: destroy_entity(b) was a structural change — `h` from above is now suspect
// if a and b shared an archetype. Re-resolve before further use.
h = world.get_component<Health>(a);
```

## `add_component` / `remove_component`

```cpp
template<typename C>
C* add_component(EntityID id, C&& value);

template<typename C>
C* add_component(EntityID id); // default-construct overload

template<typename C>
bool remove_component(EntityID id);
```

`add_component` attaches `C` to a living entity by migrating it to the archetype that extends its current one with `C`. Semantics:

- If the entity **already carries `C`**, the existing value is replaced in place (no migration) and the existing pointer is returned. Exception: for tag types there is no data, so this case returns `nullptr`.
- Returns `nullptr` if the entity is dead.
- Returns `nullptr` (with an error log) if the entity is **immutable** — `create_immutable_entity` entities refuse structural changes at runtime, and the `ImmutableEntityID` handle refuses them at compile time (there is deliberately no `add_component`/`remove_component` overload for it). See [entities.md](entities.md).
- Returns `nullptr` if called **mid-tick** — the refusal logs an error (loud-but-no-crash) and returns `nullptr`. Structural mutations inside a system must go through the command buffer (`ctx.cmd.add_component`), never `ctx.world.*`. See [command-buffer.md](command-buffer.md).
- The default-construct overload is convenient for tag types and for components whose default value is the right initial state.
- For tag types the migration succeeds but the return value is still `nullptr` (no data slot). Do not use the returned pointer as a success test for tags — check `has_component<Tag>(id)`.

`remove_component` migrates the entity to the archetype with `C` dropped. Returns `false` if the entity is dead, immutable, mid-tick, doesn't carry `C`, **or removing `C` would leave the entity with zero components** — a zero-component entity is an invariant the model doesn't allow; call `destroy_entity` instead.

Both are structural mutations: they invalidate component pointers into both the source and target archetypes (the migrated entity's own components move, and a swap-pop fills its old row), and they are exactly the per-entity cost you avoid by pre-attaching at creation time.

## Tag (zero-size) components

Any empty struct (`std::is_empty_v<C>`) is a **tag**: archetype membership without data. Tags are the right tool for boolean per-entity state that queries filter on — "is mobilised", "is colonial" — because the flag lives in the archetype signature, not in a byte column.

```cpp
struct Tag1 {};

ECS_COMPONENT(Tag1, "test_Tag::Tag1")
```

Internally a tag column has no slab at all (`ColumnVTable::size == 0`); only the archetype signature records it. Consequences you observe:

- **`get_component<Tag>(id)` returns `nullptr` by design** — there is no data slot to point at. Use `has_component<Tag>(id)` to test membership. This is the pitfall from `ECS.md`; the `nullptr` is not an error condition.
- Tags participate in queries and iteration normally: `for_each<Payload, Tag1>(...)` visits exactly the entities carrying both. The `Tag1&` the lambda receives is a reference to a static dummy instance — fine to accept, pointless to mutate.
- `add_component<Tag1>(eid)` / `remove_component<Tag1>(eid)` migrate between the tag-extended and tag-free archetypes like any other component (same migration cost!). `add_component` for a tag returns `nullptr` even on success.
- `component_version_in<Tag>(id)` works — tag columns still carry a version that bumps on push/swap-pop.
- In bulk creation (`create_entities`), tags are named in the component pack but take **no input span**.
- Tags are exempt from the checksum rule; their checksum contribution is presence, carried by the archetype signature.

Adapted from `tests/src/ecs/Tag.cpp`:

```cpp
World world;
world.create_entity(Payload { 1 });
world.create_entity(Payload { 2 }, Tag1 {});
world.create_entity(Payload { 3 }, Tag1 {});
world.create_entity(Payload { 4 }, Tag2 {});

int sum = 0;
world.for_each<Payload, Tag1>([&](Payload& p, Tag1&) {
	sum += p.v;
});
// sum == 5 — only the Tag1 carriers were visited.

EntityID const eid = world.create_entity(Payload { 5 });
world.add_component<Tag1>(eid);              // default-construct overload; returns nullptr for tags
bool const tagged = world.has_component<Tag1>(eid);   // true — THE membership test for tags
Tag1* p = world.get_component<Tag1>(eid);             // nullptr by design, not an error
```

## Singleton components

A singleton is a single per-type value owned directly by the `World` — global simulation state that doesn't belong on any particular entity (a clock, a registry, a per-session config blob). Singletons share `component_type_id_t` with entity components (one `ECS_COMPONENT` registration serves both uses) but live in their own type-erased slot, not in any archetype.

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

Semantics (all verified in `tests/src/ecs/Singleton.cpp`):

- `set_singleton(value)` stores the value and returns a pointer to it. If the singleton already exists, the existing object is **assigned in place — same allocation, same pointer**. The default-construct overload replaces an existing value with `C {}`.
- `get_singleton<C>()` returns the stored pointer, or `nullptr` when unset. Const overload returns `C const*`.
- `clear_singleton<C>()` destroys the value; returns `false` if it wasn't set.
- **Pointer stability:** unlike entity-component pointers, a singleton pointer is stable for the singleton's whole life — each singleton is an individually heap-allocated object, so entity/archetype operations never move it. It dies only on `clear_singleton<C>()` or `World` destruction.
- Lifetime is the `World`'s lifetime — singletons are *not* cleared by `clear_systems` or `end_game_session`.
- Tag (empty) types work as singletons (pure "flag is set" state); `get_singleton` returns a real (non-null) pointer in that case, since the singleton slot owns an actual object.
- Singleton types are subject to the same checksum rule as components — `set_singleton<C>` is the enforcement point (it captures the hash thunk that the full-state checksum walk uses; see [determinism.md](determinism.md)). Tag singletons are exempt.

### Singleton access inside a tick MUST be declared

This is the singleton pitfall from `ECS.md`, and it is the determinism bug you will not catch in a unit test. Singletons are not visible in any system's tick signature, so the scheduler cannot see your access unless you declare it:

- a system that calls `get_singleton<C>()` (read) inside its tick must declare `extra_reads() = { component_type_id_of<C>() }`;
- a system that mutates through the returned pointer must declare `extra_writes() = { component_type_id_of<C>() }` — serial `System<>` only; `extra_writes()` is forbidden on `SystemThreaded` (its chunks run concurrently, so a singleton write races the system with itself — `register_system` rejects it with a `static_assert`).

Why: the scheduler builds its parallel stages from declared access. An undeclared singleton is invisible to that model, so a writer and a reader can be co-scheduled into the same stage. The result is a data race that changes results with worker count and timing — silently breaking worker-count-invariant determinism. The worker-count gate catches it only probabilistically. See [systems.md](systems.md) for the declaration mechanics and [scheduling.md](scheduling.md) / [threading-and-reductions.md](threading-and-reductions.md) for what the scheduler guarantees.

### Create every singleton before the first `tick_systems`

Creating a *new* singleton mid-tick (`set_singleton` on a missing id) inserts into the World's singleton hashmap and can rehash it under a concurrent reader — even when both systems declared their access correctly. Create (at minimum default-construct) every singleton during session setup, before the first `tick_systems` call; inside ticks, only assign through `set_singleton` on already-existing singletons or mutate through `get_singleton` pointers, with the access declared.

```cpp
struct GameClock {
	int tick_count = 0;
};

ECS_COMPONENT(GameClock, "OpenVic::GameClock")

// --- session setup, before the first tick_systems ---
World world;
GameClock* clock = world.set_singleton(GameClock { 0 });

// --- inside a serial System<> that advances the clock ---
// The system must declare:
//   static constexpr std::array<component_type_id_t, 1> extra_writes() {
//       return { component_type_id_of<GameClock>() };
//   }
// and may then, in its tick:
GameClock* c = ctx.world.get_singleton<GameClock>();
++c->tick_count;
```

## `DenseSlotAllocator` — deterministic rows for singleton side tables

Some singleton-owned state is per-entity but doesn't fit a component column — variable-width or shared tables ("side tables") indexed by a dense row number that the owning entity stores. `DenseSlotAllocator` (`src/openvic-simulation/ecs/DenseSlotAllocator.hpp`) hands out those rows deterministically.

```cpp
inline constexpr uint32_t INVALID_DENSE_SLOT = static_cast<uint32_t>(-1);

struct DenseSlotAllocator {
	struct Snapshot {
		uint32_t next_unallocated = 0;
		std::vector<uint32_t> free_slots; // LIFO stack order: back() is the next allocate() result
		bool operator==(Snapshot const&) const = default;
	};

	uint32_t allocate();
	void release(uint32_t slot);
	uint32_t high_water() const;
	uint32_t free_count() const;
	void reset();
	void snapshot(Snapshot& out) const;
	bool restore(Snapshot const& snapshot);
	bool debug_validate() const;
};
```

It is **not** an entity allocator: there are no generations. The durable identity of a row's owner is that entity's `EntityID`; the row is just packed storage.

- `allocate()` returns the next slot: top of the free stack if any (**LIFO** reuse), else high-water growth (`0, 1, 2, ...`). Returns `INVALID_DENSE_SLOT` (with an error log) on exhaustion (all 2^32 − 1 rows used).
- `release(slot)` pushes the slot for reuse. Releasing a slot `>= high_water()` is logged and ignored (an O(1) range check). **Double-releasing a valid slot is a contract violation that is NOT detected per-release** — a per-call O(n) scan would make mass end-of-session sweeps O(n²). Tests and debug sweeps can call `debug_validate()` — a one-shot O(n log n) check that every free slot is in range and appears exactly once.
- `high_water()` is the number of rows ever allocated — the size the owning side table must reserve. It never shrinks except via `reset()` / `restore()`.
- `reset()` forgets everything — the `end_game_session` sweep for the owning side table.
- `snapshot(out)` / `restore(snapshot)` mirror the World's identity save/load ([world.md](world.md)): snapshot between ticks, restore into a fresh (or `reset`) allocator, and subsequent allocations continue *exactly* as in the never-saved run. `restore` validates fully first (every free slot `< next_unallocated`, no duplicates); on failure it returns `false` with an error log and the allocator is untouched.

**Determinism contract (lockstep multiplayer):** the allocation order is a pure function of the alloc/release call sequence. Therefore `allocate`/`release` may be called **only from serial systems** (`System<>` tick bodies), from CommandBuffer-apply-adjacent serial code, or outside ticks — **never from `SystemThreaded` tick bodies**, whose execution order is worker-count-dependent. Same discipline as the deferred-create path: structural decisions funnel through a serial point. See [determinism.md](determinism.md) and [threading-and-reductions.md](threading-and-reductions.md).

**Explicit release, no RAII** (house rule from `ECS.md`): call `release(slot)` at the callsite, exactly once per live slot — never from a destructor or handle wrapper. Audit-ability wins: in a determinism-critical codebase you must be able to grep every point where the allocation sequence changes.

Adapted from `tests/src/ecs/DenseSlotAllocator.cpp`:

```cpp
DenseSlotAllocator alloc;

// Dense growth: 0, 1, 2, 3, 4.
for (uint32_t expected = 0; expected < 5; ++expected) {
	uint32_t const slot = alloc.allocate();
	// slot == expected
}

alloc.release(2);
alloc.release(0);

uint32_t const r0 = alloc.allocate(); // 0 — LIFO: last released first
uint32_t const r1 = alloc.allocate(); // 2
uint32_t const r2 = alloc.allocate(); // 5 — free stack drained, high-water growth
// alloc.high_water() == 6

// Save/load: an identical further call script on the original and on a restored
// copy produces identical slot sequences and identical end snapshots.
DenseSlotAllocator::Snapshot snap;
alloc.snapshot(snap);

DenseSlotAllocator restored;
if (!restored.restore(snap)) {
	// invalid snapshot — restored is untouched
}
```

## Quick rules recap

- Register every component once with `ECS_COMPONENT(Type, "Globally::Unique::Name")` at global namespace scope; never rename the literal once it can be persisted.
- Satisfy the checksum rule: uniquely representable bytes, or a custom `ecs_checksum`, or `ECS_CHECKSUM_BYTES` (tags exempt). No raw pointers in components.
- Pre-attach components at `create_entity` time; `add_component`/`remove_component` are per-entity archetype migrations.
- Component pointers are short-lived — re-resolve after any structural change; across ticks use `CachedRef<C>` or re-look-up by `EntityID`; `component_version_in` tells you when a re-resolve is needed.
- `get_component<Tag>` is `nullptr` by design — use `has_component<Tag>`.
- No structural mutation mid-tick — `ctx.cmd.*`, never `ctx.world.*` ([command-buffer.md](command-buffer.md)).
- Declare singleton access inside ticks (`extra_reads()` / `extra_writes()`); create every singleton before the first `tick_systems`.
- `DenseSlotAllocator`: serial-only alloc/release, explicit `release` exactly once per slot, `reset()` at end of session, snapshot/restore between ticks.
- No `*Manager` wrappers around component access — use free functions taking `ecs::World&`, a singleton, or a System (`ECS.md` house rule).

## Source files

- src/openvic-simulation/ecs/ComponentTypeID.hpp — `component_type_id_t`, `fnv1a_64`, `ComponentName`, `component_type_id_of`, `ECS_COMPONENT`
- src/openvic-simulation/ecs/World.hpp — `create_entity`, `add_component`, `remove_component`, `get_component`, `has_component`, `component_version_in`, `set_singleton`, `get_singleton`, `clear_singleton`
- src/openvic-simulation/ecs/ChecksumTraits.hpp — the checksum contract: `is_checksummable_v`, `ecs_checksum` convention, `ECS_CHECKSUM_BYTES`
- src/openvic-simulation/ecs/Archetype.hpp — `ColumnVTable` (the move/destroy/hash operations a component type must support)
- src/openvic-simulation/ecs/CachedRef.hpp — version-validated cross-tick component pointer
- src/openvic-simulation/ecs/DenseSlotAllocator.hpp / src/openvic-simulation/ecs/DenseSlotAllocator.cpp — deterministic dense rows for singleton side tables
- tests/src/ecs/Component.cpp, tests/src/ecs/Tag.cpp, tests/src/ecs/Singleton.cpp, tests/src/ecs/DenseSlotAllocator.cpp, tests/src/ecs/FNVHash.cpp — executable examples of everything above
