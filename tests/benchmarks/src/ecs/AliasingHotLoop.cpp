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

// Aliasing-sensitive hot-loop kernel. Three components, two of them written multiple
// times per row, all three reached in a tight loop body. This pattern is the one a
// `restrict` (or hoisted-typed-pointer) pass should help most: without aliasing info
// the compiler must assume writes through A* may invalidate reads through B* and C*,
// forcing reloads between successive statements; with the assertion in place it can
// keep loop-invariant pointers in registers, batch loads, and reorder freely.
//
// Same kernel runs through all three system bases (`System`, `SystemThreaded`,
// `ChunkSystem`) so the per-base before/after numbers are directly comparable.
//
// Integer arithmetic (int64_t) for determinism — `WorkerCountInvariance.cpp` style.
// Components live in one archetype so per-chunk work dominates archetype-walk cost.
namespace {
	struct AliasA {
		int64_t x = 0;
		int64_t y = 0;
	};
	struct AliasB {
		int64_t x = 0;
		int64_t y = 0;
	};
	struct AliasC {
		int64_t k = 0;
		int64_t m = 0;
	};
}

ECS_COMPONENT(AliasA, "bench_AliasingHotLoop::AliasA")
ECS_COMPONENT(AliasB, "bench_AliasingHotLoop::AliasB")
ECS_COMPONENT(AliasC, "bench_AliasingHotLoop::AliasC")

namespace {
	struct AliasSerial : System<AliasSerial> {
		void tick(TickContext const& /*ctx*/, AliasA& a, AliasB& b, AliasC const& c) {
			a.x = a.x * c.k + b.x;
			b.x = b.x + a.x * c.m;
			a.y = a.y * c.k - b.y;
			b.y = b.y - a.y * c.m;
		}
	};

	struct AliasThreaded : SystemThreaded<AliasThreaded> {
		void tick(TickContext const& /*ctx*/, AliasA& a, AliasB& b, AliasC const& c) {
			a.x = a.x * c.k + b.x;
			b.x = b.x + a.x * c.m;
			a.y = a.y * c.k - b.y;
			b.y = b.y - a.y * c.m;
		}
	};

	struct AliasChunk : ChunkSystem<AliasChunk, AliasA, AliasB, AliasC const> {
		void tick_chunk(ChunkView<AliasA, AliasB, AliasC> view, TickContext const& /*ctx*/) {
			AliasA* a = view.template array<AliasA>();
			AliasB* b = view.template array<AliasB>();
			AliasC* c = view.template array<AliasC>();
			std::size_t const count = view.count();
			for (std::size_t i = 0; i < count; ++i) {
				a[i].x = a[i].x * c[i].k + b[i].x;
				b[i].x = b[i].x + a[i].x * c[i].m;
				a[i].y = a[i].y * c[i].k - b[i].y;
				b[i].y = b[i].y - a[i].y * c[i].m;
			}
		}
	};
}

ECS_SYSTEM(AliasSerial)
ECS_SYSTEM(AliasThreaded)
ECS_SYSTEM(AliasChunk)

namespace {
	constexpr std::size_t COUNTS[] = { 10000, 100000, 1000000 };
	constexpr uint32_t WORKER_COUNTS[] = { 1, 2, 4, 8 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}

	void populate(World& world, std::size_t n) {
		for (std::size_t i = 0; i < n; ++i) {
			int64_t const seed = static_cast<int64_t>(i);
			world.create_entity(
				AliasA { seed + 1, seed + 2 },
				AliasB { seed * 3 + 1, seed * 3 + 2 },
				AliasC { (seed % 7) + 1, (seed % 11) + 1 }
			);
		}
	}
}

TEST_CASE("System<> aliasing hot loop", "[benchmarks][benchmark-ecs][ecs-aliasing]") {
	ankerl::nanobench::Bench bench;
	bench.title("System<> aliasing hot loop").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populate(world, n);
		world.register_system<AliasSerial>();

		bench.batch(n).run("System<AliasSerial>" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}

TEST_CASE("SystemThreaded<> aliasing hot loop (worker-count sweep)", "[benchmarks][benchmark-ecs][ecs-aliasing]") {
	ankerl::nanobench::Bench bench;
	bench.title("SystemThreaded<> aliasing hot loop").unit("entity");

	for (std::size_t n : COUNTS) {
		for (uint32_t wc : WORKER_COUNTS) {
			World world;
			world.set_ecs_worker_count(wc);
			populate(world, n);
			world.register_system<AliasThreaded>();

			std::string const label =
				"SystemThreaded<AliasThreaded> workers=" + std::to_string(wc) + suffix(n);
			bench.batch(n).run(label, [&] {
				world.tick_systems(Date {});
			});
		}
	}
}

TEST_CASE("ChunkSystem<> aliasing hot loop", "[benchmarks][benchmark-ecs][ecs-aliasing]") {
	ankerl::nanobench::Bench bench;
	bench.title("ChunkSystem<> aliasing hot loop").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populate(world, n);
		world.register_system<AliasChunk>();

		bench.batch(n).run("ChunkSystem<AliasChunk>" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}
