#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/Reductions.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include <cstdint>
#include <span>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::fixed_point_t;

namespace {
	// Deterministic synthetic emission pattern: chunk i emits ROWS_PER_CHUNK rows, row r
	// contributing value (i * 1009 + r * 31 + 7) to key ((i * 7 + r / 4) % key_count).
	// Consecutive rows share a key (runs of 4 → exercises the newest-entry fast path) and
	// the key sequence wraps (→ exercises the linear-scan duplicate path).
	std::size_t const ROWS_PER_CHUNK = 16;

	std::size_t row_key(std::size_t chunk_idx, std::size_t row, std::size_t key_count) {
		return (chunk_idx * 7 + row / 4) % key_count;
	}

	int64_t row_value(std::size_t chunk_idx, std::size_t row) {
		return static_cast<int64_t>(chunk_idx * 1009 + row * 31 + 7);
	}

	void emit_chunk(std::size_t chunk_idx, std::size_t key_count, reductions::KeyedEmitter<int64_t>& emit) {
		for (std::size_t row = 0; row < ROWS_PER_CHUNK; ++row) {
			emit.add(row_key(chunk_idx, row, key_count), row_value(chunk_idx, row));
		}
	}

	std::vector<int64_t> serial_reference(std::size_t chunk_count, std::size_t key_count) {
		std::vector<int64_t> expected(key_count, 0);
		for (std::size_t i = 0; i < chunk_count; ++i) {
			for (std::size_t row = 0; row < ROWS_PER_CHUNK; ++row) {
				expected[row_key(i, row, key_count)] += row_value(i, row);
			}
		}
		return expected;
	}

	int64_t digest_out(std::span<int64_t const> out) {
		int64_t digest = 0;
		for (int64_t v : out) {
			digest = digest * 1000003 + v;
		}
		return digest;
	}
}

TEST_CASE("parallel_keyed_sum matches a serial reference", "[ecs][Reductions][keyed]") {
	std::size_t const chunk_count = 64;
	std::size_t const key_count = 37;
	EcsThreadPool pool { 4 };

	std::vector<int64_t> out(key_count, 0);
	reductions::parallel_keyed_sum<int64_t>(pool, chunk_count, key_count, out,
		[key_count](std::size_t chunk_idx, reductions::KeyedEmitter<int64_t>& emit) {
			emit_chunk(chunk_idx, key_count, emit);
		});

	std::vector<int64_t> expected = serial_reference(chunk_count, key_count);
	for (std::size_t k = 0; k < key_count; ++k) {
		CHECK(out[k] == expected[k]);
	}
}

TEST_CASE("parallel_keyed_sum is bit-identical across worker counts", "[ecs][Reductions][keyed][determinism]") {
	std::size_t const chunk_count = 200;
	std::size_t const key_count = 53;

	int64_t baseline = 0;
	{
		EcsThreadPool serial { 1 };
		std::vector<int64_t> out(key_count, 0);
		reductions::parallel_keyed_sum<int64_t>(serial, chunk_count, key_count, out,
			[key_count](std::size_t chunk_idx, reductions::KeyedEmitter<int64_t>& emit) {
				emit_chunk(chunk_idx, key_count, emit);
			});
		baseline = digest_out(out);
	}

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		EcsThreadPool pool { wc };
		std::vector<int64_t> out(key_count, 0);
		reductions::parallel_keyed_sum<int64_t>(pool, chunk_count, key_count, out,
			[key_count](std::size_t chunk_idx, reductions::KeyedEmitter<int64_t>& emit) {
				emit_chunk(chunk_idx, key_count, emit);
			});
		CHECK(digest_out(out) == baseline);
	}
}

TEST_CASE("parallel_keyed_sum folds duplicate keys within a chunk", "[ecs][Reductions][keyed]") {
	std::size_t const key_count = 8;
	EcsThreadPool pool { 4 };

	// One chunk emits key 3 consecutively (fast path), then key 5, then key 3 again
	// (linear-scan path), then key 5 again.
	std::vector<int64_t> out(key_count, 0);
	reductions::parallel_keyed_sum<int64_t>(pool, 1, key_count, out,
		[](std::size_t /*chunk_idx*/, reductions::KeyedEmitter<int64_t>& emit) {
			emit.add(3, 10);
			emit.add(3, 20);
			emit.add(5, 100);
			emit.add(3, 30);
			emit.add(5, 200);
		});

	CHECK(out[3] == 60);
	CHECK(out[5] == 300);
	for (std::size_t k : { 0u, 1u, 2u, 4u, 6u, 7u }) {
		CHECK(out[k] == 0);
	}
}

TEST_CASE("parallel_keyed_sum leaves untouched keys at their initial value", "[ecs][Reductions][keyed]") {
	std::size_t const key_count = 10;
	EcsThreadPool pool { 4 };

	// out pre-initialized to a sentinel: emitted keys get sentinel + contribution,
	// untouched keys keep the sentinel.
	std::vector<int64_t> out(key_count, 42);
	reductions::parallel_keyed_sum<int64_t>(pool, 2, key_count, out,
		[](std::size_t chunk_idx, reductions::KeyedEmitter<int64_t>& emit) {
			emit.add(chunk_idx, static_cast<int64_t>(chunk_idx + 1) * 100);
		});

	CHECK(out[0] == 142);
	CHECK(out[1] == 242);
	for (std::size_t k = 2; k < key_count; ++k) {
		CHECK(out[k] == 42);
	}
}

TEST_CASE("parallel_keyed_sum on zero chunks leaves out unchanged", "[ecs][Reductions][keyed]") {
	std::size_t const key_count = 5;
	EcsThreadPool pool { 4 };

	std::vector<int64_t> out(key_count, 7);
	reductions::parallel_keyed_sum<int64_t>(pool, 0, key_count, out,
		[](std::size_t /*chunk_idx*/, reductions::KeyedEmitter<int64_t>& emit) {
			emit.add(0, 1);
		});

	for (std::size_t k = 0; k < key_count; ++k) {
		CHECK(out[k] == 7);
	}
}

TEST_CASE("parallel_keyed_sum scratch reuse does not leak entries across calls", "[ecs][Reductions][keyed]") {
	std::size_t const key_count = 16;
	EcsThreadPool pool { 4 };
	reductions::KeyedSumScratch<int64_t> scratch;

	// First call: more chunks, different data — fills the scratch.
	std::vector<int64_t> first(key_count, 0);
	reductions::parallel_keyed_sum<int64_t>(pool, 8, key_count, first, scratch,
		[](std::size_t chunk_idx, reductions::KeyedEmitter<int64_t>& emit) {
			emit.add(chunk_idx % 16, static_cast<int64_t>(chunk_idx) * 1000 + 1);
			emit.add((chunk_idx + 3) % 16, 5);
		});

	// Second call: fewer chunks through the same scratch. Stale entries from the first
	// call must not contribute.
	std::vector<int64_t> out(key_count, 0);
	reductions::parallel_keyed_sum<int64_t>(pool, 3, key_count, out, scratch,
		[](std::size_t chunk_idx, reductions::KeyedEmitter<int64_t>& emit) {
			emit.add(chunk_idx, static_cast<int64_t>(chunk_idx + 1));
		});

	std::vector<int64_t> expected(key_count, 0);
	for (std::size_t i = 0; i < 3; ++i) {
		expected[i] += static_cast<int64_t>(i + 1);
	}
	for (std::size_t k = 0; k < key_count; ++k) {
		CHECK(out[k] == expected[k]);
	}
}

TEST_CASE("parallel_keyed_sum works with fixed_point_t and stays worker-count-invariant",
          "[ecs][Reductions][keyed][determinism]") {
	std::size_t const chunk_count = 100;
	std::size_t const key_count = 13;

	std::vector<int64_t> baseline_raw;
	{
		EcsThreadPool serial { 1 };
		std::vector<fixed_point_t> out(key_count, fixed_point_t::_0);
		reductions::parallel_keyed_sum<fixed_point_t>(serial, chunk_count, key_count, out,
			[key_count](std::size_t chunk_idx, reductions::KeyedEmitter<fixed_point_t>& emit) {
				for (std::size_t row = 0; row < 8; ++row) {
					emit.add(
						(chunk_idx * 5 + row / 2) % key_count,
						fixed_point_t { static_cast<int32_t>(chunk_idx * 7 + row) } / 3
					);
				}
			});
		for (fixed_point_t const& v : out) {
			baseline_raw.push_back(v.get_raw_value());
		}
	}

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		EcsThreadPool pool { wc };
		std::vector<fixed_point_t> out(key_count, fixed_point_t::_0);
		reductions::parallel_keyed_sum<fixed_point_t>(pool, chunk_count, key_count, out,
			[key_count](std::size_t chunk_idx, reductions::KeyedEmitter<fixed_point_t>& emit) {
				for (std::size_t row = 0; row < 8; ++row) {
					emit.add(
						(chunk_idx * 5 + row / 2) % key_count,
						fixed_point_t { static_cast<int32_t>(chunk_idx * 7 + row) } / 3
					);
				}
			});
		for (std::size_t k = 0; k < key_count; ++k) {
			CHECK(out[k].get_raw_value() == baseline_raw[k]);
		}
	}
}
