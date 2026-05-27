#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <utility>
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

	// Sparse per-chunk (key, value) buffer for parallel_keyed_sum. Each instance is
	// exclusively owned by one work item during the parallel section. add() folds
	// duplicate keys in place: the first occurrence of a key fixes its position in
	// emission order, later occurrences accumulate onto it via +=.
	template<typename T>
	struct KeyedEmitter {
		std::vector<std::pair<std::size_t, T>> entries;

		void add(std::size_t key, T value) {
			// Chunks are typically grouped by key (e.g. pops by province), so consecutive
			// emissions usually hit the newest entry.
			if (!entries.empty() && entries.back().first == key) {
				entries.back().second += value;
				return;
			}
			for (std::pair<std::size_t, T>& entry : entries) {
				if (entry.first == key) {
					entry.second += value;
					return;
				}
			}
			entries.emplace_back(key, value);
		}
	};

	// Reusable scratch for parallel_keyed_sum. Hold one across ticks to reuse the
	// per-chunk buffer capacity instead of reallocating every call.
	template<typename T>
	struct KeyedSumScratch {
		std::vector<KeyedEmitter<T>> per_chunk;
	};

	// Deterministic keyed parallel fold: body(chunk_idx, emit) runs in parallel and
	// emits (key, value) pairs for its chunk via emit.add(key, value) into a per-chunk
	// sparse buffer; after the join, the buffers are folded into `out` sequentially in
	// chunk_idx ascending order (and emission order within a chunk). Same discipline as
	// parallel_sum — no atomics, no shared mutable state during the parallel section —
	// so the result is bit-identical across worker counts.
	// `out` is caller-initialized (typically zeros); contributions are += onto it, so
	// keys no chunk emits keep their initial value. T needs only += (fixed_point_t works).
	template<typename T, typename Body>
	void parallel_keyed_sum(
		EcsThreadPool& pool, std::size_t chunk_count, std::size_t key_count,
		std::span<T> out, KeyedSumScratch<T>& scratch, Body&& body
	) {
		assert(out.size() >= key_count);
		if (scratch.per_chunk.size() < chunk_count) {
			scratch.per_chunk.resize(chunk_count);
		}
		for (std::size_t i = 0; i < chunk_count; ++i) {
			scratch.per_chunk[i].entries.clear();
		}
		pool.parallel_for(chunk_count, [&](std::size_t chunk_idx, uint32_t /*worker_id*/) {
			body(chunk_idx, scratch.per_chunk[chunk_idx]);
		});
		for (std::size_t i = 0; i < chunk_count; ++i) {
			for (std::pair<std::size_t, T> const& entry : scratch.per_chunk[i].entries) {
				assert(entry.first < key_count);
				out[entry.first] += entry.second;
			}
		}
	}

	// Convenience overload: local scratch, allocates per call.
	template<typename T, typename Body>
	void parallel_keyed_sum(
		EcsThreadPool& pool, std::size_t chunk_count, std::size_t key_count,
		std::span<T> out, Body&& body
	) {
		KeyedSumScratch<T> scratch;
		parallel_keyed_sum(pool, chunk_count, key_count, out, scratch, std::forward<Body>(body));
	}
}
