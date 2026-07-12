#include "openvic-simulation/ecs/Checksum.hpp"
#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstddef>
#include <cstdint>
#include <thread>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === Optional static should_run(TickContext const&) predicate — dispatch-time cadence gate ===
// Covers: (a) gate semantics (false / true / absent), (b) exactly-once-per-tick main-thread
// evaluation across all dispatch branches, (c) a multi-system stage where one SystemThreaded
// skips while co-staged systems run, worker-count invariant via the full-state checksum,
// (d) schedule stability — schedule_hash and stage layout are independent of predicates and
// skip patterns, (e) should_run gating ≡ in-body early-out, (f) the compile-time trait wall.

namespace {
	struct SrA {
		int64_t v = 0;
	};
	struct SrB {
		int64_t v = 0;
	};
	struct SrC {
		int64_t v = 0;
	};
	// Eval-count singleton, incremented from inside should_run via ctx.world (NOT a system
	// member or global counter — proves the predicate reaches deterministic world state).
	struct SrEvalLog {
		int64_t evals = 0;
	};
}
ECS_COMPONENT(SrA, "test_SystemShouldRun::A")
ECS_COMPONENT(SrB, "test_SystemShouldRun::B")
ECS_COMPONENT(SrC, "test_SystemShouldRun::C")
ECS_COMPONENT(SrEvalLog, "test_SystemShouldRun::EvalLog")

// === (f) compile-time enforcement — void_t detection traits, paired positive/negative so a
// typo cannot make a negative pass vacuously (MSVC mishandles !requires{...}; see
// ImmutableEntity.cpp for the pattern rationale). The registration gate in
// World::register_system is exactly `!has_should_run_v<S> || should_run_signature_valid_v<S>`,
// so these assertions prove which shapes register cleanly (absent / valid / valid-noexcept)
// and which hard-error (everything present-but-wrong). The wall structs are plain types —
// the traits don't require the CRTP System base. ===
namespace {
	struct SrValidPred {
		static bool should_run(TickContext const&) { return true; }
	};
	struct SrNoexceptPred {
		static bool should_run(TickContext const&) noexcept { return true; }
	};
	struct SrAbsent {};
	struct SrNonStatic {
		bool should_run(TickContext const&) { return true; }
	};
	struct SrWrongParam {
		static bool should_run(int) { return true; }
	};
	struct SrWrongRet {
		static void should_run(TickContext const&) {}
	};
	struct SrDataMember {
		bool should_run = true;
	};
	// Callable static data member (fn-pointer; the lambda-typed `static constexpr auto`
	// variant has the same shape) — callable with the right signature, but not a real
	// static function. Deliberately rejected.
	struct SrFnPtrMember {
		static constexpr bool (*should_run)(TickContext const&) = [](TickContext const&) {
			return true;
		};
	};
	// Overload set containing a valid candidate: presence is caught by the call probe
	// (address-of is ambiguous), validity fails (ambiguous &) — hard error, never a
	// silent pick-one.
	struct SrOverloaded {
		static bool should_run(TickContext const&) { return true; }
		static bool should_run(Date) { return true; }
	};

	static_assert(has_should_run_v<SrValidPred> && should_run_signature_valid_v<SrValidPred>);
	static_assert(has_should_run_v<SrNoexceptPred> && should_run_signature_valid_v<SrNoexceptPred>);
	static_assert(!has_should_run_v<SrAbsent>);
	static_assert(has_should_run_v<SrNonStatic> && !should_run_signature_valid_v<SrNonStatic>);
	static_assert(has_should_run_v<SrWrongParam> && !should_run_signature_valid_v<SrWrongParam>);
	static_assert(has_should_run_v<SrWrongRet> && !should_run_signature_valid_v<SrWrongRet>);
	static_assert(has_should_run_v<SrDataMember> && !should_run_signature_valid_v<SrDataMember>);
	static_assert(has_should_run_v<SrFnPtrMember> && !should_run_signature_valid_v<SrFnPtrMember>);
	static_assert(has_should_run_v<SrOverloaded> && !should_run_signature_valid_v<SrOverloaded>);
}

TEST_CASE("should_run compile-time guarantees hold (static_assert wall)",
          "[ecs][scheduler][should_run][compiletime]") {
	// The static_assert block above is the real test; this case just surfaces it in the report.
	CHECK(true);
}

// === Main-thread instrumentation for (b). File-scope globals, NOT a singleton —
// std::thread::id has no guaranteed unique object representations and world_checksum hashes
// every singleton, so a thread-id singleton could trip the checksum traits. ===
namespace {
	std::thread::id sr_main_tid;
	bool sr_all_evals_on_main = true;

	// Shared instrumentation body. Mutating a singleton from should_run violates the
	// documented purity contract — this is deliberate TEST instrumentation exercising the
	// mechanics (eval count + thread), safe exactly because evaluation is main-thread
	// serial before the parallel section.
	void sr_record_eval(TickContext const& ctx) {
		SrEvalLog* log = ctx.world.get_singleton<SrEvalLog>();
		if (log != nullptr) {
			log->evals += 1;
		}
		if (std::this_thread::get_id() != sr_main_tid) {
			sr_all_evals_on_main = false;
		}
	}
}

// === Test systems ===
namespace {
	// (a) gate semantics.
	struct SrAlwaysOff : System<SrAlwaysOff> {
		static bool should_run(TickContext const&) { return false; }
		void tick(TickContext const&, SrA& a) { a.v += 1000; }
	};
	struct SrAlwaysOn : System<SrAlwaysOn> {
		static bool should_run(TickContext const&) { return true; }
		void tick(TickContext const&, SrB& b) { b.v += 1; }
	};
	struct SrNoPredicateSys : System<SrNoPredicateSys> {
		void tick(TickContext const&, SrC& c) { c.v += 1; }
	};

	// (b) exactly-once + main-thread.
	struct SrEvalCounter : System<SrEvalCounter> {
		static bool should_run(TickContext const& ctx) {
			sr_record_eval(ctx);
			return true;
		}
		void tick(TickContext const&, SrA& a) { a.v += 1; }
	};
	struct SrEvalCounterOff : System<SrEvalCounterOff> {
		static bool should_run(TickContext const& ctx) {
			sr_record_eval(ctx);
			return false;
		}
		void tick(TickContext const&, SrA& a) { a.v += 1000; }
	};
	// Disjoint fillers to force a multi-system stage around the counter.
	struct SrFillerB : System<SrFillerB> {
		void tick(TickContext const&, SrB& b) { b.v += 1; }
	};
	struct SrFillerC : System<SrFillerC> {
		void tick(TickContext const&, SrC& c) { c.v += 1; }
	};

	// (c)/(d) multi-system stage with a date-gated SystemThreaded. Disjoint write targets
	// (SrA / SrB / SrC) keep all three conflict-free → co-staged.
	struct SrThreadedGated : SystemThreaded<SrThreadedGated> {
		static bool should_run(TickContext const& ctx) {
			return ctx.today.is_month_start();
		}
		void tick(TickContext const&, SrA& a) { a.v = a.v * 31 + 7; }
	};
	// Predicate-free twin with identical access — for the (d) stage-layout comparison.
	struct SrThreadedTwin : SystemThreaded<SrThreadedTwin> {
		void tick(TickContext const&, SrA& a) { a.v = a.v * 31 + 7; }
	};
	struct SrThreadedRun : SystemThreaded<SrThreadedRun> {
		void tick(TickContext const&, SrB& b) { b.v = b.v * 31 + 3; }
	};
	struct SrSerialRun : System<SrSerialRun> {
		void tick(TickContext const&, SrC& c) { c.v = c.v * 31 + 5; }
	};

	// (e) gate ≡ in-body early-out, serial and threaded variants. Same kernel, same
	// is_month_start condition — one checked by the dispatcher, one inside the body.
	struct SrMonthlyGated : System<SrMonthlyGated> {
		static bool should_run(TickContext const& ctx) {
			return ctx.today.is_month_start();
		}
		void tick(TickContext const&, SrA& a) { a.v = a.v * 31 + 7; }
	};
	struct SrMonthlyEarlyOut : System<SrMonthlyEarlyOut> {
		void tick(TickContext const& ctx, SrA& a) {
			if (!ctx.today.is_month_start()) {
				return;
			}
			a.v = a.v * 31 + 7;
		}
	};
	struct SrMonthlyGatedThreaded : SystemThreaded<SrMonthlyGatedThreaded> {
		static bool should_run(TickContext const& ctx) {
			return ctx.today.is_month_start();
		}
		void tick(TickContext const&, SrA& a) { a.v = a.v * 31 + 7; }
	};
	struct SrMonthlyEarlyOutThreaded : SystemThreaded<SrMonthlyEarlyOutThreaded> {
		void tick(TickContext const& ctx, SrA& a) {
			if (!ctx.today.is_month_start()) {
				return;
			}
			a.v = a.v * 31 + 7;
		}
	};
}
ECS_SYSTEM(SrAlwaysOff)
ECS_SYSTEM(SrAlwaysOn)
ECS_SYSTEM(SrNoPredicateSys)
ECS_SYSTEM(SrEvalCounter)
ECS_SYSTEM(SrEvalCounterOff)
ECS_SYSTEM(SrFillerB)
ECS_SYSTEM(SrFillerC)
ECS_SYSTEM(SrThreadedGated)
ECS_SYSTEM(SrThreadedTwin)
ECS_SYSTEM(SrThreadedRun)
ECS_SYSTEM(SrSerialRun)
ECS_SYSTEM(SrMonthlyGated)
ECS_SYSTEM(SrMonthlyEarlyOut)
ECS_SYSTEM(SrMonthlyGatedThreaded)
ECS_SYSTEM(SrMonthlyEarlyOutThreaded)

// === (a) gate semantics ===

TEST_CASE("should_run false skips the tick body; true and absent always run",
          "[ecs][scheduler][should_run]") {
	World world;
	for (std::size_t i = 0; i < 100; ++i) {
		world.create_entity(
			SrA { static_cast<int64_t>(i) }, SrB { 0 }, SrC { 0 }
		);
	}
	world.register_system<SrAlwaysOff>();
	world.register_system<SrAlwaysOn>();
	world.register_system<SrNoPredicateSys>();

	int const ticks = 7;
	for (int t = 0; t < ticks; ++t) {
		world.tick_systems(Date {});
	}

	std::size_t row = 0;
	world.for_each<SrA, SrB, SrC>([&](SrA& a, SrB& b, SrC& c) {
		CHECK(a.v == static_cast<int64_t>(row)); // SrAlwaysOff never touched SrA
		CHECK(b.v == ticks); // SrAlwaysOn ran every tick
		CHECK(c.v == ticks); // absent predicate → always runs
		++row;
	});
	CHECK(row == 100);
}

// === (b) exactly-once per tick, on the main thread, in every dispatch branch ===

namespace {
	// Returns final SrEvalLog.evals after `ticks` ticks. `multi_stage` co-registers the
	// disjoint fillers so the counter system lands in a multi-system stage (asserted);
	// otherwise it is the lone system (single-system-stage branch).
	template<typename CounterSystem>
	int64_t eval_count_after(int ticks, bool multi_stage, bool serial_mode, int64_t* out_a_sum) {
		World world;
		world.set_serial_mode(serial_mode);
		world.set_singleton(SrEvalLog {});
		for (std::size_t i = 0; i < 50; ++i) {
			world.create_entity(SrA { 0 }, SrB { 0 }, SrC { 0 });
		}
		world.register_system<CounterSystem>();
		if (multi_stage) {
			world.register_system<SrFillerB>();
			world.register_system<SrFillerC>();
			REQUIRE(world.debug_stage_index_of(system_type_id_of<CounterSystem>())
				== world.debug_stage_index_of(system_type_id_of<SrFillerB>()));
			REQUIRE(world.debug_stage_index_of(system_type_id_of<CounterSystem>())
				== world.debug_stage_index_of(system_type_id_of<SrFillerC>()));
		}

		for (int t = 0; t < ticks; ++t) {
			world.tick_systems(Date {});
		}

		if (out_a_sum != nullptr) {
			int64_t sum = 0;
			world.for_each<SrA>([&](SrA& a) { sum += a.v; });
			*out_a_sum = sum;
		}
		SrEvalLog const* log = world.get_singleton<SrEvalLog>();
		REQUIRE(log != nullptr);
		return log->evals;
	}
}

TEST_CASE("should_run is evaluated exactly once per tick, on the main thread",
          "[ecs][scheduler][should_run]") {
	sr_main_tid = std::this_thread::get_id();
	sr_all_evals_on_main = true;
	int const ticks = 9;

	// Single-system stage: one eval per tick, never per chunk / per entity.
	int64_t a_sum = 0;
	CHECK(eval_count_after<SrEvalCounter>(ticks, false, false, &a_sum) == ticks);
	CHECK(a_sum == ticks * 50);

	// Multi-system stage: still one eval per tick.
	CHECK(eval_count_after<SrEvalCounter>(ticks, true, false, nullptr) == ticks);

	// serial_mode forces the serial branch for every stage — same exactly-once contract.
	CHECK(eval_count_after<SrEvalCounter>(ticks, true, true, nullptr) == ticks);

	// A false predicate still costs exactly one evaluation per tick — and the body never ran.
	a_sum = -1;
	CHECK(eval_count_after<SrEvalCounterOff>(ticks, false, false, &a_sum) == ticks);
	CHECK(a_sum == 0);
	CHECK(eval_count_after<SrEvalCounterOff>(ticks, true, false, nullptr) == ticks);

	CHECK(sr_all_evals_on_main);
}

// === (c) multi-system stage with a skipping SystemThreaded — worker-count invariance ===

namespace {
	// Tick the gated trio across `tick_count` consecutive days starting 1836-01-01 (the
	// skip pattern flips at every month boundary) and return the full-state checksum.
	uint64_t run_trio_and_checksum(
		uint32_t worker_count, bool serial_mode, std::size_t entity_count, int tick_count
	) {
		World world;
		world.set_ecs_worker_count(worker_count);
		world.set_serial_mode(serial_mode);
		for (std::size_t i = 0; i < entity_count; ++i) {
			world.create_entity(
				SrA { static_cast<int64_t>(i + 1) },
				SrB { static_cast<int64_t>((i * 17) % 13 + 1) },
				SrC { static_cast<int64_t>((i * 7) % 11 + 1) }
			);
		}
		world.register_system<SrThreadedGated>();
		world.register_system<SrThreadedRun>();
		world.register_system<SrSerialRun>();

		Date today { 1836, 1, 1 };
		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(today);
			++today;
		}
		return world_checksum(world);
	}
}

TEST_CASE("Skipping SystemThreaded in a multi-system stage: checksum identical across worker counts",
          "[ecs][determinism][should_run]") {
	std::size_t const entities = 500;
	int const ticks = 65; // spans the Feb 1 and Mar 1 month starts → skip pattern varies

	// The trio really is one stage (disjoint SrA/SrB/SrC access → conflict-free).
	{
		World world;
		world.create_entity(SrA {}, SrB {}, SrC {});
		world.register_system<SrThreadedGated>();
		world.register_system<SrThreadedRun>();
		world.register_system<SrSerialRun>();
		std::size_t const stage = world.debug_stage_index_of(system_type_id_of<SrThreadedGated>());
		REQUIRE(world.debug_stage_count() == 1);
		REQUIRE(stage == world.debug_stage_index_of(system_type_id_of<SrThreadedRun>()));
		REQUIRE(stage == world.debug_stage_index_of(system_type_id_of<SrSerialRun>()));
	}

	uint64_t const baseline = run_trio_and_checksum(1, false, entities, ticks);
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		uint64_t const result = run_trio_and_checksum(wc, false, entities, ticks);
		CHECK(result == baseline);
	}
	// serial_mode cross-check: the predicate pass sits at stage top, so the serial branch
	// observes the exact same skip decisions as the parallel branch.
	uint64_t const serial_result = run_trio_and_checksum(8, true, entities, ticks);
	CHECK(serial_result == baseline);
}

TEST_CASE("Skipping SystemThreaded alone in its stage: checksum identical across worker counts",
          "[ecs][determinism][should_run]") {
	// Single-system-stage branch: the skip happens by not calling tick_all at all.
	std::size_t const entities = 500;
	int const ticks = 65;

	uint64_t baseline = 0;
	bool first = true;
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		World world;
		world.set_ecs_worker_count(wc);
		for (std::size_t i = 0; i < entities; ++i) {
			world.create_entity(SrA { static_cast<int64_t>(i + 1) });
		}
		world.register_system<SrThreadedGated>();
		Date today { 1836, 1, 1 };
		for (int t = 0; t < ticks; ++t) {
			world.tick_systems(today);
			++today;
		}
		uint64_t const result = world_checksum(world);
		if (first) {
			baseline = result;
			first = false;
		}
		CHECK(result == baseline);
	}
}

// === (d) schedule stability — predicates and skip patterns never touch the schedule ===

TEST_CASE("schedule_hash and stage layout are constant across ticks with varying skip patterns",
          "[ecs][scheduler][should_run][Hash]") {
	World world;
	for (std::size_t i = 0; i < 50; ++i) {
		world.create_entity(SrA { 1 }, SrB { 1 }, SrC { 1 });
	}
	world.register_system<SrThreadedGated>();
	world.register_system<SrThreadedRun>();
	world.register_system<SrSerialRun>();

	uint64_t const h0 = world.schedule_hash();
	std::size_t const stages0 = world.debug_stage_count();
	CHECK(h0 != 0);

	Date today { 1836, 1, 1 };
	for (int t = 0; t < 65; ++t) {
		world.tick_systems(today); // gated system runs Jan 1 / Feb 1 / Mar 1, skips otherwise
		++today;
		CHECK(world.schedule_hash() == h0);
		CHECK(world.debug_stage_count() == stages0);
	}
}

TEST_CASE("A predicated system occupies the same stage layout as its predicate-free twin",
          "[ecs][scheduler][should_run][Hash]") {
	// schedule_hash folds (stage_idx, type_id), and the predicate is part of the type — so
	// two DIFFERENT types can never compare hash-equal. The schedule-invariance claim is
	// about layout: predicate presence must not move a system between stages or change the
	// stage count. Asserted via the layout debug accessors on structurally identical worlds.
	World with_pred;
	with_pred.register_system<SrThreadedGated>();
	with_pred.register_system<SrThreadedRun>();
	with_pred.register_system<SrSerialRun>();

	World without_pred;
	without_pred.register_system<SrThreadedTwin>(); // identical access, no should_run
	without_pred.register_system<SrThreadedRun>();
	without_pred.register_system<SrSerialRun>();

	CHECK(with_pred.debug_stage_count() == without_pred.debug_stage_count());
	CHECK(with_pred.debug_stage_index_of(system_type_id_of<SrThreadedGated>())
		== without_pred.debug_stage_index_of(system_type_id_of<SrThreadedTwin>()));
	CHECK(with_pred.debug_stage_index_of(system_type_id_of<SrThreadedRun>())
		== without_pred.debug_stage_index_of(system_type_id_of<SrThreadedRun>()));
	CHECK(with_pred.debug_stage_index_of(system_type_id_of<SrSerialRun>())
		== without_pred.debug_stage_index_of(system_type_id_of<SrSerialRun>()));
}

// === (e) should_run gating ≡ in-body early-out ===

namespace {
	template<typename SystemT>
	uint64_t run_monthly_and_checksum(std::size_t entity_count, int tick_count) {
		World world;
		for (std::size_t i = 0; i < entity_count; ++i) {
			world.create_entity(SrA { static_cast<int64_t>(i + 1) });
		}
		world.register_system<SystemT>();
		Date today { 1836, 1, 1 };
		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(today);
			++today;
		}
		return world_checksum(world);
	}
}

TEST_CASE("is_month_start gating via should_run equals an in-body early-out",
          "[ecs][determinism][should_run]") {
	std::size_t const entities = 300;
	int const ticks = 65; // 1836-01-01 .. 1836-03-05 — three month starts hit

	CHECK(run_monthly_and_checksum<SrMonthlyGated>(entities, ticks)
		== run_monthly_and_checksum<SrMonthlyEarlyOut>(entities, ticks));
	CHECK(run_monthly_and_checksum<SrMonthlyGatedThreaded>(entities, ticks)
		== run_monthly_and_checksum<SrMonthlyEarlyOutThreaded>(entities, ticks));
	// Gated serial ≡ gated threaded ≡ early-out serial — all four agree.
	CHECK(run_monthly_and_checksum<SrMonthlyGated>(entities, ticks)
		== run_monthly_and_checksum<SrMonthlyGatedThreaded>(entities, ticks));
}
