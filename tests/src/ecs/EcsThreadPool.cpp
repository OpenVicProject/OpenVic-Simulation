#include "openvic-simulation/ecs/EcsThreadPool.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

TEST_CASE("EcsThreadPool::parallel_for invokes body N times", "[ecs][EcsThreadPool]") {
	for (uint32_t worker_count : { 1u, 2u, 4u, 8u }) {
		EcsThreadPool pool { worker_count };
		std::atomic<int> counter { 0 };
		std::size_t const N = 1000;
		pool.parallel_for(N, [&counter](std::size_t /*chunk_idx*/, uint32_t /*worker_id*/) {
			counter.fetch_add(1, std::memory_order_relaxed);
		});
		CHECK(counter.load() == static_cast<int>(N));
	}
}

TEST_CASE("EcsThreadPool::parallel_for visits each chunk_idx exactly once", "[ecs][EcsThreadPool]") {
	EcsThreadPool pool { 4 };
	std::size_t const N = 256;
	std::vector<std::atomic<int>> seen(N);
	pool.parallel_for(N, [&seen](std::size_t chunk_idx, uint32_t /*worker_id*/) {
		seen[chunk_idx].fetch_add(1, std::memory_order_relaxed);
	});
	for (std::size_t i = 0; i < N; ++i) {
		CHECK(seen[i].load() == 1);
	}
}

TEST_CASE("EcsThreadPool::parallel_for produces same result regardless of worker count",
          "[ecs][EcsThreadPool][determinism]") {
	std::vector<int> baseline(500, 0);
	{
		EcsThreadPool serial { 1 };
		serial.parallel_for(500, [&baseline](std::size_t chunk_idx, uint32_t /*worker_id*/) {
			baseline[chunk_idx] = static_cast<int>(chunk_idx * 7 + 3);
		});
	}

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		std::vector<int> results(500, 0);
		EcsThreadPool pool { wc };
		pool.parallel_for(500, [&results](std::size_t chunk_idx, uint32_t /*worker_id*/) {
			results[chunk_idx] = static_cast<int>(chunk_idx * 7 + 3);
		});
		CHECK(results == baseline);
	}
}

TEST_CASE("EcsThreadPool::run_concurrent runs each function exactly once", "[ecs][EcsThreadPool]") {
	EcsThreadPool pool { 4 };
	std::atomic<int> a { 0 };
	std::atomic<int> b { 0 };
	std::atomic<int> c { 0 };
	std::vector<std::function<void()>> bodies;
	bodies.emplace_back([&a]() { a.fetch_add(1); });
	bodies.emplace_back([&b]() { b.fetch_add(1); });
	bodies.emplace_back([&c]() { c.fetch_add(1); });
	pool.run_concurrent(std::span<std::function<void()> const>(bodies.data(), bodies.size()));
	CHECK(a.load() == 1);
	CHECK(b.load() == 1);
	CHECK(c.load() == 1);
}

TEST_CASE("EcsThreadPool::parallel_for blocks until done", "[ecs][EcsThreadPool]") {
	EcsThreadPool pool { 4 };
	std::atomic<int> counter { 0 };
	pool.parallel_for(100, [&counter](std::size_t /*chunk_idx*/, uint32_t /*worker_id*/) {
		// Brief artificial work to ensure we'd visibly fail if the join was missing.
		for (int i = 0; i < 1000; ++i) {
			counter.fetch_add(1, std::memory_order_relaxed);
		}
	});
	CHECK(counter.load() == 100 * 1000);
}
