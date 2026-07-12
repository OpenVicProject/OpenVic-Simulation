# Determinism and checksums

OpenVic targets lockstep multiplayer: every peer runs the same simulation and must produce **bit-identical state** every tick, regardless of CPU, worker-thread count, or whether the session was freshly started or loaded from a save. This page is the contract that makes that work — the rules your game code must follow, the ordering guarantees the ECS gives you in return, and the checksum machinery (`src/openvic-simulation/ecs/Checksum.hpp`) that measures whether two worlds are actually in the same state.

The short version: write per-row integer/fixed-point arithmetic, declare everything you touch, never let a thread id or a memory address influence a result — and the worker-count-invariance tests will hold you to it.

## Rules your game code must follow

### 1. No float math — `fixed_point_t` everywhere

Floating-point results vary with compiler, optimization flags, FMA contraction, and x87-vs-SSE — identical source code can produce different bits on two peers. All simulation arithmetic uses `fixed_point_t`. The checksum machinery enforces this structurally: `std::has_unique_object_representations_v` is false for types with `float`/`double` members, so a component carrying one fails the checksummable check at compile time (see [Making a type checksummable](#making-a-type-checksummable-checksumtraitshpp) below). The `ECS_CHECKSUM_BYTES` escape hatch exists for author-asserted cases, but using it on a float field is you asserting the values were produced deterministically — which simulation math with raw floats cannot guarantee.

### 2. No thread-schedule-dependent results — use `reductions::*`

Any fold whose accumulation order depends on which worker ran first (a shared accumulator, a per-`worker_id` partial array folded in completion order) produces different results at different worker counts. Use the helpers in `src/openvic-simulation/ecs/Reductions.hpp` — `parallel_sum`, `parallel_min`, `parallel_max`, `parallel_keyed_sum`. They buffer per-chunk results keyed by `chunk_idx` and fold them **sequentially in `chunk_idx` ascending order** after the parallel section joins, so the result is bit-identical regardless of worker count. See threading-and-reductions.md for usage.

For the same reason, never key anything off the `worker_id` your code can observe, and never read wall-clock time, thread ids, or process-local RNG inside a tick.

### 3. Declare every singleton you touch inside a tick

Singletons are not visible in a system's tick signature, so the scheduler cannot see them unless you declare them:

```cpp
// Read via ctx.world.get_singleton<GameClock>() in tick:
static constexpr std::array<component_type_id_t, 1> extra_reads() {
	return { component_type_id_of<GameClock>() };
}

// Mutation through the returned pointer (serial System<> only):
static constexpr std::array<component_type_id_t, 1> extra_writes() {
	return { component_type_id_of<SpawnCounter>() };
}
```

**Why this is a determinism rule and not just a style rule:** an undeclared singleton is invisible to the scheduler's access model. A writer and a reader of the same singleton can then be co-scheduled into one parallel stage, and the reader observes the write or not depending on the thread schedule — a silent desync. The worker-count gate catches this only *probabilistically* (the race has to actually fire during the test run), so the declaration is the real defense. `extra_writes()` is statically forbidden on `SystemThreaded` — a chunk-parallel system writing a shared target would race with itself. One exception: singleton reads inside a `should_run` gate need no `extra_reads()` declaration — it is evaluated once per tick on the main thread while no workers run. See systems.md.

### 4. Create every singleton before the first `tick_systems`

Calling `set_singleton` for a *new* type mid-tick mutates the World's singleton hashmap and can rehash it under concurrent readers — undefined behavior, not merely nondeterminism. Create all singletons during session setup; mutating an *existing* singleton's value mid-tick is fine when declared via `extra_writes()`.

### 5. No raw pointers in components — references are `EntityID` or dense indices

A raw pointer is uniquely representable, so a byte-hashed component containing one compiles cleanly — and then hashes a memory address, which differs every run. This silently breaks every determinism gate (worker-count invariance, golden runs, save/load). It is the one rule the compiler cannot check for you. Store `EntityID` (hash-stable and save-stable — see below) or a dense `uint32_t` index instead. If a type genuinely must hold a pointer, give it a custom `ecs_checksum` that hashes the pointee's deterministic content or mere presence, never the address.

### 6. Keep `should_run` pure over deterministic state

The optional `static bool should_run(TickContext const&)` cadence gate must be a pure function of `ctx.today` and singletons read via `ctx.world`. It runs on every peer every tick; if it reads anything per-machine, peers diverge on *whether a system ran at all*. The skip is dispatch-time only — `schedule_hash()` is untouched — so gating never perturbs the schedule. Full contract in `src/openvic-simulation/ecs/System.hpp` and systems.md.

### 7. Don't make logic sensitive to packing order across save/load

Iteration order (chunk-then-row) is deterministic *within a run*, but row packing is **not** preserved across save/load — a restored World repacks entities in canonical slot-index order, while the never-saved run's packing is scrambled by historical swap-pop compaction. Pure per-row arithmetic is packing-invariant and safe. Anything **id-assignment-sensitive** — code whose observable result depends on the order it executes, most importantly loops that call `ctx.cmd.create_entity` per visited row — must iterate in EntityID / dense-index order, never raw chunk order, or the restored run will assign different ids than the continued run. `tests/src/ecs/IdentitySnapshotInvariance.cpp` demonstrates both sides of this rule.

## What the ECS guarantees in return

Given identical inputs and game code that follows the rules above, the ECS guarantees:

- **Deterministic iteration order within a run.** `for_each` / `for_each_with_entity` / `for_each_chunk` walk matched archetypes in a stable order, chunks ascending, rows ascending. Archetype creation order — and therefore packing — is deterministic given an identical sequence of structural operations, and identical across worker counts.
- **Worker-count-invariant scheduling.** The scheduler builds a stable topological schedule from declared dependencies and auto-resolved access conflicts; stage results are barrier-separated. The result of a tick is bit-identical at any worker count, including `set_serial_mode(true)`.
- **Worker-count-invariant deferred structural ops.** `ctx.cmd.create_entity` from a `SystemThreaded` body records into per-chunk buffers; placeholders are resolved to real `EntityID`s at apply time, on a single thread, in `chunk_idx` ascending order — so allocation order cannot depend on which worker finished first. See command-buffer.md.
- **Bulk create ≡ loop.** `World::create_entities` yields the *identical* end state — including identical `EntityID` assignment — as the equivalent `create_entity` loop (same free-list pops, same row packing). See entities.md.
- **Checksum-stable singletons.** Singleton storage is a hashmap with nondeterministic iteration order, but the checksum walk folds singletons in ascending `component_type_id_t` order regardless of the order `set_singleton` was called.
- **A schedule handshake.** Peers verify they registered the same systems before simulating together:

```cpp
uint64_t schedule_hash();
```

FNV-1a over the `(stage_index, system_type_id_t)` pairs of the current schedule (`src/openvic-simulation/ecs/World.hpp`). Multiplayer peers compute this at session-start handshake; a mismatch rejects the join. Registration *within* a conflict-free stage is order-insensitive — `tests/src/ecs/SystemFiltersWorkerCountInvariance.cpp` asserts that registering four co-staged systems in reverse order produces the same hash. See scheduling.md.

## EntityID stability across save/load

`EntityID` is `{ uint32_t index, uint32_t generation }` (`src/openvic-simulation/ecs/EntityID.hpp`). Ids are **save-stable**: the identity layer (slot generations, immutability flags, free-list order) can be snapshotted and restored exactly, so an `EntityID` stored inside a component means the same thing after a load as it did in the never-saved run. This is why checksumming hashes `EntityID` fields raw, and why components reference other entities by id rather than pointer.

```cpp
bool snapshot_identity(WorldIdentitySnapshot& out) const;
bool restore_identity(WorldIdentitySnapshot const& snapshot);

template<typename... Cs>
bool restore_entity(EntityID eid, Cs&&... values);
```

- `snapshot_identity` captures the identity layer **only** — per-slot generations, per-slot immutability, free-list order (`WorldIdentitySnapshot` in `src/openvic-simulation/ecs/World.hpp`). Archetypes, packing, singletons, and systems are deliberately not captured; the loader rebuilds them. Refuses (error log + `false`) mid-tick or while any reserved-but-unfinalised slot exists (a `CommandBuffer` holding un-applied creates) — snapshot only between ticks, after every buffer has applied. It also validates the free chain and refuses to save a corrupt one.
- `restore_identity` requires a **fresh** World (no entity slot ever allocated), outside any tick. The snapshot is fully validated before any mutation; on failure the World is untouched. Afterward every live slot is reserved-but-unfinalised: addressable at its original `(index, generation)` but `is_alive == false` until finalised.
- `restore_entity` finalises one restored slot with its components (same component rules as `create_entity`). Between `restore_identity` and the last `restore_entity`, the **only** legal entity operations are `restore_entity` calls — in particular, `destroy_entity` on a not-yet-finalised id would push the slot onto the free list and silently corrupt the restored order. Recreate live entities in **slot-index ascending order**: identity correctness is order-independent, but packing is not, and the canonical order is what makes packing reproducible across loads.

The guarantee you get: the same allocation-request sequence after load produces the same ids as the never-saved run — including free-list reuse, so entities spawned post-load receive the exact `EntityID`s they would have received without the save/load cycle. Producing the same request sequence is *your* obligation (rule 7 above). The gate for all of this is `tests/src/ecs/IdentitySnapshotInvariance.cpp`: `digest(tick^k(restore(snapshot(s)))) == digest(tick^k(s))` at every worker count. Full API reference in world.md.

## The full-state checksum (`Checksum.hpp`)

The checksum is the measuring instrument behind every determinism gate: a deterministic 64-bit FNV-1a digest of **all live ECS state**. Equal totals (on same-endian platforms — cross-endian comparison is out of scope for the project) imply equal live state under the canonical walk.

```cpp
// The checksum.
uint64_t world_checksum(World const& world);

// Same walk, with the per-part breakdown filled in.
// Invariant: result.total == world_checksum(world) == fold_checksum_breakdown(result).
WorldChecksumBreakdown world_checksum_breakdown(World const& world);

// Recomputes the total from the entries alone.
uint64_t fold_checksum_breakdown(WorldChecksumBreakdown const& breakdown);
```

### What it covers

The walk is plain memory order — canonical in this project because packing is deterministic within a run and across worker counts:

- **Every live entity row**: per archetype (by index; archetypes with no live entities are *skipped*, so the digest is insensitive to dead archetype-creation history), the sorted signature first — tag components contribute here, presence only — then chunks ascending; per chunk the `EntityID` `(index, generation)` pairs row-ascending, then component data column by column.
- **Every singleton**, in ascending component-id order regardless of `set_singleton` call order.

Notable observable consequences (all asserted in `tests/src/ecs/Checksum.cpp`):

- Flipping one component field, destroying an entity, or mutating a singleton (including one element inside a heap-holding singleton) changes the checksum.
- A destroy-then-recreate with identical component values still changes the checksum: slot reuse bumps the generation, and generations are part of live state — a peer that diverged in entity history is *supposed* to mismatch.
- Two worlds built by identical creation scripts checksum equal, even if one of them once held a since-drained archetype the other never created.

### What it does not cover

The query cache, the chunk pool, the system registry and schedule (`schedule_hash()` covers the schedule separately), and dead-slot bookkeeping (free-list order, dead generations — those belong to `snapshot_identity`). The checksum is **read-only by construction**: it walks the archetype vector directly, never touches the query cache, never mutates the World — running queries between two checksum calls does not move the value.

**Threading:** call it between ticks only. It takes `World const&` and mutates nothing, but it reads component data unsynchronised — computing it while `tick_systems` is running would race with component writes.

### Divergence debugging with the breakdown

```cpp
struct ArchetypeChecksumEntry {
	// Index into the World's archetype vector (memory order = canonical walk order).
	uint32_t archetype_index = 0;
	// Sorted component ids — copied so breakdown dumps are self-describing.
	std::vector<component_type_id_t> signature;
	// Self-contained: folded from CHECKSUM_SEED over the signature, then the rows.
	uint64_t hash = 0;
};

struct SingletonChecksumEntry {
	component_type_id_t id = 0;
	uint64_t hash = 0;
};

struct WorldChecksumBreakdown {
	std::vector<ArchetypeChecksumEntry> archetype_entries; // memory order, empties skipped
	std::vector<SingletonChecksumEntry> singleton_entries; // ascending component id
	uint64_t total = 0;
};
```

When two peers' totals differ, the first question is always *where*. Each entry's `hash` is self-contained (folded from `CHECKSUM_SEED`), so two peers can exchange breakdowns and diff entry-by-entry to find the first diverging archetype or singleton instead of bisecting blind.

## Making a type checksummable (`ChecksumTraits.hpp`)

Every component or singleton type `C` must hash exactly one of two ways — a type satisfying neither is a **compile error at registration/use** (a `static_assert` with a full fix-it message), never a silent skip:

1. **Raw bytes** — automatic, nothing to write, allowed only if `std::has_unique_object_representations_v<C>` holds: no `float`/`double` members and **no compiler-inserted padding**. Padding bytes are indeterminate garbage; zero-initialized chunk memory does *not* save you, because whole-struct copies from stack temporaries import the source's garbage padding. Fill gaps with explicit `_pad` members, zeroed at construction.
2. **A custom hash** — a free function with exactly this shape, found by ADL at `C`'s scope, declared **before `C`'s first `World`/`CommandBuffer` use in the translation unit**:

   ```cpp
   uint64_t ecs_checksum(C const& value, uint64_t seed);
   ```

   **Mandatory** for anything holding heap data (`memory::vector` members, `std::string`, ring buffers, ...): the byte path would hash the heap *pointer*, not the contents. The custom hash must walk the heap contents deterministically — sizes plus elements in index order, never capacities or addresses. A custom hash takes precedence over the byte path, so a uniquely-representable type may still provide one (e.g. to ignore scratch fields).

Tag (empty) components are exempt: their contribution is presence only, carried by the archetype-signature / singleton-id fold.

The traits and primitives you can use directly:

```cpp
inline constexpr uint64_t CHECKSUM_FNV_PRIME = 0x100000001b3ULL;
inline constexpr uint64_t CHECKSUM_SEED = 0xcbf29ce484222325ULL;

// FNV-1a over a raw byte range, continuing from `seed`.
inline uint64_t fnv1a_64_bytes(void const* data, std::size_t size, uint64_t seed);

// Endian-independent fold of one 64-bit value, low byte first.
constexpr uint64_t fold_uint64(uint64_t value, uint64_t seed);

template<typename C>
inline constexpr bool has_custom_checksum_v; // ADL detects ecs_checksum(C const&, uint64_t)

template<typename C>
inline constexpr bool is_checksummable_v;    // tag, custom hash, or uniquely representable

// Hash one value of C into `seed`: custom path > byte path; tags fold nothing.
template<typename C>
uint64_t checksum_value(C const& value, uint64_t seed);
```

Build custom hashes out of `fold_uint64` / `fnv1a_64_bytes` / nested `checksum_value` calls so every hash in the project is the same FNV-1a stream. `is_checksummable_v` is handy in `static_assert`s next to a component definition, catching a padding regression at the definition site rather than at first World use.

Enforcement happens automatically at the two registration points — instantiating a component's column vtable (first `World`/`CommandBuffer` use) and `World::set_singleton<C>` — so you cannot get an unhashable type into a World.

### Example: byte path, custom hash, and a heap-holding singleton

Adapted from `tests/src/ecs/Checksum.cpp`:

```cpp
#include "openvic-simulation/ecs/ChecksumTraits.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"

#include <vector>

using namespace OpenVic::ecs;

// Byte path: no floats, no padding, no pointers — nothing to write.
struct CkPair {
	int32_t a = 0;
	int32_t b = 0;
};

// Uniquely representable but carrying a custom hash that deliberately ignores a
// scratch field — the custom path takes precedence over the byte path.
struct CkCustom {
	int64_t counted = 0;
	int64_t ignored = 0;
};
inline uint64_t ecs_checksum(CkCustom const& c, uint64_t seed) {
	return fold_uint64(static_cast<uint64_t>(c.counted), seed);
}

// Heap-holding singleton: a custom hash is MANDATORY — walk size + elements in
// index order, never capacities or addresses.
struct CkVecSingleton {
	std::vector<int64_t> values;
};
inline uint64_t ecs_checksum(CkVecSingleton const& s, uint64_t seed) {
	uint64_t h = fold_uint64(s.values.size(), seed);
	for (int64_t value : s.values) {
		h = fold_uint64(static_cast<uint64_t>(value), h);
	}
	return h;
}

ECS_COMPONENT(CkPair, "test_Checksum::Pair")
ECS_COMPONENT(CkCustom, "test_Checksum::Custom")
ECS_COMPONENT(CkVecSingleton, "test_Checksum::VecSingleton")

// Catch padding/heap regressions at the definition site:
static_assert(is_checksummable_v<CkPair>, "two int32s, no padding");
static_assert(has_custom_checksum_v<CkCustom>, "ADL detection finds ecs_checksum");
```

### `ECS_CHECKSUM_BYTES` — author-asserted byte hashing

```cpp
#define ECS_CHECKSUM_BYTES(Type)
```

Generates an `ecs_checksum` overload that byte-hashes `Type`, for types `std::has_unique_object_representations_v` rejects *conservatively* (typically `float`/`double` members). Expand at `Type`'s scope, immediately after its definition — it works inside anonymous namespaces and is deliberately not namespace-wrapped so ADL finds it there. The one thing the macro *does* check: the generated overload `static_assert`s `std::is_trivially_copyable_v<Type>`, so expanding it on a non-trivially-copyable type is a hard compile error. Beyond that, by expanding it the author asserts three things the macro cannot check: no raw-pointer members, no implicit padding bytes (padding *will* produce unstable checksums), and float values produced deterministically. Prefer fixing the type over asserting.

## Worker-count invariance — the test gate

The contract test: same starting World + same input → bit-identical post-tick state for **any** worker count. Every gate follows the same shape — run the scenario at worker count 1 as the baseline, re-run at 1, 2, 4, 8, 16, and require equal digests. `wc=1` exercises the same code paths but is race-free by construction, which is what makes it a trusted baseline; `wc>=2` introduces real concurrency.

The full-state-checksum variant, adapted from `tests/src/ecs/Checksum.cpp` — use this as the template when adding a gate for new game systems:

```cpp
#include "openvic-simulation/ecs/Checksum.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/World.hpp"

namespace {
	// Threaded spawner: every CkSeed entity spawns one CkSpawned with a deterministic value.
	struct CkSpawner : SystemThreaded<CkSpawner> {
		void tick(TickContext const& ctx, EntityID, CkSeed const& s) {
			ctx.cmd.create_entity(ctx.world, CkSpawned { s.seed * 31 + 7 });
		}
	};
	// Serial churn: mutates CkValue on the seed entities every tick.
	struct CkChurn : System<CkChurn> {
		void tick(TickContext const& /*ctx*/, CkValue& v, CkSeed const& s) {
			v.v = v.v * 31 + s.seed * 7;
		}
	};
}
ECS_SYSTEM(CkSpawner)
ECS_SYSTEM(CkChurn)

namespace {
	uint64_t run_and_checksum(uint32_t worker_count, bool serial_mode, std::size_t seed_count, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count); // before the first tick_systems
		if (serial_mode) {
			world.set_serial_mode(true);
		}

		for (std::size_t i = 0; i < seed_count; ++i) {
			world.create_entity(
				CkSeed { static_cast<int64_t>((i * 17) % 251 + 1) },
				CkValue { static_cast<int64_t>(i + 1) }
			);
		}
		world.set_singleton(CkVecSingleton { { 4, 5, 6 } }); // singletons before first tick

		world.register_system<CkSpawner>();
		world.register_system<CkChurn>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		return world_checksum(world);
	}
}

TEST_CASE("Full-state checksum is identical across worker counts and serial mode",
          "[ecs][Checksum][determinism][WorkerCountInvariance]") {
	std::size_t const seeds = 200;
	int const ticks = 5;
	uint64_t const baseline = run_and_checksum(1, false, seeds, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		uint64_t const result = run_and_checksum(wc, false, seeds, ticks);
		CHECK(result == baseline);
	}

	uint64_t const serial = run_and_checksum(1, true, seeds, ticks);
	CHECK(serial == baseline);
}
```

The two World knobs the harness uses (`src/openvic-simulation/ecs/World.hpp`):

```cpp
// Override the ECS worker count. Call before the first `tick_systems` invocation.
// 0 → defaults to hw_concurrency, capped at 16.
void set_ecs_worker_count(uint32_t count);

// Force the scheduler to run every stage on the calling thread. Used for tests
// to validate "parallel result == serial result". Default false.
void set_serial_mode(bool enabled);
```

The existing gates, each guarding a different determinism risk:

| Test | What it gates |
|---|---|
| `tests/src/ecs/WorkerCountInvariance.cpp` | Serial and threaded per-row systems; deferred `cmd.create_entity` from `SystemThreaded` (placeholder resolution order), single- and multi-tick. |
| `tests/src/ecs/SystemFiltersWorkerCountInvariance.cpp` | Filtered systems (`Filter<Without<...>>`) in multi-system parallel stages — the worker-side query-cache lookup path — plus the disjoint-iteration same-component-writer override, and `schedule_hash` stability under registration reordering. |
| `tests/src/ecs/Checksum.cpp` | Full-state checksum determinism, sensitivity, and the checksum-based worker-count gate above. |
| `tests/src/ecs/IdentitySnapshotInvariance.cpp` | Save/load: `digest(tick^k(restore(snapshot(s)))) == digest(tick^k(s))` at every worker count, including exact `EntityID` reuse from the restored free list. |

Run them with `scons run_ovsim_tests=yes`. When you add game systems, extend the gate: a worker-count sweep over a realistic scenario, digested with `world_checksum`, is cheap to write and catches the whole class of races and order-dependencies that code review misses. Remember the gate's limits, though — it catches scheduling races only probabilistically. The declarations (rule 3) and the type-level checksum enforcement are what make determinism hold by construction; the gate is the alarm, not the lock.

## Cross-references

- world.md — full `World` API including snapshots/save-load and singletons
- systems.md — `SystemAccess`, `extra_reads()` / `extra_writes()`, `should_run`
- scheduling.md — stages, conflict resolution, `schedule_hash`
- threading-and-reductions.md — what is safe in parallel, `reductions::*`
- command-buffer.md — deferred structural ops and apply-time ordering
- entities.md — `EntityID`, bulk creation, immutable entities
- pitfalls.md — the consolidated rules list

## Source files

- src/openvic-simulation/ecs/Checksum.hpp — `world_checksum`, `world_checksum_breakdown`, `fold_checksum_breakdown`, breakdown structs
- src/openvic-simulation/ecs/Checksum.cpp — the canonical walk implementation
- src/openvic-simulation/ecs/ChecksumTraits.hpp — the per-type hashing contract, traits, primitives, `ECS_CHECKSUM_BYTES`
- src/openvic-simulation/ecs/World.hpp — `schedule_hash`, `set_ecs_worker_count`, `set_serial_mode`, `snapshot_identity` / `restore_identity` / `restore_entity`, `WorldIdentitySnapshot`
- src/openvic-simulation/ecs/Reductions.hpp — deterministic parallel folds
- src/openvic-simulation/ecs/EntityID.hpp — `EntityID` / `ImmutableEntityID`
- tests/src/ecs/WorkerCountInvariance.cpp, tests/src/ecs/SystemFiltersWorkerCountInvariance.cpp, tests/src/ecs/Checksum.cpp, tests/src/ecs/IdentitySnapshotInvariance.cpp — the determinism gates
