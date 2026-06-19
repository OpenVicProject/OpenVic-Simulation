#include "openvic-simulation/ecs/EcsThreadPool.hpp"

#include <algorithm>
#include <utility>

using namespace OpenVic::ecs;

EcsThreadPool::EcsThreadPool(uint32_t worker_count) {
	uint32_t const n = std::max<uint32_t>(1u, worker_count);
	workers_.reserve(n);
	for (uint32_t i = 0; i < n; ++i) {
		workers_.emplace_back([this, i]() { worker_loop(i); });
	}
}

EcsThreadPool::~EcsThreadPool() {
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		stop_ = true;
	}
	cv_.notify_all();
	for (std::thread& t : workers_) {
		if (t.joinable()) {
			t.join();
		}
	}
}

void EcsThreadPool::worker_loop(uint32_t worker_id) {
	for (;;) {
		Job job;
		bool have_job = false;
		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			cv_.wait(lock, [this]() { return stop_ || !queue_.empty(); });
			if (stop_ && queue_.empty()) {
				return;
			}
			if (!queue_.empty()) {
				job = std::move(queue_.back());
				queue_.pop_back();
				have_job = true;
			}
		}
		if (!have_job) {
			continue;
		}

		if (job.parallel_body != nullptr) {
			(*job.parallel_body)(job.chunk_idx, worker_id);
		} else if (job.concurrent_body) {
			job.concurrent_body();
		}

		// Decrement the dispatch's own DoneState under its mutex so the caller's
		// predicate (evaluated under the same mutex inside cv.wait) cannot observe
		// count == 0 until the decrement-and-notify sequence has completed — that
		// is what allows the caller's DoneState to live on the caller's stack
		// without lifetime races.
		DoneState* const done = job.done;
		std::lock_guard<std::mutex> lock(done->mutex);
		--done->count;
		if (done->count == 0) {
			done->cv.notify_all();
		}
	}
}

void EcsThreadPool::run_parallel_for_impl(std::size_t chunk_count, ParallelForBody body) {
	// Per-call DoneState — separate counter+CV for each dispatch lets a System
	// dispatched via run_concurrent itself call parallel_for without trampling the
	// outer dispatch's accounting. Lives on this stack frame; workers hold its
	// mutex while decrementing so we cannot return (and destroy it) until the
	// last worker has fully released the mutex.
	DoneState done;
	done.count = chunk_count;

	// Push every chunk index as a separate Job into the queue. The `body` lives on the
	// caller's stack for the duration of this call; jobs hold a non-owning pointer to it.
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		queue_.reserve(queue_.size() + chunk_count);
		for (std::size_t i = 0; i < chunk_count; ++i) {
			Job j;
			j.parallel_body = &body;
			j.chunk_idx = i;
			j.done = &done;
			queue_.push_back(std::move(j));
		}
	}
	cv_.notify_all();

	// Wait until every job has decremented its way down to zero. Predicate is
	// evaluated under done.mutex (cv.wait acquires it), serialising with the
	// worker_loop decrement-under-lock above.
	{
		std::unique_lock<std::mutex> lock(done.mutex);
		done.cv.wait(lock, [&done]() {
			return done.count == 0;
		});
	}
}

void EcsThreadPool::run_concurrent(std::span<std::function<void()> const> bodies) {
	if (bodies.empty()) {
		return;
	}
	if (workers_.size() <= 1 || bodies.size() == 1) {
		for (auto const& fn : bodies) {
			fn();
		}
		return;
	}
	DoneState done;
	done.count = bodies.size();

	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		queue_.reserve(queue_.size() + bodies.size());
		for (auto const& fn : bodies) {
			Job j;
			j.concurrent_body = fn; // copy
			j.done = &done;
			queue_.push_back(std::move(j));
		}
	}
	cv_.notify_all();
	{
		std::unique_lock<std::mutex> lock(done.mutex);
		done.cv.wait(lock, [&done]() {
			return done.count == 0;
		});
	}
}
