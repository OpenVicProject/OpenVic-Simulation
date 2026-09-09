#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

namespace OpenVic::ecs {
	// Dedicated thread pool for ECS scheduler dispatch. Intentionally separate from
	// `OpenVic::ThreadPool` (which serves un-migrated production-tick / market-clearing
	// code with a different work-bundle shape) so neither side disturbs the other.
	//
	// Workers are numbered 0..worker_count-1 with stable identities; each worker passes
	// its `worker_id` to the body it executes. ECS callers do not depend on worker_id
	// for determinism — per-chunk CommandBuffers are keyed by chunk_idx, not worker_id —
	// but it is exposed for diagnostic or thread-local-scratch uses.
	//
	// Hard invariants:
	//   * `parallel_for` is blocking — does not return until every chunk's body has run.
	//   * `run_concurrent` is blocking — does not return until every supplied function
	//     has completed.
	//   * No work is ever silently dropped; bodies that throw will std::terminate (we do
	//     not guarantee exception-safety from inside system bodies — they should be noexcept).
	class EcsThreadPool {
	public:
		// Construct with a fixed worker count. worker_count == 0 is treated as 1 (a
		// degenerate single-thread pool, useful for tests and headless determinism runs).
		explicit EcsThreadPool(uint32_t worker_count);
		~EcsThreadPool();

		EcsThreadPool(EcsThreadPool const&) = delete;
		EcsThreadPool& operator=(EcsThreadPool const&) = delete;
		EcsThreadPool(EcsThreadPool&&) = delete;
		EcsThreadPool& operator=(EcsThreadPool&&) = delete;

		uint32_t worker_count() const noexcept {
			return static_cast<uint32_t>(workers_.size());
		}

		// Run body(chunk_idx, worker_id) for every chunk_idx in [0, chunk_count). Blocking.
		// The internal scheduling strategy (work-queue, modulo, stealing) is opaque and
		// deliberately not exposed — the only externally observable property is "every
		// chunk's body runs exactly once and parallel_for does not return early".
		template<typename Body>
		void parallel_for(std::size_t chunk_count, Body&& body) {
			if (chunk_count == 0) {
				return;
			}
			if (workers_.size() <= 1 || chunk_count == 1) {
				// Fast path: single-thread fall-through. Same observable behaviour as the
				// parallel path; saves the queue/cv overhead in degenerate cases.
				for (std::size_t i = 0; i < chunk_count; ++i) {
					body(i, /*worker_id=*/0u);
				}
				return;
			}
			run_parallel_for_impl(chunk_count, [&body](std::size_t chunk_idx, uint32_t worker_id) {
				body(chunk_idx, worker_id);
			});
		}

		// Run each supplied function exactly once across the pool — used for inter-system
		// parallelism within a scheduler stage. Blocking.
		void run_concurrent(std::span<std::function<void()> const> bodies);

	private:
		// Type-erased body for parallel_for. Templated wrapper above forwards to this.
		using ParallelForBody = std::function<void(std::size_t /*chunk_idx*/, uint32_t /*worker_id*/)>;

		// Per-call completion tracker. Stored on the caller's stack inside parallel_for /
		// run_concurrent and pointed-to by every Job that dispatch submits. Workers
		// decrement the *job's* DoneState — not a shared pool counter — so a System
		// dispatched via run_concurrent can itself call parallel_for without the inner
		// dispatch corrupting the outer dispatch's accounting (pre-fix, both used a
		// single pool-wide `remaining_` atomic, which caused spurious wakeups and
		// SIZE_MAX underflow when a `SystemThreaded` shared a stage with another System).
		// Lifetime is enforced by the worker taking `done->mutex` while decrementing
		// `done->count` — the caller's `done.cv.wait` predicate is also evaluated under
		// the same mutex, so the caller can't return (and destroy the DoneState) until
		// the decrement-and-notify sequence has completed.
		struct DoneState {
			std::mutex mutex;
			std::condition_variable cv;
			std::size_t count = 0; // Always touched while holding `mutex`.
		};

		void run_parallel_for_impl(std::size_t chunk_count, ParallelForBody body);

		void worker_loop(uint32_t worker_id);

		std::vector<std::thread> workers_;

		// Work item: either a parallel_for slice or a run_concurrent function.
		struct Job {
			// For parallel_for: pointer-back to the shared body + the chunk_idx assigned
			// to this job. For run_concurrent: a one-shot function to invoke; chunk_idx is 0.
			ParallelForBody const* parallel_body = nullptr; // borrowed; lives on caller stack
			std::function<void()> concurrent_body; // owned
			std::size_t chunk_idx = 0;
			DoneState* done = nullptr; // borrowed; lives on caller stack until count hits 0
		};

		std::mutex queue_mutex_;
		std::condition_variable cv_;
		std::vector<Job> queue_; // FIFO; back-popped under queue_mutex_
		bool stop_ = false;
	};
}
