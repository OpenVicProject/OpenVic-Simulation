# Threading and reductions

The ECS runs your systems in parallel for you: the scheduler dispatches independent systems concurrently and chunk-parallelizes `SystemThreaded` systems, all through one shared `EcsThreadPool` owned by the `World`. Your job as a game-code author is to write straight-line tick bodies, declare your access honestly (see [systems.md](systems.md)), and — when you need a cross-entity aggregate like "total wages per province" — use the deterministic reduction helpers in `Reductions.hpp` instead of atomics, mutexes, or hand-rolled `parallel_for` calls. Everything here is built so the simulation result is bit-identical at every worker count, which lockstep multiplayer depends on (see [determinism.md](determinism.md)).

## What the scheduler parallelizes for you

You never dispatch threads from game code during a tick. `World::tick_systems` drives all parallelism:

- **Inter-system parallelism.** Systems whose declared access sets do not conflict are grouped into the same *stage* and run concurrently. Conflicting systems are serialized into different stages with a barrier between them. How stages are formed is covered in [scheduling.md](scheduling.md).
- **Intra-system (chunk) parallelism.** A system derived from `SystemThreaded<Derived>` has its `tick` run per row, with chunks of the matched archetypes distributed across pool workers. Plain `System<Derived>` ticks run as one serial unit of work.
- **Mixed stages just work.** A multi-system stage may freely mix `System<>` and `SystemThreaded` systems. The scheduler builds one flat work-item list — one item per chunk for each `SystemThreaded`, one whole-tick item per plain `System<>` — and dispatches it via a single outer `parallel_for`. It also prewarms the `World` query cache on the main thread before the parallel section, so workers only ever read it.

What you are guaranteed, provided your access declarations are correct:

- Every stage fully completes (including its `CommandBuffer` apply phase) before the next stage starts.
- A `SystemThreaded` system's deferred mutations go into per-chunk `CommandBuffer`s that are merged in `chunk_idx` ascending order — the apply order is deterministic and identical across all worker counts (see [command-buffer.md](command-buffer.md)).
- The simulation result does not depend on the worker count. `tests/src/ecs/WorkerCountInvariance.cpp` gates this with 10-tick digests (plus 1- and 5-tick deferred-create digests) at workers = 1, 2, 4, 8, 16.

## Configuring the worker count

```cpp
// World.hpp
void set_ecs_worker_count(uint32_t count);
EcsThreadPool& ecs_thread_pool();
void set_serial_mode(bool enabled);
```

- `set_ecs_worker_count(count)` — overrides the pool size. Call it **before the first `tick_systems` invocation**. `0` means "use the default": `std::thread::hardware_concurrency()`, capped at 16. Calling it after the pool exists resets the pool so it is rebuilt with the new count on next access.
- `ecs_thread_pool()` — returns the `EcsThreadPool` the scheduler uses, lazily constructed with the configured worker count on first access. This is how you feed a pool to the `reductions::*` helpers from outside a tick.
- `set_serial_mode(true)` — disables stage-level (inter-system) parallelism: each stage's systems run one at a time, dispatched from the calling thread; a `SystemThreaded` still parallelises over its own chunks unless the pool has one worker (see [scheduling.md](scheduling.md)). A test/debug knob for validating "parallel result == serial result"; not for production use.

The worker count is **not** part of the deterministic state — two multiplayer peers may run with different worker counts and must still produce identical simulations. That is exactly why the rules below exist.

## Intra-system parallelism: `SystemThreaded`

The one sanctioned way to get parallelism *inside* a system is to derive from `SystemThreaded<Derived>` instead of `System<Derived>`. Your `tick` is unchanged — it still processes one row — but the scheduler distributes chunks across workers:

```cpp
struct ParPos {
	int64_t x = 0;
};
struct ParVel {
	int64_t dx = 0;
};
ECS_COMPONENT(ParPos, "game::Pos")
ECS_COMPONENT(ParVel, "game::Vel")

struct ParMover : SystemThreaded<ParMover> {
	void tick(TickContext const& /*ctx*/, ParPos& p, ParVel const& v) {
		p.x += v.dx;
	}
};
ECS_SYSTEM(ParMover)

// Registration and ticking — identical to a plain System<>:
World world;
world.set_ecs_worker_count(8); // before the first tick_systems
// ... create entities with ParPos + ParVel ...
world.register_system<ParMover>();
world.tick_systems(Date {});
```

(Adapted from `tests/src/ecs/IntraSystemParallel.cpp`, which also verifies that every row is touched exactly once and that the 5-tick digest is identical at worker counts 1 through 16.)

Rules specific to `SystemThreaded`:

- **`extra_writes()` is forbidden** — enforced by a `static_assert` in `World::register_system`. A chunk-parallel system writing a shared cross-archetype target (or a singleton) would race with *itself* across its own workers. If you need to write something outside the rows you iterate, the system must be a plain `System<>`.
- **Touch only your own row** (plus declared `extra_reads()` data). Two workers processing different rows of the same archetype concurrently is the whole point; writing a neighbor's row is a data race.
- **Structural changes go through `ctx.cmd`** as always — for a `SystemThreaded`, `ctx.cmd` is the per-chunk `CommandBuffer` for the row's chunk, and the per-chunk buffers are merged in `chunk_idx` ascending order, so command apply order is worker-count-invariant.
- An optional `static bool should_run(TickContext const&)` cadence gate works the same as on `System<>` — evaluated once per tick on the main thread; a skipped system contributes no work items at all (see [systems.md](systems.md)).

## Never call `parallel_for` from inside a system tick

This is the central rule of this page, straight from the project pitfall list:

> **Don't call `EcsThreadPool::parallel_for` from inside a system tick body.**

The scheduler *already* wraps your stage in the outer `parallel_for`. The deadlock mechanics, briefly:

1. In a multi-system stage, your tick body executes **on a pool worker thread** (even a plain `System<>`'s whole tick runs as one work item on a worker).
2. `parallel_for` is blocking: the caller pushes the inner jobs onto the shared queue and then waits on a condition variable until all of them complete. While waiting, **it does not drain the queue itself**.
3. The inner jobs therefore sit unowned until *some other* worker picks them up. If every worker is simultaneously blocked the same way — entirely possible when several systems in a stage each try this — no worker is left to drain the queue. Deadlock.

Two details that make this rule easy to violate without noticing:

- **It works on a single-worker pool.** `parallel_for` has a fast path: with `workers_.size() <= 1` (or `chunk_count == 1`) the body runs inline on the calling thread. Your nested call will appear fine in a serial test run and deadlock under load.
- **It can work while a plain `System<>` happens to be alone in its stage.** Single-system stages invoke `tick_all` on the main thread, so a plain `System<>`'s whole tick runs there. A `SystemThreaded`'s tick bodies, however, always execute inside the pool's `parallel_for` — its `tick_all` dispatches chunks to workers even in a solo stage — so nesting there deadlocks no matter the stage layout. And for `System<>`, stage layout is computed by the scheduler from access sets — registering one more system later can co-stage yours onto a worker. Do not depend on stage layout for safety.

(The pool's per-call `DoneState` accounting *is* nesting-safe — an inner dispatch cannot corrupt an outer dispatch's completion counter — but correct bookkeeping does not un-block a waiting worker. The thread is still parked.)

**What to do instead:**

- Need per-row parallelism? Make the system a `SystemThreaded` and let the scheduler dispatch it.
- Need a parallel aggregate? Run a `reductions::*` fold **from the main thread, outside `tick_systems`** — before the tick, after it, or between ticks — feeding it `world.ecs_thread_pool()`. Inside a tick body, write straight-line code.

## `EcsThreadPool` reference

Defined in `src/openvic-simulation/ecs/EcsThreadPool.hpp`. This is the dedicated pool for ECS scheduler dispatch — intentionally separate from `OpenVic::ThreadPool` (which serves un-migrated production-tick / market-clearing code) so neither side disturbs the other. From game code you normally only ever *pass it along* (to `reductions::*`); you rarely construct one outside tests.

```cpp
explicit EcsThreadPool(uint32_t worker_count);
```

Constructs a pool with a fixed worker count. `worker_count == 0` is treated as `1` — a degenerate single-thread pool, useful for tests and headless determinism runs. The pool is non-copyable and non-movable; its destructor joins all workers.

```cpp
uint32_t worker_count() const noexcept;
```

The actual number of workers (never 0).

```cpp
template<typename Body>
void parallel_for(std::size_t chunk_count, Body&& body);
```

Runs `body(chunk_idx, worker_id)` for every `chunk_idx` in `[0, chunk_count)`. Contracts:

- **Blocking** — does not return until every chunk's body has run. The body may live on your stack; the pool borrows it for the duration of the call.
- Each `chunk_idx` is visited **exactly once**; no work is silently dropped.
- The internal scheduling strategy is opaque and deliberately unspecified — never assume an execution order across chunk indices.
- Bodies that throw will `std::terminate`. Tick bodies and reduction bodies should be effectively `noexcept`.
- `worker_id` (stable in `0..worker_count-1`) is exposed **for diagnostics or thread-local scratch only**. Never key determinism-relevant data by `worker_id` — which worker runs which chunk is schedule-dependent. Key by `chunk_idx` instead; that is exactly what the `reductions::*` helpers do for you.
- **Never call it from inside a system tick body** (see above). Safe call sites are threads that are not pool workers — in practice, the main thread outside `tick_systems`.

```cpp
void run_concurrent(std::span<std::function<void()> const> bodies);
```

Runs each supplied function exactly once across the pool; blocking. A general-purpose dispatch the scheduler no longer uses (multi-system stages go through the single flat-work-item `parallel_for` instead) — game code has no normal reason to call it, and the same "not from inside a tick" rule applies.

## Reductions

Header: `src/openvic-simulation/ecs/Reductions.hpp`, namespace `OpenVic::ecs::reductions`.

The pattern all four helpers share: each chunk's body writes its partial result into a **`chunk_idx`-indexed slot** — no atomics, no shared mutable state during the parallel section — then, after the parallel section joins, the per-chunk results are folded **sequentially in `chunk_idx` ascending order**. The only operation order that affects the output is that sequential fold, which is independent of the pool — so the result is bit-identical regardless of `worker_count`. This is the project-sanctioned replacement for "loop over everything with an atomic accumulator", which is both contended and non-deterministic for non-associative arithmetic.

`chunk_count` here is just "number of independent slices of work" — it does not have to equal an archetype's chunk count, though slicing along storage chunks is the natural fit (see [storage-model.md](storage-model.md)).

### `parallel_sum` / `parallel_min` / `parallel_max`

```cpp
template<typename T, typename Body>
T parallel_sum(EcsThreadPool& pool, std::size_t chunk_count, T init, Body&& body);

template<typename T, typename Body>
T parallel_min(EcsThreadPool& pool, std::size_t chunk_count, T init, Body&& body);

template<typename T, typename Body>
T parallel_max(EcsThreadPool& pool, std::size_t chunk_count, T init, Body&& body);
```

`body(chunk_idx) -> T` is computed once per chunk in parallel; the results are then folded serially:

- `parallel_sum` — starts from `init` and adds the per-chunk values in `chunk_idx` ascending order (`acc = acc + per_chunk[i]`). For integer / `fixed_point_t` types, where addition is associative, the result is bit-identical across worker counts. (Floats are banned in the simulation anyway — see [determinism.md](determinism.md).)
- `parallel_min` / `parallel_max` — running min/max from `init` (use a sentinel like `INT64_MAX` / `INT64_MIN`). Ties preserve the **leftmost** (lowest `chunk_idx`) value, so the result depends only on the per-chunk values, never the worker schedule.
- `chunk_count == 0` returns `init` unchanged; the body is never invoked.

Adapted from `tests/src/ecs/Reductions.cpp`:

```cpp
#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/Reductions.hpp"

using namespace OpenVic::ecs;

// Outside tick_systems, on the main thread:
EcsThreadPool& pool = world.ecs_thread_pool();

std::size_t const chunk_count = per_chunk_totals.size();
int64_t total = reductions::parallel_sum<int64_t>(pool, chunk_count, int64_t { 0 },
	[&per_chunk_totals](std::size_t chunk_idx) {
		return per_chunk_totals[chunk_idx];
	});

int64_t worst = reductions::parallel_min<int64_t>(pool, chunk_count, INT64_MAX,
	[&per_chunk_totals](std::size_t chunk_idx) {
		return per_chunk_totals[chunk_idx];
	});
```

### `parallel_keyed_sum` — per-key totals

For aggregates bucketed by a dense key (e.g. "sum pop wages per province", where provinces are keys `0..key_count-1`):

```cpp
template<typename T, typename Body>
void parallel_keyed_sum(
	EcsThreadPool& pool, std::size_t chunk_count, std::size_t key_count,
	std::span<T> out, KeyedSumScratch<T>& scratch, Body&& body
);

// Convenience overload: local scratch, allocates per call.
template<typename T, typename Body>
void parallel_keyed_sum(
	EcsThreadPool& pool, std::size_t chunk_count, std::size_t key_count,
	std::span<T> out, Body&& body
);
```

`body(chunk_idx, emit)` runs in parallel and emits `(key, value)` pairs for its chunk via `emit.add(key, value)` into a per-chunk **sparse** buffer (`KeyedEmitter<T>`). After the join, the buffers are folded into the **dense** `out` array sequentially — `chunk_idx` ascending, and within a chunk in emission order. Same discipline as `parallel_sum`: no atomics, no shared mutable state during the parallel section, so the result is bit-identical across worker counts.

Contracts:

- `out.size() >= key_count` (asserted), and every emitted `key < key_count` (asserted at fold time).
- `out` is **caller-initialized** (typically zeros). Contributions are `+=` onto it, so keys no chunk emits keep their initial value.
- `T` needs only `+=` — `fixed_point_t` works, as do plain integers.
- `chunk_count == 0` leaves `out` untouched; the body is never invoked.

Supporting types:

```cpp
template<typename T>
struct KeyedEmitter {
	std::vector<std::pair<std::size_t, T>> entries;

	void add(std::size_t key, T value);
};

template<typename T>
struct KeyedSumScratch {
	std::vector<KeyedEmitter<T>> per_chunk;
};
```

- Each `KeyedEmitter` is exclusively owned by one work item during the parallel section — never share one across chunks. `add()` folds duplicate keys in place: the first occurrence of a key fixes its position in the chunk's emission order, later occurrences accumulate onto it via `+=`. There is a fast path when consecutive emissions hit the same key (chunks are typically grouped by key — e.g. pops by province), and a linear scan otherwise — so keep emission patterns key-clustered where you can.
- `KeyedSumScratch` is reusable scratch: hold one across ticks to reuse the per-chunk buffer capacity instead of reallocating every call. Stale entries from a previous (larger) call are cleared for you — only `chunk_count` slots are cleared and folded each call.

Adapted from `tests/src/ecs/KeyedReductions.cpp` (the `fixed_point_t` worker-count-invariance case):

```cpp
#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/Reductions.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

using namespace OpenVic::ecs;
using OpenVic::fixed_point_t;

// Held across ticks so per-chunk buffer capacity is reused:
reductions::KeyedSumScratch<fixed_point_t> wage_scratch;

// Once per tick, outside tick_systems, on the main thread:
std::size_t const chunk_count = pop_chunks.size(); // one slice per storage chunk
std::size_t const key_count = province_count;

std::vector<fixed_point_t> wages_per_province(key_count, fixed_point_t::_0);
reductions::parallel_keyed_sum<fixed_point_t>(
	pool, chunk_count, key_count, wages_per_province, wage_scratch,
	[&pop_chunks](std::size_t chunk_idx, reductions::KeyedEmitter<fixed_point_t>& emit) {
		PopChunkView const& chunk = pop_chunks[chunk_idx];
		for (std::size_t row = 0; row < chunk.row_count; ++row) {
			emit.add(chunk.province_key[row], chunk.wage[row]);
		}
	}
);
// wages_per_province[k] is now bit-identical at every worker count.
// Provinces with no pops keep their initial value (fixed_point_t::_0).
```

## Determinism checklist for parallel code

The threading-relevant rules, with rationale (full treatment in [determinism.md](determinism.md), consolidated list in [pitfalls.md](pitfalls.md)):

- **No worker-id-keyed accumulation.** Which worker runs which chunk is schedule noise. Key everything by `chunk_idx` — or just use `reductions::*`, which do.
- **No atomics/mutex accumulators in tick bodies.** Even when race-free, the combine order varies run to run. The reductions' sequential fold is the deterministic substitute.
- **No `parallel_for` / `run_concurrent` / `reductions::*` from inside a tick body** — deadlock risk as described above. Main thread, outside `tick_systems`, only.
- **Declare every access**, including singletons via `extra_reads()` / `extra_writes()` — an undeclared access is invisible to the scheduler's conflict model, so a writer and reader can be co-staged and race; the worker-count-invariance gate catches that only probabilistically.
- **No float math anywhere in the simulation** — `fixed_point_t` everywhere, including reduction values.

## Source files

- `src/openvic-simulation/ecs/EcsThreadPool.hpp` / `src/openvic-simulation/ecs/EcsThreadPool.cpp` — the pool: `parallel_for`, `run_concurrent`, invariants.
- `src/openvic-simulation/ecs/Reductions.hpp` — `parallel_sum`, `parallel_min`, `parallel_max`, `parallel_keyed_sum`, `KeyedEmitter`, `KeyedSumScratch`.
- `src/openvic-simulation/ecs/World.hpp` — `set_ecs_worker_count`, `ecs_thread_pool`, `set_serial_mode`.
- `src/openvic-simulation/ecs/System.hpp` — `SystemThreaded`, the `extra_writes` restriction.
- Tests: `tests/src/ecs/EcsThreadPool.cpp`, `tests/src/ecs/Reductions.cpp`, `tests/src/ecs/KeyedReductions.cpp`, `tests/src/ecs/IntraSystemParallel.cpp`, `tests/src/ecs/WorkerCountInvariance.cpp`.
