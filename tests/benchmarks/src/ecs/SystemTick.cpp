#include "openvic-simulation/ecs/ChunkSystem.hpp"
#include "openvic-simulation/ecs/ChunkView.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	// Integer arithmetic — deterministic across worker counts. Same kernel shape as
	// tests/src/ecs/WorkerCountInvariance.cpp so the bench numbers stay comparable.
	struct TickValue {
		int64_t v = 0;
	};
	struct TickDelta {
		int64_t d = 0;
	};
}

ECS_COMPONENT(TickValue, "bench_SystemTick::TickValue")
ECS_COMPONENT(TickDelta, "bench_SystemTick::TickDelta")

namespace {
	// Serial CRTP system — single thread, per-row tick.
	struct TickSerial : System<TickSerial> {
		void tick(TickContext const& /*ctx*/, TickValue& v, TickDelta const& d) {
			v.v = v.v * 31 + d.d * 7;
		}
	};

	// Threaded CRTP system — chunk-parallel via the EcsThreadPool. Inherits System<>'s
	// per-row tick signature; the scheduler dispatches chunks across the pool.
	struct TickThreaded : SystemThreaded<TickThreaded> {
		void tick(TickContext const& /*ctx*/, TickValue& v, TickDelta const& d) {
			v.v = v.v * 31 + d.d * 7;
		}
	};

	// Chunk-tight CRTP system — receives whole-chunk slabs, runs a tight inner loop with
	// no per-row function-call overhead. Expected to be the fastest serial form.
	struct TickChunk : ChunkSystem<TickChunk, TickValue, TickDelta const> {
		void tick_chunk(ChunkView<TickValue, TickDelta> view, TickContext const& /*ctx*/) {
			TickValue* val = view.template array<TickValue>();
			TickDelta* del = view.template array<TickDelta>();
			std::size_t const count = view.count();
			for (std::size_t i = 0; i < count; ++i) {
				val[i].v = val[i].v * 31 + del[i].d * 7;
			}
		}
	};
}

ECS_SYSTEM(TickSerial)
ECS_SYSTEM(TickThreaded)
ECS_SYSTEM(TickChunk)

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };
	constexpr uint32_t WORKER_COUNTS[] = { 1, 2, 4, 8, 16 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}

	void populate(World& world, std::size_t n) {
		for (std::size_t i = 0; i < n; ++i) {
			world.create_entity(
				TickValue { static_cast<int64_t>(i + 1) },
				TickDelta { static_cast<int64_t>((i * 17) % 13 + 1) }
			);
		}
	}
}

TEST_CASE("System<> serial tick", "[benchmarks][benchmark-ecs][ecs-systick]") {
	ankerl::nanobench::Bench bench;
	bench.title("System<> serial tick").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populate(world, n);
		world.register_system<TickSerial>();

		bench.batch(n).run("System<TickSerial>" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}

TEST_CASE("ChunkSystem<> tight inner loop tick", "[benchmarks][benchmark-ecs][ecs-systick]") {
	ankerl::nanobench::Bench bench;
	bench.title("ChunkSystem<> tick").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populate(world, n);
		world.register_system<TickChunk>();

		bench.batch(n).run("ChunkSystem<TickChunk>" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}

TEST_CASE("SystemThreaded<> chunk-parallel tick (worker-count sweep)", "[benchmarks][benchmark-ecs][ecs-systick]") {
	ankerl::nanobench::Bench bench;
	bench.title("SystemThreaded<> worker-count sweep").unit("entity");

	for (std::size_t n : COUNTS) {
		for (uint32_t wc : WORKER_COUNTS) {
			World world;
			world.set_ecs_worker_count(wc);
			populate(world, n);
			world.register_system<TickThreaded>();

			std::string const label =
				"SystemThreaded<TickThreaded> workers=" + std::to_string(wc) + suffix(n);
			bench.batch(n).run(label, [&] {
				world.tick_systems(Date {});
			});
		}
	}
}
