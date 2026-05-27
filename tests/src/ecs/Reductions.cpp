#include "openvic-simulation/ecs/EcsThreadPool.hpp"
#include "openvic-simulation/ecs/Reductions.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

TEST_CASE("parallel_sum is bit-identical across worker counts", "[ecs][Reductions][determinism]") {
	std::size_t const N = 1000;
	std::vector<int64_t> data(N);
	for (std::size_t i = 0; i < N; ++i) {
		data[i] = static_cast<int64_t>(i * 17 + 3);
	}

	int64_t baseline = 0;
	{
		EcsThreadPool serial { 1 };
		baseline = reductions::parallel_sum<int64_t>(serial, N, int64_t { 0 },
			[&data](std::size_t i) { return data[i]; });
	}

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		EcsThreadPool pool { wc };
		int64_t result = reductions::parallel_sum<int64_t>(pool, N, int64_t { 0 },
			[&data](std::size_t i) { return data[i]; });
		CHECK(result == baseline);
	}
}

TEST_CASE("parallel_min returns smallest body result", "[ecs][Reductions]") {
	std::size_t const N = 100;
	EcsThreadPool pool { 4 };
	int64_t result = reductions::parallel_min<int64_t>(pool, N, INT64_MAX,
		[](std::size_t i) { return static_cast<int64_t>((i * 13) % 97); });

	int64_t expected = INT64_MAX;
	for (std::size_t i = 0; i < N; ++i) {
		int64_t v = static_cast<int64_t>((i * 13) % 97);
		if (v < expected) {
			expected = v;
		}
	}
	CHECK(result == expected);
}

TEST_CASE("parallel_max returns largest body result", "[ecs][Reductions]") {
	std::size_t const N = 100;
	EcsThreadPool pool { 4 };
	int64_t result = reductions::parallel_max<int64_t>(pool, N, INT64_MIN,
		[](std::size_t i) { return static_cast<int64_t>((i * 13) % 97); });

	int64_t expected = INT64_MIN;
	for (std::size_t i = 0; i < N; ++i) {
		int64_t v = static_cast<int64_t>((i * 13) % 97);
		if (v > expected) {
			expected = v;
		}
	}
	CHECK(result == expected);
}

TEST_CASE("parallel_sum on zero chunks returns init", "[ecs][Reductions]") {
	EcsThreadPool pool { 4 };
	int64_t result = reductions::parallel_sum<int64_t>(pool, 0, int64_t { 42 },
		[](std::size_t /*i*/) { return int64_t { 0 }; });
	CHECK(result == 42);
}
