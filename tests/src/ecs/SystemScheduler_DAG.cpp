#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <array>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct SchedulerTagA { int n = 0; };
	struct SchedulerTagB { int n = 0; };
	struct SchedulerTagC { int n = 0; };
}
ECS_COMPONENT(SchedulerTagA, "test_SystemScheduler_DAG::TagA")
ECS_COMPONENT(SchedulerTagB, "test_SystemScheduler_DAG::TagB")
ECS_COMPONENT(SchedulerTagC, "test_SystemScheduler_DAG::TagC")

namespace {
	// Order-recorder systems — push their id into a static log on each tick.
	// They each touch a DIFFERENT tag component, so they are NOT in conflict
	// — registration / declared deps determine ordering.
	std::vector<int>* g_scheduler_dag_log = nullptr;

	struct SchedDagSystemA;
	struct SchedDagSystemB;
	struct SchedDagSystemC;
}
// A system's declared_run_after / declared_run_before calls system_type_id_of<Other>(),
// which needs SystemName<Other> COMPLETE at that call's point of instantiation. GCC anchors
// that point right after the referencing (non-template) class, so every ECS_SYSTEM identity
// must be declared before the systems that reference it — not batched at the bottom (which
// only happens to compile under MSVC's end-of-translation-unit instantiation). Forward-declare
// the systems, specialise their names, then define them.
ECS_SYSTEM(SchedDagSystemA)
ECS_SYSTEM(SchedDagSystemB)
ECS_SYSTEM(SchedDagSystemC)

namespace {
	struct SchedDagSystemA : System<SchedDagSystemA> {
		void tick(TickContext const& /*ctx*/, SchedulerTagA& /*t*/) {
			if (g_scheduler_dag_log) {
				g_scheduler_dag_log->push_back(1);
			}
		}
	};

	struct SchedDagSystemB : System<SchedDagSystemB> {
		// B explicitly runs after A.
		static constexpr auto declared_run_after() {
			return std::array<system_type_id_t, 1> {
				system_type_id_of<SchedDagSystemA>()
			};
		}
		void tick(TickContext const& /*ctx*/, SchedulerTagB& /*t*/) {
			if (g_scheduler_dag_log) {
				g_scheduler_dag_log->push_back(2);
			}
		}
	};

	struct SchedDagSystemC : System<SchedDagSystemC> {
		// C explicitly runs before A.
		static constexpr auto declared_run_before() {
			return std::array<system_type_id_t, 1> {
				system_type_id_of<SchedDagSystemA>()
			};
		}
		void tick(TickContext const& /*ctx*/, SchedulerTagC& /*t*/) {
			if (g_scheduler_dag_log) {
				g_scheduler_dag_log->push_back(3);
			}
		}
	};
}

TEST_CASE("Scheduler honours run_after / run_before ordering", "[ecs][SystemScheduler]") {
	std::vector<int> log;
	g_scheduler_dag_log = &log;

	World world;
	world.create_entity(SchedulerTagA {});
	world.create_entity(SchedulerTagB {});
	world.create_entity(SchedulerTagC {});

	// Register out of declared order — scheduler must still respect deps:
	// C runs before A; A runs before B. Expected: [3, 1, 2].
	world.register_system<SchedDagSystemB>();
	world.register_system<SchedDagSystemA>();
	world.register_system<SchedDagSystemC>();

	world.tick_systems(Date {});

	REQUIRE(log.size() == 3u);
	CHECK(log[0] == 3); // C
	CHECK(log[1] == 1); // A
	CHECK(log[2] == 2); // B

	g_scheduler_dag_log = nullptr;
}

TEST_CASE("Scheduler order is identical regardless of registration order",
          "[ecs][SystemScheduler][determinism]") {
	std::vector<int> log_first;
	std::vector<int> log_second;

	for (int run = 0; run < 2; ++run) {
		std::vector<int> log;
		g_scheduler_dag_log = &log;

		World world;
		world.create_entity(SchedulerTagA {});
		world.create_entity(SchedulerTagB {});
		world.create_entity(SchedulerTagC {});

		if (run == 0) {
			world.register_system<SchedDagSystemB>();
			world.register_system<SchedDagSystemC>();
			world.register_system<SchedDagSystemA>();
		} else {
			world.register_system<SchedDagSystemA>();
			world.register_system<SchedDagSystemB>();
			world.register_system<SchedDagSystemC>();
		}
		world.tick_systems(Date {});

		if (run == 0) {
			log_first = log;
		} else {
			log_second = log;
		}
		g_scheduler_dag_log = nullptr;
	}

	CHECK(log_first == log_second);
}

TEST_CASE("schedule_hash is non-zero and stable across registration orders",
          "[ecs][SystemScheduler][Hash][determinism]") {
	uint64_t h_first = 0;
	uint64_t h_second = 0;

	{
		World w;
		w.register_system<SchedDagSystemA>();
		w.register_system<SchedDagSystemB>();
		w.register_system<SchedDagSystemC>();
		h_first = w.schedule_hash();
	}
	{
		World w;
		w.register_system<SchedDagSystemC>();
		w.register_system<SchedDagSystemB>();
		w.register_system<SchedDagSystemA>();
		h_second = w.schedule_hash();
	}

	CHECK(h_first != 0);
	CHECK(h_first == h_second);
}

TEST_CASE("schedule_hash differs when systems differ", "[ecs][SystemScheduler][Hash]") {
	World w1;
	w1.register_system<SchedDagSystemA>();
	w1.register_system<SchedDagSystemB>();

	World w2;
	w2.register_system<SchedDagSystemA>();
	w2.register_system<SchedDagSystemC>();

	CHECK(w1.schedule_hash() != w2.schedule_hash());
}
