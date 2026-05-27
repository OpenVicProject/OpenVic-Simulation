#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct GuardTag { int n = 0; };
	struct GuardTagB { int m = 0; };
}
ECS_COMPONENT(GuardTag, "test_InTickGuard::GuardTag")
ECS_COMPONENT(GuardTagB, "test_InTickGuard::GuardTagB")

namespace {
	// A system that misuses the World API by calling `world.add_component` directly. The
	// in_tick_ guard must intercept this, log an error, and turn it into a no-op
	// (returning nullptr) so the test below can observe non-effect.
	struct MisbehavingSystem : System<MisbehavingSystem> {
		void tick(TickContext const& ctx, EntityID eid, GuardTag&) {
			// Forbidden! World::add_component during a tick must early-return.
			ctx.world.add_component<GuardTagB>(eid);
		}
	};

	// A system that uses the proper `ctx.cmd` path — should succeed.
	struct WellBehavedSystem : System<WellBehavedSystem> {
		void tick(TickContext const& ctx, EntityID eid, GuardTag&) {
			ctx.cmd.add_component<GuardTagB>(eid);
		}
	};
}
ECS_SYSTEM(MisbehavingSystem)
ECS_SYSTEM(WellBehavedSystem)

TEST_CASE("World::add_component during tick is rejected", "[ecs][InTickMutationGuard]") {
	World world;
	EntityID const eid = world.create_entity(GuardTag {});
	world.register_system<MisbehavingSystem>();
	world.tick_systems(Date {});
	// The misbehaving call to world.add_component<GuardTagB> should have been rejected.
	CHECK_FALSE(world.has_component<GuardTagB>(eid));
}

TEST_CASE("ctx.cmd.add_component succeeds and applies at stage barrier",
          "[ecs][InTickMutationGuard]") {
	World world;
	EntityID const eid = world.create_entity(GuardTag {});
	world.register_system<WellBehavedSystem>();
	world.tick_systems(Date {});
	// After the stage barrier, the deferred add_component should have applied.
	CHECK(world.has_component<GuardTagB>(eid));
}
