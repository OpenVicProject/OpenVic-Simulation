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

// === should_run gate vs in-body early-out, on ticks where the system does nothing ===
// The case the predicate exists for: a monthly SystemThreaded on a non-month-start day.
// With an in-body early-out the skipped tick still pays collect_chunks plus one no-op
// work item per chunk; with should_run it costs one predicate call. Both variants run in
// the SAME process and the same Bench so binary-layout noise cancels (cross-build
// comparisons of sub-microsecond kernels swing wildly on byte-identical code).

namespace {
	struct GateVal {
		int64_t v = 0;
	};
	struct RunVal {
		int64_t v = 0;
	};
}

ECS_COMPONENT(GateVal, "bench_SystemShouldRun::GateVal")
ECS_COMPONENT(RunVal, "bench_SystemShouldRun::RunVal")

namespace {
	// Monthly system gated by the dispatcher — skipped ticks never enumerate chunks.
	struct GatePred : SystemThreaded<GatePred> {
		static bool should_run(TickContext const& ctx) {
			return ctx.today.is_month_start();
		}
		void tick(TickContext const& /*ctx*/, GateVal& v) {
			v.v = v.v * 31 + 7;
		}
	};

	// Same kernel gated inside the body — skipped ticks still collect chunks and
	// dispatch one no-op work item (or row loop) per chunk.
	struct GateBody : SystemThreaded<GateBody> {
		void tick(TickContext const& ctx, GateVal& v) {
			if (!ctx.today.is_month_start()) {
				return;
			}
			v.v = v.v * 31 + 7;
		}
	};

	// Tiny co-staged runner (disjoint RunVal access) — forces the multi-system-stage
	// combined-work-item path while contributing negligible work of its own, so the
	// measured delta is the skipped system's dispatch overhead.
	struct SmallRunner : SystemThreaded<SmallRunner> {
		void tick(TickContext const& /*ctx*/, RunVal& r) {
			r.v = r.v * 31 + 3;
		}
	};
}

ECS_SYSTEM(GatePred)
ECS_SYSTEM(GateBody)
ECS_SYSTEM(SmallRunner)

namespace {
	// Large enough that the skipped system's chunk count dominates: 1M single-int64
	// entities span on the order of thousands of chunks — the shape the predicate
	// targets (~6300 chunks for the pop archetypes).
	constexpr std::size_t COUNTS[] = { 100000, 1000000 };
	constexpr std::size_t RUNNER_ENTITIES = 64;

	// Never a month start — every benched tick is a skipped tick.
	constexpr Date SKIP_DAY { 1836, 1, 15 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}

	void populate_gated(World& world, std::size_t n) {
		for (std::size_t i = 0; i < n; ++i) {
			world.create_entity(GateVal { static_cast<int64_t>(i + 1) });
		}
	}
}

TEST_CASE("Skipped SystemThreaded, single-system stage: should_run vs in-body early-out",
          "[benchmarks][benchmark-ecs][ecs-shouldrun]") {
	ankerl::nanobench::Bench bench;
	bench.title("skipped tick, single-system stage").unit("tick");

	for (std::size_t n : COUNTS) {
		World gated;
		populate_gated(gated, n);
		gated.register_system<GatePred>();
		bench.run("should_run gate" + suffix(n), [&] {
			gated.tick_systems(SKIP_DAY);
		});

		World body;
		populate_gated(body, n);
		body.register_system<GateBody>();
		bench.run("in-body early-out" + suffix(n), [&] {
			body.tick_systems(SKIP_DAY);
		});
	}
}

TEST_CASE("Skipped SystemThreaded, multi-system stage: should_run vs in-body early-out",
          "[benchmarks][benchmark-ecs][ecs-shouldrun]") {
	ankerl::nanobench::Bench bench;
	bench.title("skipped tick, multi-system stage").unit("tick");

	for (std::size_t n : COUNTS) {
		World gated;
		populate_gated(gated, n);
		for (std::size_t i = 0; i < RUNNER_ENTITIES; ++i) {
			gated.create_entity(RunVal { static_cast<int64_t>(i + 1) });
		}
		gated.register_system<GatePred>();
		gated.register_system<SmallRunner>();
		bench.run("should_run gate" + suffix(n), [&] {
			gated.tick_systems(SKIP_DAY);
		});

		World body;
		populate_gated(body, n);
		for (std::size_t i = 0; i < RUNNER_ENTITIES; ++i) {
			body.create_entity(RunVal { static_cast<int64_t>(i + 1) });
		}
		body.register_system<GateBody>();
		body.register_system<SmallRunner>();
		bench.run("in-body early-out" + suffix(n), [&] {
			body.tick_systems(SKIP_DAY);
		});
	}
}
