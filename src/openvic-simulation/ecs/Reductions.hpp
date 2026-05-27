#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "openvic-simulation/ecs/EcsThreadPool.hpp"

namespace OpenVic::ecs::reductions {
	// Deterministic parallel reductions: per-chunk worker bodies write into a
	// chunk_idx-indexed std::vector<T>; after the parallel section joins, we fold the
	// per-chunk results sequentially in chunk_idx ascending order. Final result is
	// bit-identical regardless of worker_count — the only operation order that affects
	// the output is the sequential fold at the end, which is independent of the pool.

	// Compute body(chunk_idx) -> T per chunk, then sum the results sequentially.
	// `init` is the sum's starting value; the per-chunk T values are added to it in
	// chunk_idx ascending order. For integer/fixed_point types where addition is
	// associative, the result is bit-identical across worker counts.
	template<typename T, typename Body>
	T parallel_sum(EcsThreadPool& pool, std::size_t chunk_count, T init, Body&& body) {
		std::vector<T> per_chunk(chunk_count);
		pool.parallel_for(chunk_count, [&](std::size_t chunk_idx, uint32_t /*worker_id*/) {
			per_chunk[chunk_idx] = body(chunk_idx);
		});
		T acc = init;
		for (std::size_t i = 0; i < chunk_count; ++i) {
			acc = acc + per_chunk[i];
		}
		return acc;
	}

	// Compute body(chunk_idx) -> T per chunk, then take the running min. The fold runs
	// sequentially in chunk_idx ascending order; min/max-of-equal preserves the leftmost
	// value, so the result depends only on the chunk count and per-chunk values, not on
	// the worker schedule.
	template<typename T, typename Body>
	T parallel_min(EcsThreadPool& pool, std::size_t chunk_count, T init, Body&& body) {
		std::vector<T> per_chunk(chunk_count);
		pool.parallel_for(chunk_count, [&](std::size_t chunk_idx, uint32_t /*worker_id*/) {
			per_chunk[chunk_idx] = body(chunk_idx);
		});
		T acc = init;
		for (std::size_t i = 0; i < chunk_count; ++i) {
			if (per_chunk[i] < acc) {
				acc = per_chunk[i];
			}
		}
		return acc;
	}

	template<typename T, typename Body>
	T parallel_max(EcsThreadPool& pool, std::size_t chunk_count, T init, Body&& body) {
		std::vector<T> per_chunk(chunk_count);
		pool.parallel_for(chunk_count, [&](std::size_t chunk_idx, uint32_t /*worker_id*/) {
			per_chunk[chunk_idx] = body(chunk_idx);
		});
		T acc = init;
		for (std::size_t i = 0; i < chunk_count; ++i) {
			if (per_chunk[i] > acc) {
				acc = per_chunk[i];
			}
		}
		return acc;
	}
}
