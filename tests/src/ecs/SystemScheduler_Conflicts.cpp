#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct ConflictTagX { int n = 0; };
	struct ConflictTagY { int n = 0; };
}
ECS_COMPONENT(ConflictTagX, "test_SystemScheduler_Conflicts::TagX")
ECS_COMPONENT(ConflictTagY, "test_SystemScheduler_Conflicts::TagY")

namespace {
	std::vector<int>* g_conflict_log = nullptr;

	// W/W conflict: WriterA and WriterB both write ConflictTagX. Auto-orienter must
	// pick one before the other deterministically.
	struct ConflictWriterA : System<ConflictWriterA> {
		void tick(TickContext const& /*ctx*/, ConflictTagX& x) {
			x.n += 1;
			if (g_conflict_log) {
				g_conflict_log->push_back(1);
			}
		}
	};

	struct ConflictWriterB : System<ConflictWriterB> {
		void tick(TickContext const& /*ctx*/, ConflictTagX& x) {
			x.n *= 2;
			if (g_conflict_log) {
				g_conflict_log->push_back(2);
			}
		}
	};

	// R/R is fine — should run in the same stage with no conflict edge required.
	struct ConflictReaderA : System<ConflictReaderA> {
		void tick(TickContext const& /*ctx*/, ConflictTagY const& /*y*/) {
			if (g_conflict_log) {
				g_conflict_log->push_back(11);
			}
		}
	};

	struct ConflictReaderB : System<ConflictReaderB> {
		void tick(TickContext const& /*ctx*/, ConflictTagY const& /*y*/) {
			if (g_conflict_log) {
				g_conflict_log->push_back(12);
			}
		}
	};
}
ECS_SYSTEM(ConflictWriterA)
ECS_SYSTEM(ConflictWriterB)
ECS_SYSTEM(ConflictReaderA)
ECS_SYSTEM(ConflictReaderB)

TEST_CASE("Auto-orientation produces deterministic order on W/W conflict",
          "[ecs][SystemScheduler][Conflicts]") {
	// Run the same registration twice with the same systems but different orderings
	// — auto-orientation must pick the same direction both times.
	std::vector<int> log_run1;
	std::vector<int> log_run2;

	{
		std::vector<int> log;
		g_conflict_log = &log;
		World world;
		world.create_entity(ConflictTagX {});
		world.register_system<ConflictWriterA>();
		world.register_system<ConflictWriterB>();
		world.tick_systems(Date {});
		log_run1 = log;
		g_conflict_log = nullptr;
	}
	{
		std::vector<int> log;
		g_conflict_log = &log;
		World world;
		world.create_entity(ConflictTagX {});
		world.register_system<ConflictWriterB>();
		world.register_system<ConflictWriterA>();
		world.tick_systems(Date {});
		log_run2 = log;
		g_conflict_log = nullptr;
	}

	REQUIRE(log_run1.size() == 2u);
	REQUIRE(log_run2.size() == 2u);
	// Both runs must agree on the order (auto-orientation is registration-order-independent).
	CHECK(log_run1 == log_run2);
}

TEST_CASE("Read-only systems may share a stage", "[ecs][SystemScheduler][Conflicts]") {
	World world;
	world.create_entity(ConflictTagY {});
	world.register_system<ConflictReaderA>();
	world.register_system<ConflictReaderB>();

	uint64_t const h1 = world.schedule_hash();

	// Re-register in opposite order — same set of pure-read systems.
	World world2;
	world2.create_entity(ConflictTagY {});
	world2.register_system<ConflictReaderB>();
	world2.register_system<ConflictReaderA>();

	uint64_t const h2 = world2.schedule_hash();

	// With no conflicts, both readers land in the same stage; hash is identical
	// regardless of registration order (depth=0 and id-ordered tiebreaker).
	CHECK(h1 == h2);
}
