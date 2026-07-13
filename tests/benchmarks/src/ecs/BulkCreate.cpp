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

// Bulk entity creation (ECS_SIM_ARCHITECTURE §9 item 4) vs the equivalent create_entity
// loop, on both the direct World path (session setup: ~300k pops) and the CommandBuffer
// path (war-time regiment churn). Loop and bulk variants run in the SAME process so
// binary-layout noise cancels (see the microbenchmark noise caveat in the repo notes).

namespace {
	struct BkA {
		int v = 0;
	};
	struct BkB {
		float w = 0.0f;
	};
	// One float, no padding — author-asserted byte hash for the checksum enforcement.
	ECS_CHECKSUM_BYTES(BkB)
	struct BkTag {};
}

ECS_COMPONENT(BkA, "bench_BulkCreate::BkA")
ECS_COMPONENT(BkB, "bench_BulkCreate::BkB")
ECS_COMPONENT(BkTag, "bench_BulkCreate::BkTag")

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 100000, 300000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}

	// Trivially-copyable inputs: the bulk move path memcpys and leaves sources intact, so
	// the same spans can feed every benchmark iteration.
	struct Inputs {
		std::vector<BkA> as;
		std::vector<BkB> bs;
		std::vector<EntityID> out_ids;

		explicit Inputs(std::size_t n) {
			as.reserve(n);
			bs.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				as.push_back(BkA { static_cast<int>(i) });
				bs.push_back(BkB { static_cast<float>(i) });
			}
			out_ids.resize(n);
		}
	};
}

TEST_CASE("Direct bulk create vs create_entity loop", "[benchmarks][benchmark-ecs][ecs-bulk]") {
	ankerl::nanobench::Bench bench;
	bench.title("World::create_entities vs loop").unit("entity");

	for (std::size_t n : COUNTS) {
		Inputs inputs { n };

		bench.batch(n).run("loop world.create_entity" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(BkA { static_cast<int>(i) }, BkB { static_cast<float>(i) }, BkTag {});
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		bench.batch(n).run("bulk world.create_entities" + suffix(n), [&] {
			World world;
			world.create_entities<BkA, BkB, BkTag>(n, inputs.out_ids, inputs.as, inputs.bs);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

TEST_CASE("CommandBuffer bulk create vs create_entity loop", "[benchmarks][benchmark-ecs][ecs-bulk]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer::create_entities vs loop").unit("entity");

	for (std::size_t n : COUNTS) {
		Inputs inputs { n };

		bench.batch(n).run("loop cmd.create_entity + apply" + suffix(n), [&] {
			World world;
			CommandBuffer cmd;
			for (std::size_t i = 0; i < n; ++i) {
				cmd.create_entity(world, BkA { static_cast<int>(i) }, BkB { static_cast<float>(i) }, BkTag {});
			}
			cmd.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		bench.batch(n).run("bulk cmd.create_entities + apply" + suffix(n), [&] {
			World world;
			CommandBuffer cmd;
			cmd.create_entities<BkA, BkB, BkTag>(world, n, inputs.out_ids, inputs.as, inputs.bs);
			cmd.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

TEST_CASE("CommandBuffer parallel-mode bulk create vs loop", "[benchmarks][benchmark-ecs][ecs-bulk]") {
	ankerl::nanobench::Bench bench;
	bench.title("CommandBuffer parallel bulk vs loop").unit("entity");

	for (std::size_t n : COUNTS) {
		Inputs inputs { n };

		bench.batch(n).run("loop parallel cmd.create_entity + apply" + suffix(n), [&] {
			World world;
			CommandBuffer cmd;
			cmd.set_parallel_mode(true);
			for (std::size_t i = 0; i < n; ++i) {
				cmd.create_entity(world, BkA { static_cast<int>(i) }, BkB { static_cast<float>(i) }, BkTag {});
			}
			cmd.set_parallel_mode(false);
			cmd.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		bench.batch(n).run("bulk parallel cmd.create_entities + apply" + suffix(n), [&] {
			World world;
			CommandBuffer cmd;
			cmd.set_parallel_mode(true);
			cmd.create_entities<BkA, BkB, BkTag>(world, n, inputs.out_ids, inputs.as, inputs.bs);
			cmd.set_parallel_mode(false);
			cmd.apply(world);
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}
