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
	struct SchX {
		int64_t v = 0;
	};
	struct SchY {
		int64_t v = 0;
	};
	struct SchZ {
		int64_t v = 0;
	};
	struct SchW {
		int64_t v = 0;
	};
}

ECS_COMPONENT(SchX, "bench_Scheduler::SchX")
ECS_COMPONENT(SchY, "bench_Scheduler::SchY")
ECS_COMPONENT(SchZ, "bench_Scheduler::SchZ")
ECS_COMPONENT(SchW, "bench_Scheduler::SchW")

namespace {
	// Four systems writing to four different components. No access conflicts, so the
	// scheduler can run them all in one parallel stage.
	struct SchSysX : System<SchSysX> {
		void tick(TickContext const& /*ctx*/, SchX& x) {
			x.v = x.v * 31 + 7;
		}
	};
	struct SchSysY : System<SchSysY> {
		void tick(TickContext const& /*ctx*/, SchY& y) {
			y.v = y.v * 31 + 7;
		}
	};
	struct SchSysZ : System<SchSysZ> {
		void tick(TickContext const& /*ctx*/, SchZ& z) {
			z.v = z.v * 31 + 7;
		}
	};
	struct SchSysW : System<SchSysW> {
		void tick(TickContext const& /*ctx*/, SchW& w) {
			w.v = w.v * 31 + 7;
		}
	};

	// Four systems chained by W/W and R/W conflicts on a single component — must serialize.
	struct SchChain1 : System<SchChain1> {
		void tick(TickContext const& /*ctx*/, SchX& x) {
			x.v += 1;
		}
	};
	struct SchChain2 : System<SchChain2> {
		void tick(TickContext const& /*ctx*/, SchX& x) {
			x.v *= 2;
		}
	};
	struct SchChain3 : System<SchChain3> {
		void tick(TickContext const& /*ctx*/, SchX& x) {
			x.v -= 3;
		}
	};
	struct SchChain4 : System<SchChain4> {
		void tick(TickContext const& /*ctx*/, SchX& x) {
			x.v ^= 5;
		}
	};

	// One SystemThreaded mixed alongside three plain System<> — exercises the multi-system
	// mixed-stage code path the scheduler uses for parallel dispatch.
	struct SchMixedThreaded : SystemThreaded<SchMixedThreaded> {
		void tick(TickContext const& /*ctx*/, SchX& x) {
			x.v = x.v * 31 + 7;
		}
	};
}

ECS_SYSTEM(SchSysX)
ECS_SYSTEM(SchSysY)
ECS_SYSTEM(SchSysZ)
ECS_SYSTEM(SchSysW)
ECS_SYSTEM(SchChain1)
ECS_SYSTEM(SchChain2)
ECS_SYSTEM(SchChain3)
ECS_SYSTEM(SchChain4)
ECS_SYSTEM(SchMixedThreaded)

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}

	void populate4(World& world, std::size_t n) {
		for (std::size_t i = 0; i < n; ++i) {
			world.create_entity(SchX {}, SchY {}, SchZ {}, SchW {});
		}
	}
}

TEST_CASE("Scheduler: single-system stage", "[benchmarks][benchmark-ecs][ecs-scheduler]") {
	ankerl::nanobench::Bench bench;
	bench.title("scheduler dispatch (1 system)").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populate4(world, n);
		world.register_system<SchSysX>();

		bench.batch(n).run("tick_systems 1 system" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}

TEST_CASE("Scheduler: 4-system parallel-eligible stage", "[benchmarks][benchmark-ecs][ecs-scheduler]") {
	ankerl::nanobench::Bench bench;
	bench.title("scheduler dispatch (4 systems, no conflicts)").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populate4(world, n);
		world.register_system<SchSysX>();
		world.register_system<SchSysY>();
		world.register_system<SchSysZ>();
		world.register_system<SchSysW>();

		bench.batch(n).run("tick_systems 4-way" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}

TEST_CASE("Scheduler: 4-system conflict chain", "[benchmarks][benchmark-ecs][ecs-scheduler]") {
	ankerl::nanobench::Bench bench;
	bench.title("scheduler dispatch (4 systems, write-chain)").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		for (std::size_t i = 0; i < n; ++i) {
			world.create_entity(SchX { static_cast<int64_t>(i) });
		}
		world.register_system<SchChain1>();
		world.register_system<SchChain2>();
		world.register_system<SchChain3>();
		world.register_system<SchChain4>();

		bench.batch(n).run("tick_systems serialized chain" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}

TEST_CASE("Scheduler: mixed System<> + SystemThreaded stage", "[benchmarks][benchmark-ecs][ecs-scheduler]") {
	ankerl::nanobench::Bench bench;
	bench.title("scheduler dispatch (mixed System + SystemThreaded)").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populate4(world, n);
		world.register_system<SchMixedThreaded>();
		world.register_system<SchSysY>();
		world.register_system<SchSysZ>();
		world.register_system<SchSysW>();

		bench.batch(n).run("tick_systems mixed stage" + suffix(n), [&] {
			world.tick_systems(Date {});
		});
	}
}
