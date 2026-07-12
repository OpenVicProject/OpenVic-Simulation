#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct CbA {
		int v = 0;
	};
	struct CbB {
		float w = 0.0f;
	};
	// One float, no padding — author-asserted byte hash for the checksum enforcement.
	ECS_CHECKSUM_BYTES(CbB)
	struct CbTag {};
}

ECS_COMPONENT(CbA, "bench_CommandBuffer::CbA")
ECS_COMPONENT(CbB, "bench_CommandBuffer::CbB")
ECS_COMPONENT(CbTag, "bench_CommandBuffer::CbTag")

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}
}

TEST_CASE("CommandBuffer create_entity queue + apply", "[benchmarks][benchmark-ecs][ecs-cmdbuf]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer create_entity").unit("op");

	for (std::size_t n : COUNTS) {
		// Buffer queues N creates, then apply finalises them onto the World.
		bench.batch(n).run("queue + apply" + suffix(n), [&] {
			World world;
			CommandBuffer cb;
			for (std::size_t i = 0; i < n; ++i) {
				cb.create_entity(world, CbA { static_cast<int>(i) }, CbB { 0.0f });
			}
			cb.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		// Direct World::create_entity baseline — same outcome, no buffer indirection.
		bench.batch(n).run("direct world.create_entity (baseline)" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(CbA { static_cast<int>(i) }, CbB { 0.0f });
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

TEST_CASE("CommandBuffer destroy_entity queue + apply", "[benchmarks][benchmark-ecs][ecs-cmdbuf]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer destroy_entity").unit("op");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("queue + apply" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(CbA { static_cast<int>(i) }, CbB { 0.0f }));
			}
			CommandBuffer cb;
			for (EntityID id : ids) {
				cb.destroy_entity(id);
			}
			cb.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

TEST_CASE("CommandBuffer add_component queue + apply", "[benchmarks][benchmark-ecs][ecs-cmdbuf]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer add_component").unit("op");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("queue + apply" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(CbA { static_cast<int>(i) }));
			}
			CommandBuffer cb;
			for (EntityID id : ids) {
				cb.add_component<CbB>(id, CbB {});
			}
			cb.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

TEST_CASE("CommandBuffer remove_component queue + apply", "[benchmarks][benchmark-ecs][ecs-cmdbuf]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer remove_component").unit("op");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("queue + apply" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(CbA { static_cast<int>(i) }, CbTag {}));
			}
			CommandBuffer cb;
			for (EntityID id : ids) {
				cb.remove_component<CbTag>(id);
			}
			cb.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

// Mixed-op workload: a more realistic pattern with creates, adds, and destroys interleaved.
TEST_CASE("CommandBuffer mixed-op queue + apply", "[benchmarks][benchmark-ecs][ecs-cmdbuf]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer mixed ops").unit("op");

	for (std::size_t n : COUNTS) {
		// 3 ops per cycle: create, add, destroy.
		bench.batch(n * 3).run("create+add+destroy x N" + suffix(n), [&] {
			World world;
			std::vector<EntityID> existing;
			existing.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				existing.push_back(world.create_entity(CbA { static_cast<int>(i) }));
			}
			CommandBuffer cb;
			for (std::size_t i = 0; i < n; ++i) {
				cb.create_entity(world, CbA { static_cast<int>(i + n) }, CbB { 0.0f });
			}
			for (EntityID id : existing) {
				cb.add_component<CbB>(id, CbB { 1.0f });
			}
			for (EntityID id : existing) {
				cb.destroy_entity(id);
			}
			cb.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

// merge_from cost: build several buffers (simulating per-chunk recording in SystemThreaded)
// and merge them into one before apply.
TEST_CASE("CommandBuffer merge_from then apply", "[benchmarks][benchmark-ecs][ecs-cmdbuf]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer merge_from").unit("op");
	constexpr std::size_t shard_count = 8;

	for (std::size_t n : COUNTS) {
		std::size_t const per_shard = n / shard_count;
		std::size_t const total = per_shard * shard_count;

		bench.batch(total).run("8 shards × creates + merge + apply" + suffix(n), [&] {
			World world;
			std::vector<CommandBuffer> shards(shard_count);
			for (std::size_t s = 0; s < shard_count; ++s) {
				for (std::size_t i = 0; i < per_shard; ++i) {
					shards[s].create_entity(world, CbA { static_cast<int>(i) }, CbB { 0.0f });
				}
			}
			CommandBuffer pending;
			for (CommandBuffer& shard : shards) {
				pending.merge_from(std::move(shard));
			}
			pending.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}
