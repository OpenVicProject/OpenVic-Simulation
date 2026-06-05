#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/Reductions.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	constexpr std::size_t CHUNK_COUNTS[] = { 16, 256, 4096 };
	constexpr uint32_t WORKER_COUNTS[] = { 1, 2, 4, 8, 16 };

	std::string suffix(std::size_t chunks, uint32_t workers) {
		return " chunks=" + std::to_string(chunks) + " workers=" + std::to_string(workers);
	}

	// Per-chunk body: a tight integer fold over a fixed window. Big enough that real work
	// dominates per-chunk dispatch overhead at small chunk counts; cheap enough that the
	// bench still completes quickly.
	constexpr std::size_t PER_CHUNK_ROWS = 256;

	int64_t chunkSum(std::size_t chunk_idx) {
		int64_t acc = 0;
		for (std::size_t i = 0; i < PER_CHUNK_ROWS; ++i) {
			acc += static_cast<int64_t>(chunk_idx * PER_CHUNK_ROWS + i);
		}
		return acc;
	}

}

TEST_CASE("reductions::parallel_sum sweep", "[benchmarks][benchmark-ecs][ecs-reductions]") {
	ankerl::nanobench::Bench bench;
	bench.title("reductions::parallel_sum").unit("chunk");

	for (std::size_t chunks : CHUNK_COUNTS) {
		for (uint32_t wc : WORKER_COUNTS) {
			EcsThreadPool pool { wc };
			bench.batch(chunks).run("parallel_sum" + suffix(chunks, wc), [&] {
				int64_t const result = reductions::parallel_sum<int64_t>(
					pool, chunks, int64_t { 0 },
					[](std::size_t chunk_idx) { return chunkSum(chunk_idx); }
				);
				ankerl::nanobench::doNotOptimizeAway(result);
			});
		}
	}
}

TEST_CASE("reductions::parallel_min sweep", "[benchmarks][benchmark-ecs][ecs-reductions]") {
	ankerl::nanobench::Bench bench;
	bench.title("reductions::parallel_min").unit("chunk");

	for (std::size_t chunks : CHUNK_COUNTS) {
		for (uint32_t wc : WORKER_COUNTS) {
			EcsThreadPool pool { wc };
			bench.batch(chunks).run("parallel_min" + suffix(chunks, wc), [&] {
				int64_t const result = reductions::parallel_min<int64_t>(
					pool, chunks, std::numeric_limits<int64_t>::max(),
					[](std::size_t chunk_idx) { return chunkSum(chunk_idx); }
				);
				ankerl::nanobench::doNotOptimizeAway(result);
			});
		}
	}
}

TEST_CASE("reductions::parallel_max sweep", "[benchmarks][benchmark-ecs][ecs-reductions]") {
	ankerl::nanobench::Bench bench;
	bench.title("reductions::parallel_max").unit("chunk");

	for (std::size_t chunks : CHUNK_COUNTS) {
		for (uint32_t wc : WORKER_COUNTS) {
			EcsThreadPool pool { wc };
			bench.batch(chunks).run("parallel_max" + suffix(chunks, wc), [&] {
				int64_t const result = reductions::parallel_max<int64_t>(
					pool, chunks, std::numeric_limits<int64_t>::min(),
					[](std::size_t chunk_idx) { return chunkSum(chunk_idx); }
				);
				ankerl::nanobench::doNotOptimizeAway(result);
			});
		}
	}
}

// Direct EcsThreadPool::parallel_for — no per-chunk result vector, no fold. Establishes
// the raw dispatch cost the reductions add on top of.
TEST_CASE("EcsThreadPool::parallel_for raw dispatch", "[benchmarks][benchmark-ecs][ecs-reductions]") {
	ankerl::nanobench::Bench bench;
	bench.title("EcsThreadPool::parallel_for").unit("chunk");

	for (std::size_t chunks : CHUNK_COUNTS) {
		for (uint32_t wc : WORKER_COUNTS) {
			EcsThreadPool pool { wc };
			bench.batch(chunks).run("parallel_for" + suffix(chunks, wc), [&] {
				std::atomic<int64_t> sink { 0 };
				pool.parallel_for(chunks, [&](std::size_t chunk_idx, uint32_t /*worker_id*/) {
					sink.fetch_add(chunkSum(chunk_idx), std::memory_order_relaxed);
				});
				ankerl::nanobench::doNotOptimizeAway(sink.load(std::memory_order_relaxed));
			});
		}
	}
}
