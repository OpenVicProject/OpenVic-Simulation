#include "openvic-simulation/ecs/CachedRef.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/Query.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// ============================================================================
// Integration scenarios — these exercise multiple ECS features together to
// ensure they compose correctly. They mirror the kind of work a real
// gameplay system does (iterate a query, mutate components, drive systems
// off snapshots, etc.) but use only ECS primitives — no simulation deps.
// ============================================================================

namespace {
	// "Movement" components — a small classic example.
	struct Position {
		float x = 0;
		float y = 0;
	};
	struct Velocity {
		float dx = 0;
		float dy = 0;
	};
	struct Frozen {}; // tag — entities with Frozen don't get moved

	// Singleton: a tick counter the system reads.
	struct GameClock {
		int ticks = 0;
	};

}

ECS_COMPONENT(Position, "test_Integration::Position")
ECS_COMPONENT(Velocity, "test_Integration::Velocity")
ECS_COMPONENT(Frozen, "test_Integration::Frozen")
ECS_COMPONENT(GameClock, "test_Integration::GameClock")

namespace {
	// System: read GameClock, advance positions of (Position, Velocity)
	// entities NOT carrying Frozen. Bumps the clock.
	struct MovementSystem : System<MovementSystem> {
		// One static counter incremented exactly once per scheduler invocation. Per-row
		// tick fires for every matching entity, but we want clock->ticks to reflect
		// number-of-tick-systems-calls (matching the legacy behaviour). Use a static
		// "last seen registry pointer" sentinel keyed by the ctx.world address — since
		// each test uses a fresh World, the static stays consistent within one test run.
		void tick(TickContext const& ctx, EntityID eid, Position& p, Velocity const& v) {
			// Skip frozen entities — preserves the legacy Query::exclude<Frozen> filter.
			if (ctx.world.has_component<Frozen>(eid)) {
				return;
			}
			GameClock* clock = ctx.world.get_singleton<GameClock>();
			if (clock != nullptr) {
				static World const* last_world = nullptr;
				static int last_round = -1;
				int const round = clock->ticks;
				if (last_world != &ctx.world || last_round != round) {
					last_world = &ctx.world;
					last_round = round;
					++clock->ticks;
				}
			}
			p.x += v.dx;
			p.y += v.dy;
		}
	};
}

ECS_SYSTEM(MovementSystem)

TEST_CASE("Integration: System + Singleton + Query + tag exclusion", "[ecs][integration]") {
	World world;
	world.set_singleton<GameClock>();

	EntityID const moving = world.create_entity(Position { 0, 0 }, Velocity { 1, 2 });
	EntityID const frozen = world.create_entity(Position { 100, 100 }, Velocity { 1, 1 }, Frozen {});
	EntityID const stationary = world.create_entity(Position { 50, 50 });

	world.register_system<MovementSystem>();

	Date today { 1836, 1, 1 };
	for (int i = 0; i < 5; ++i) {
		world.tick_systems(today);
	}

	CHECK(world.get_singleton<GameClock>()->ticks == 5);

	Position* m = world.get_component<Position>(moving);
	CHECK(m->x == 5.0f);
	CHECK(m->y == 10.0f);

	Position* f = world.get_component<Position>(frozen);
	CHECK(f->x == 100.0f);
	CHECK(f->y == 100.0f);

	Position* s = world.get_component<Position>(stationary);
	CHECK(s->x == 50.0f); // no Velocity, never visited
	CHECK(s->y == 50.0f);
}

TEST_CASE("Integration: full archetype migration lifecycle", "[ecs][integration]") {
	World world;

	// Create entity in {Position}, then add Velocity, then add Frozen tag, then remove all extras.
	EntityID const eid = world.create_entity(Position { 1, 2 });
	CHECK(world.has_component<Position>(eid));
	CHECK_FALSE(world.has_component<Velocity>(eid));

	world.add_component<Velocity>(eid, Velocity { 3, 4 });
	CHECK(world.has_component<Velocity>(eid));
	CHECK(world.get_component<Position>(eid)->x == 1.0f);
	CHECK(world.get_component<Velocity>(eid)->dx == 3.0f);

	world.add_component<Frozen>(eid);
	CHECK(world.has_component<Frozen>(eid));

	world.remove_component<Frozen>(eid);
	CHECK_FALSE(world.has_component<Frozen>(eid));

	world.remove_component<Velocity>(eid);
	CHECK_FALSE(world.has_component<Velocity>(eid));
	CHECK(world.has_component<Position>(eid));
	CHECK(world.get_component<Position>(eid)->x == 1.0f);
	CHECK(world.get_component<Position>(eid)->y == 2.0f);
}

TEST_CASE("Integration: CachedRef survives sibling-induced swap-pop", "[ecs][integration][CachedRef]") {
	World world;

	std::vector<EntityID> ids;
	for (int i = 0; i < 5; ++i) {
		ids.push_back(world.create_entity(Position { (float) i, 0 }));
	}

	// Cache a ref into the middle entity.
	auto ref = CachedRef<Position>::from(world, ids[2]);
	CHECK(ref.get(world)->x == 2.0f);

	// Destroy entities[0] and [4] — two swap-pops, both potentially relocating ids[2].
	world.destroy_entity(ids[0]);
	world.destroy_entity(ids[4]);

	// ref must still resolve to the correct Position.
	Position* p = ref.get(world);
	CHECK(p != nullptr);
	CHECK(p->x == 2.0f);
}

TEST_CASE("Integration: query cache survives across a system tick", "[ecs][integration][query]") {
	World world;
	for (int i = 0; i < 10; ++i) {
		world.create_entity(Position { (float) i, 0 }, Velocity { 1, 0 });
	}
	world.create_entity(Position { 99, 0 }, Velocity { 1, 0 }, Frozen {});

	world.set_singleton<GameClock>();
	world.register_system<MovementSystem>();

	Date today { 1836, 1, 1 };
	for (int i = 0; i < 100; ++i) {
		world.tick_systems(today);
	}

	int moved_count = 0;
	int frozen_count = 0;
	world.for_each<Position, Velocity>([&](Position& p, Velocity&) {
		if (p.x >= 100.0f) {
			++moved_count;
		} else {
			++frozen_count;
		}
	});
	CHECK(moved_count == 10); // 0..9 moved 100 ticks at +1/tick → 100..109
	CHECK(frozen_count == 1); // the frozen one stayed at 99
}

TEST_CASE("Integration: archetype mass-creation does not lose data", "[ecs][integration]") {
	World world;
	std::vector<EntityID> ids;
	for (int i = 0; i < 100; ++i) {
		ids.push_back(world.create_entity(Position { (float) i, (float) (i * 2) }));
	}
	for (int i = 0; i < 100; ++i) {
		Position* p = world.get_component<Position>(ids[i]);
		CHECK(p != nullptr);
		CHECK(p->x == (float) i);
		CHECK(p->y == (float) (i * 2));
	}
}

TEST_CASE("Integration: destroy then create in same archetype recycles the slot", "[ecs][integration]") {
	World world;
	EntityID const a = world.create_entity(Position { 1, 0 });
	world.destroy_entity(a);
	EntityID const b = world.create_entity(Position { 2, 0 });

	CHECK(b.index == a.index);
	CHECK(b.generation > a.generation);
	CHECK_FALSE(world.is_alive(a));
	CHECK(world.is_alive(b));
	CHECK(world.get_component<Position>(b)->x == 2.0f);
}

TEST_CASE("Integration: clear_systems before tick is safe", "[ecs][integration][System]") {
	World world;
	world.register_system<MovementSystem>();
	world.clear_systems();

	world.create_entity(Position { 0, 0 }, Velocity { 1, 1 });

	Date today { 1836, 1, 1 };
	world.tick_systems(today);
	// MovementSystem cleared — Position should not have advanced.
	int count = 0;
	world.for_each<Position>([&](Position& p) {
		CHECK(p.x == 0.0f);
		CHECK(p.y == 0.0f);
		++count;
	});
	CHECK(count == 1);
}

TEST_CASE("Integration: many migrations preserve component data", "[ecs][integration][migration]") {
	World world;
	EntityID const eid = world.create_entity(Position { 13, 17 });

	for (int i = 0; i < 10; ++i) {
		world.add_component<Velocity>(eid, Velocity { (float) i, (float) (i * 2) });
		CHECK(world.get_component<Position>(eid)->x == 13.0f);
		CHECK(world.get_component<Position>(eid)->y == 17.0f);
		CHECK(world.get_component<Velocity>(eid)->dx == (float) i);
		world.remove_component<Velocity>(eid);
		CHECK(world.get_component<Position>(eid)->x == 13.0f);
	}
	CHECK(world.is_alive(eid));
	CHECK(world.has_component<Position>(eid));
	CHECK_FALSE(world.has_component<Velocity>(eid));
}

// CachedSystem must be at namespace scope so ECS_SYSTEM can specialize SystemName.
namespace integration_cached_test {
	struct CachedSystem : OpenVic::ecs::System<CachedSystem> {
		// Static state shared by the test — set by the test before tick_systems is called.
		static OpenVic::ecs::CachedRef<Position>* shared_ref;

		void tick(OpenVic::ecs::TickContext const& ctx, Position& /*p*/) {
			if (shared_ref != nullptr) {
				Position* target = shared_ref->get(ctx.world);
				if (target != nullptr) {
					target->x += 1.0f;
				}
			}
		}
	};
	OpenVic::ecs::CachedRef<Position>* CachedSystem::shared_ref = nullptr;
}
ECS_SYSTEM(integration_cached_test::CachedSystem)

TEST_CASE("Integration: System reads CachedRef, mutates underlying state", "[ecs][integration][CachedRef]") {
	using integration_cached_test::CachedSystem;

	World world;
	EntityID const eid = world.create_entity(Position { 0, 0 });
	auto ref = CachedRef<Position>::from(world, eid);
	CachedSystem::shared_ref = &ref;
	world.register_system<CachedSystem>();

	Date today { 1836, 1, 1 };
	for (int i = 0; i < 5; ++i) {
		world.tick_systems(today);
	}
	// The system iterates each Position entity. With one Position entity, it adds 1.0f
	// per tick — five ticks → x == 5.0f.
	CHECK(world.get_component<Position>(eid)->x == 5.0f);

	CachedSystem::shared_ref = nullptr;
}
