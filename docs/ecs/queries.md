# Queries

A `Query` describes *which archetypes* an iteration visits: a set of required components plus a set of excluded components. You build one with `with<Cs...>()` / `exclude<Cs...>()` / `build()`, then pass it to one of `World`'s `for_each` overloads — or, for systems, declare the exclude set declaratively with `using Filters = ecs::Filter<ecs::Without<C>>;`. Matching is exact and deterministic, and results are cached by the `World` so repeating the same query every tick is cheap.

Queries match **archetypes, not individual entities**. An entity is visited if and only if its archetype's component set is a superset of the require list and disjoint from the exclude list. There is no per-entity predicate — if you need per-entity conditions, test them inside the loop body. See [storage-model.md](storage-model.md) for why this archetype-granular model is what makes iteration fast.

## Building a `Query`

Defined in src/openvic-simulation/ecs/Query.hpp:

```cpp
struct Query {
	std::vector<component_type_id_t> require_ids;
	std::vector<component_type_id_t> exclude_ids;

	template<typename... Cs>
	Query& with();

	template<typename... Cs>
	Query& exclude();

	Query& build();

	bool operator==(Query const& other) const;
};
```

- `with<Cs...>()` appends the component ids of `Cs...` to `require_ids`. An archetype must contain **all** of them to match.
- `exclude<Cs...>()` appends to `exclude_ids`. An archetype containing **any** of them is rejected.
- Both may be called multiple times; the lists accumulate.
- `build()` sorts and deduplicates both lists and returns `*this` for chaining. It is idempotent — calling it again is harmless.
- `operator==` compares both lists. Because `build()` canonicalises order, two queries built from the same component sets compare equal regardless of the order you listed them in (`a.with<QA, QB>()` equals `b.with<QB, QA>()` after `build()`).

**Contract: call `build()` exactly once before passing the Query to any `for_each` overload.** The `World` compares the lists against canonical *sorted* archetype signatures and uses them as a hash key for the query cache; an unbuilt (unsorted, possibly duplicated) list silently produces wrong matches and useless cache keys. After `build()`, the same `Query` may be reused indefinitely — across calls and across ticks — as long as you make no further `with` / `exclude` calls. If you do add more, call `build()` again before the next use.

Typical shape:

```cpp
ecs::Query q;
q.with<Wealth>().exclude<Frozen>().build();
```

Reuse built queries where you can: it skips the (small) re-sort, and it makes the call sites self-documenting.

### Determinism

Component ids are compile-time FNV-1a hashes of the `ECS_COMPONENT` name literal (src/openvic-simulation/ecs/ComponentTypeID.hpp), so they are byte-identical across builds, platforms and machines. `build()`'s sorted order, query equality, and the `World`'s cache keys are therefore all stable — a query built on one multiplayer peer means exactly the same thing on another. See [determinism.md](determinism.md).

## Matcher semantics

After `build()`, an archetype matches when:

- its signature contains **every** id in `require_ids`, and
- its signature contains **none** of the ids in `exclude_ids`.

Consequences worth knowing:

- **Supersets match.** `with<A>()` visits entities of archetype `{A}`, `{A, B}`, `{A, B, C}`, … — anything carrying `A`.
- **An empty exclude list is plain superset matching.** The convenience overloads `for_each<Cs...>(fn)` (no `Query` argument) are exactly `with<Cs...>()` with an empty exclude set.
- **Excluding a component that no entity carries excludes nothing.** `Without` a never-instantiated component matches every archetype that satisfies the require list (verified in tests/src/ecs/SystemFilters.cpp).
- **There is no "match everything" query.** Every `for_each` variant statically requires at least one component (`static_assert`: "for_each requires at least one component"), so a broad query needs at least one anchor component that everything you care about carries.

### Matcher hashing — what you can (and cannot) observe

Internally each archetype carries a 64-bit `matcher_hash` bitfield (one bit per component, derived from `id % 63`), and query resolution uses it as an O(1) prefilter before an exact sorted-set walk. Two things matter to you:

1. **Results are always exact.** Bit collisions (`id % 63` maps many ids to one bit) are confirmed by the exact walk; the bitfield never produces false matches or false rejections. tests/src/ecs/MatcherHash.cpp pins this behaviour.
2. **It is deterministic.** The bits derive from the stable FNV component ids, so prefilter behaviour — and therefore performance — is identical across runs and machines.

You never compute or compare matcher hashes in game code.

## Iterating with a query: the `for_each` family

All six variants live on `World` (src/openvic-simulation/ecs/World.hpp):

```cpp
// Visit every entity whose archetype contains all of Cs..., calling fn(C&...) per row.
template<typename... Cs, typename Fn>
void for_each(Fn&& fn);

// Same, but the function also receives the EntityID.
template<typename... Cs, typename Fn>
void for_each_with_entity(Fn&& fn);

// Query overloads — match archetypes against Query::require_ids and reject any that
// overlap Query::exclude_ids.
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

Shared rules:

- **The lambda signature reflects `Cs...`, never the exclude set.** `for_each<Wealth>(q, fn)` calls `fn(Wealth&)` even if `q` excludes three other components. Excludes shape *which archetypes* are visited, not *what data* you receive.
- **Every non-tag component in `Cs...` must be in the query's require set.** The query overloads fetch a column for each `C` in `Cs...` from every matched archetype; if a matched archetype lacks that column the access is undefined behaviour. The safe pattern is to build the query with `with<Cs...>()` (plus any extra require-only components) — `Cs...` may be a *subset* of the require list, never a superset. A common use of the subset form: require a tag in the `Query` but leave it out of the lambda, since tags carry no data anyway (`q.with<Wealth, Migrant>().build(); world.for_each<Wealth>(q, ...)`).
- **Tag (zero-size) components in `Cs...` bind to a shared static dummy instance.** You can name them in the pack and the code compiles and runs, but the reference carries no per-row data — presence is conveyed by archetype membership. (Same reason `get_component<Tag>` returns `nullptr`; see [components.md](components.md).)
- **References are valid only inside the callback, and only structurally-quiescent.** Component references point straight into chunk columns. Any structural change — `create_entity`, `destroy_entity`, `add_component`, `remove_component` — can relocate rows and invalidate everything. **Do not structurally mutate the `World` from inside a `for_each` body.** Mutating component *data* in place is fine and is the normal use.
- **`for_each_with_entity` exists for the collect-then-act pattern.** You cannot destroy during iteration without invalidating columns, so collect `EntityID`s and act after the loop (example below). Inside systems, queue structural ops through `ctx.cmd` instead — see [command-buffer.md](command-buffer.md).
- **Nested `for_each` on the same thread is fine if read-only/in-place.** Iterating inside another iteration's body works (tested in tests/src/ecs/Iteration.cpp) — the danger is structural mutation, not nesting.
- **These calls are not thread-safe against each other.** Call them from one thread at a time (in practice: the main thread, outside `tick_systems`). Even a "read-only" `for_each` may write the query cache on a miss. The only sanctioned parallel iteration is through systems under the scheduler — see [systems.md](systems.md) and [threading-and-reductions.md](threading-and-reductions.md).

Iteration order is matched-archetype order, then chunk order, then row order — fully deterministic for a given operation history. Do **not** bake row/chunk order into id-assignment-sensitive logic, though: row packing is deliberately not part of the save contract, so it can differ after a load ([determinism.md](determinism.md)).

### Example: filtered iteration + collect-then-destroy

Adapted from tests/src/ecs/Iteration.cpp:

```cpp
struct Health {
	int32_t hp = 0;
};
struct Buried {}; // tag: already processed, skip forever

ECS_COMPONENT(Health, "example::Health")
ECS_COMPONENT(Buried, "example::Buried")

void bury_the_dead(ecs::World& world) {
	ecs::Query q;
	q.with<Health>().exclude<Buried>().build();

	// Collect first — destroying mid-iteration would invalidate the columns
	// the loop is reading.
	std::vector<ecs::EntityID> to_destroy;
	world.for_each_with_entity<Health>(q, [&](ecs::EntityID eid, Health& h) {
		if (h.hp <= 0) {
			to_destroy.push_back(eid);
		}
	});

	for (ecs::EntityID const& eid : to_destroy) {
		world.destroy_entity(eid);
	}
}
```

### Example: chunk-granular iteration

`for_each_chunk` hands you one `ChunkView<Cs...>` per non-empty chunk: raw, contiguous, aligned component slabs plus the parallel `EntityID` array, all of length `view.count()`. Use it when the inner loop wants tight, function-call-free array access (SIMD-friendly). The view — like everything it points at — is valid only inside the callback. `array<C>()` returns `nullptr` for tag types; never dereference those.

```cpp
int64_t total_wealth(ecs::World& world, ecs::Query const& q) {
	int64_t total = 0;
	world.for_each_chunk<Wealth>(q, [&](ecs::ChunkView<Wealth> view) {
		Wealth* OV_RESTRICT wealth = view.array<Wealth>();
		for (std::size_t i = 0; i < view.count(); ++i) {
			total += wealth[i].value;
		}
	});
	return total;
}
```

(`ChunkView` is defined in src/openvic-simulation/ecs/ChunkView.hpp; it is the same type `ChunkSystem` hands to `tick_chunk` — see [systems.md](systems.md).)

## System filters: `Filter` and `Without`

Systems don't construct `Query` objects by hand. A system's **require** set comes from its tick parameter pack (`System<>` / `SystemThreaded<>`) or its template component list (`ChunkSystem<>`). To add an **exclude** set, declare a `Filters` member alias using the vocabulary in src/openvic-simulation/ecs/QueryFilter.hpp:

```cpp
// Exclusion marker: archetypes containing C are not iterated.
template<typename C>
struct Without {};

// A system's filter set. Currently only Without<C> entries are supported.
template<typename... Fs>
struct Filter {
	static std::vector<component_type_id_t> exclude_ids();
};
```

Usage (adapted from tests/src/ecs/SystemFilters.cpp):

```cpp
struct Population {
	int64_t size = 0;
};
struct Dormant {}; // tag: province not in play this session

ECS_COMPONENT(Population, "example::Population")
ECS_COMPONENT(Dormant, "example::Dormant")

struct GrowthSystem : ecs::System<GrowthSystem> {
	using Filters = ecs::Filter<ecs::Without<Dormant>>;

	void tick(ecs::TickContext const& ctx, Population& pop) {
		pop.size += pop.size / 100;
	}
};
ECS_SYSTEM(GrowthSystem)
```

Semantics and contracts:

- `Without<C>` makes the system's iteration **skip every archetype that carries `C`**. It only adds to the exclude set; the require set is untouched (still exactly the tick pack / template list).
- A system with no `Filters` alias behaves as before — empty exclude list. (`system_filters_t<S>` defaults to `Filter<>`.)
- **Filtering is identical on every dispatch path** — serial `System<>`, `SystemThreaded<>` per-chunk parallel dispatch, `ChunkSystem<>`, and the with-`EntityID` tick variants all honour the same exclude set. Worker-count invariance holds with filters (gated by tests/src/ecs/SystemFiltersWorkerCountInvariance.cpp).
- **`Without<C>` declares no access on `C`.** The excluded component never appears in the system's access set, so a system excluding `C` does *not* conflict with another system writing `C` — the scheduler may run them in the same stage. That is correct (you never touch `C`'s data), but remember it when reasoning about ordering: an exclude is a structural filter, not a read. See [scheduling.md](scheduling.md).
- **Only `Without<C>` exists today.** Any other marker inside `Filter<...>` is a hard compile error (`"ecs::Filter currently supports only ecs::Without<C> entries."`) rather than a silent no-op. The shape is intentionally extensible (a future presence-only `With<C>` or `Optional<C>` could join), but do not invent markers.
- `Filter<...>::exclude_ids()` returns the sorted, deduplicated component ids — you rarely call it yourself, but it is the observable contract the system bases feed into the iteration query (duplicates collapse, order is canonical).

Like ad-hoc query results, a system's filtered iteration automatically picks up brand-new matching archetypes created between ticks — no re-registration needed.

## The query cache

`World` caches resolved query results per `(require_ids, exclude_ids)` key: the list of matching archetype indices. Two user-visible guarantees:

1. **Repetition is cheap.** Running the same query every tick (which is what every system does) resolves to a cached archetype list; the full walk over all archetypes happens only on a miss.
2. **Invalidation is automatic and exact.** Whenever a *new archetype* is created, cached entries are lazily rebuilt on next use, so a query picks up newly created matching archetypes immediately (tested in tests/src/ecs/Iteration.cpp and tests/src/ecs/MatcherHash.cpp). You never flush or manage the cache. Creating entities in an *existing* archetype doesn't invalidate anything — the cached list is archetype-granular, and rows are found live.

Caching changes cost only, never results or determinism.

### The prewarm contract — why ad-hoc queries don't belong inside a tick

During `tick_systems`, the scheduler resolves every scheduled system's iteration query (tick-pack require ids + `Filters` exclude ids) **on the main thread, before each stage's parallel section**. Worker threads then only *read* the cache while dispatching the stage. That contract is load-bearing: a cache miss mutates the cache, and a mutation from a worker thread racing other workers (or another miss) is a data race.

Practical rules that fall out of this:

- **Do not run ad-hoc `world.for_each` from inside a system's tick body.** Only the system's own declared iteration query is guaranteed prewarmed; any other key can miss and mutate the cache from a worker thread. (It also bypasses the scheduler's access model — undeclared reads are invisible to conflict resolution, breaking determinism; see [systems.md](systems.md).) Cross-archetype reads from a tick go through `world.get_component` with the component declared in `extra_reads()`.
- **Ad-hoc queries belong outside `tick_systems`** — session setup, save/load, debug dumps, tests — where the single-threaded rules above apply and cache misses are harmless.

You never interact with the cache directly; there is no public API for it beyond this contract.

## Pitfalls recap

- Forgetting `build()` — silently wrong matching and broken caching. Build once, then reuse.
- Putting a non-tag component in `for_each<Cs...>`'s pack without requiring it in the `Query` — undefined behaviour on archetypes that lack the column.
- Structural mutation (`create_entity` / `destroy_entity` / `add_component` / `remove_component`) inside a `for_each` body — invalidates the columns being iterated. Collect-then-act, or `ctx.cmd` inside systems.
- Expecting `Without<C>` to order your system against writers of `C` — it declares no access; it only filters archetypes.
- Calling `world.for_each` with a novel query from inside a system tick — query-cache race on worker threads plus undeclared access.
- Relying on chunk/row iteration order for id-assignment-sensitive logic — packing is not save-stable.

The consolidated list lives in [pitfalls.md](pitfalls.md).

## Source files

- src/openvic-simulation/ecs/Query.hpp — `Query` builder.
- src/openvic-simulation/ecs/QueryFilter.hpp — `Filter`, `Without`, `system_filters_t`.
- src/openvic-simulation/ecs/World.hpp — `for_each` family, query-cache key types.
- src/openvic-simulation/ecs/ChunkView.hpp — `ChunkView<Cs...>` passed to `for_each_chunk`.
- src/openvic-simulation/ecs/ComponentTypeID.hpp — stable FNV component ids underpinning query determinism.
- Tests: tests/src/ecs/Query.cpp, tests/src/ecs/Iteration.cpp, tests/src/ecs/MatcherHash.cpp, tests/src/ecs/SystemFilters.cpp.
