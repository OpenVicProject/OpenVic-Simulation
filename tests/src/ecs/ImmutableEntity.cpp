#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct ImmA {
		int v = 0;
	};
	struct ImmB {
		int w = 0;
	};
	struct ImmTickComp {
		int v = 0;
		uint32_t seen_index = 0;
	};
	struct ImmSpawnSource {
		int32_t n = 0;
	};
	struct ImmSpawned {
		int32_t from = 0;
	};
}

ECS_COMPONENT(ImmA, "test_ImmutableEntity::ImmA")
ECS_COMPONENT(ImmB, "test_ImmutableEntity::ImmB")
ECS_COMPONENT(ImmTickComp, "test_ImmutableEntity::ImmTickComp")
ECS_COMPONENT(ImmSpawnSource, "test_ImmutableEntity::ImmSpawnSource")
ECS_COMPONENT(ImmSpawned, "test_ImmutableEntity::ImmSpawned")

// === Compile-time guarantee wall ===
// There is no compile-fail harness in this repo. We express "this call must NOT compile" with
// void_t detection traits rather than `!requires{...}` because MSVC hard-errors (instead of
// SFINAE-ing) on an ill-formed member-function-TEMPLATE call with explicit template args inside a
// requires-expression. The traits below work uniformly on MSVC/Clang/GCC. Each negative is paired
// with the matching positive on the same handle/component so a typo can't make a negative pass
// vacuously. A regression here breaks the build.
namespace {
	template<typename Obj, typename Handle, typename C, typename = void>
	struct can_add_component : std::false_type {};
	template<typename Obj, typename Handle, typename C>
	struct can_add_component<Obj, Handle, C,
		std::void_t<decltype(std::declval<Obj&>().template add_component<C>(std::declval<Handle>()))>>
		: std::true_type {};

	template<typename Obj, typename Handle, typename C, typename = void>
	struct can_remove_component : std::false_type {};
	template<typename Obj, typename Handle, typename C>
	struct can_remove_component<Obj, Handle, C,
		std::void_t<decltype(std::declval<Obj&>().template remove_component<C>(std::declval<Handle>()))>>
		: std::true_type {};

	template<typename Obj, typename Handle, typename C, typename = void>
	struct can_get_component : std::false_type {};
	template<typename Obj, typename Handle, typename C>
	struct can_get_component<Obj, Handle, C,
		std::void_t<decltype(std::declval<Obj&>().template get_component<C>(std::declval<Handle>()))>>
		: std::true_type {};

	// Structural mutation IS reachable on a plain EntityID, but NOT on an ImmutableEntityID
	// (no overload accepts it and there is no implicit conversion). This is the guarantee.
	static_assert(can_add_component<World, EntityID, ImmA>::value);
	static_assert(!can_add_component<World, ImmutableEntityID, ImmA>::value);
	static_assert(can_remove_component<World, EntityID, ImmA>::value);
	static_assert(!can_remove_component<World, ImmutableEntityID, ImmA>::value);

	// The same holds for the deferred CommandBuffer structural ops.
	static_assert(can_add_component<CommandBuffer, EntityID, ImmA>::value);
	static_assert(!can_add_component<CommandBuffer, ImmutableEntityID, ImmA>::value);
	static_assert(can_remove_component<CommandBuffer, EntityID, ImmA>::value);
	static_assert(!can_remove_component<CommandBuffer, ImmutableEntityID, ImmA>::value);

	// Reads/data mutation ARE reachable on an ImmutableEntityID (only the archetype is frozen).
	static_assert(can_get_component<World, EntityID, ImmA>::value);
	static_assert(can_get_component<World, ImmutableEntityID, ImmA>::value);

	// The laundered id (unsafe_mutable_id()) is an EntityID, so it re-enters the mutable path —
	// that is exactly the can_add_component<..., EntityID, ...> case asserted above.

	// Type relationships + the explicit escape hatch.
	static_assert(!std::is_convertible_v<ImmutableEntityID, EntityID>);
	static_assert(!std::is_convertible_v<EntityID, ImmutableEntityID>);
	static_assert(!std::is_same_v<ImmutableEntityID, EntityID>);
	static_assert(std::is_same_v<decltype(std::declval<ImmutableEntityID>().unsafe_mutable_id()), EntityID>);

	// The factory hands back the strong handle, not a plain EntityID.
	static_assert(std::is_same_v<
		decltype(std::declval<World&>().create_immutable_entity(std::declval<ImmA>())), ImmutableEntityID>);
}

TEST_CASE("Immutable compile-time guarantees hold (static_assert wall)", "[ecs][World][immutable][compiletime]") {
	// The static_assert block above is the real test; this case just surfaces it in the report.
	CHECK(true);
}

TEST_CASE("create_immutable_entity is live, carries components, data is mutable", "[ecs][World][immutable]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 7 }, ImmB { 9 });

	CHECK(world.is_alive(e));
	CHECK(world.has_component<ImmA>(e));
	CHECK(world.has_component<ImmB>(e));

	ImmA* a = world.get_component<ImmA>(e);
	REQUIRE(a != nullptr);
	CHECK(a->v == 7);

	// Data mutation through the immutable handle is allowed — only the archetype is frozen.
	a->v = 42;
	CHECK(world.get_component<ImmA>(e)->v == 42);
}

TEST_CASE("unsafe_mutable_id addresses the same entity", "[ecs][World][immutable]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 1 });
	EntityID const m = e.unsafe_mutable_id();

	CHECK(world.is_alive(m));
	CHECK(world.get_component<ImmA>(m) == world.get_component<ImmA>(e));
}

TEST_CASE("add_component on a laundered immutable id is refused (no migration)", "[ecs][World][immutable][backstop]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 5 });
	ImmA* const before = world.get_component<ImmA>(e);

	ImmB* const added = world.add_component<ImmB>(e.unsafe_mutable_id(), ImmB { 3 });

	CHECK(added == nullptr);
	CHECK_FALSE(world.has_component<ImmB>(e));
	// No migration ⇒ the entity stayed in its archetype and its component pointer is unchanged.
	CHECK(world.get_component<ImmA>(e) == before);
	CHECK(world.get_component<ImmA>(e)->v == 5);
}

TEST_CASE("remove_component on a laundered immutable id is refused", "[ecs][World][immutable][backstop]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 5 }, ImmB { 6 });

	CHECK_FALSE(world.remove_component<ImmB>(e.unsafe_mutable_id()));
	CHECK(world.has_component<ImmB>(e));
}

TEST_CASE("structural ops still succeed on a normal mutable entity (backstop is immutability-gated)",
		  "[ecs][World][immutable][backstop]") {
	World world;
	EntityID const e = world.create_entity(ImmA { 5 });

	CHECK(world.add_component<ImmB>(e, ImmB { 3 }) != nullptr);
	CHECK(world.has_component<ImmB>(e));
	CHECK(world.remove_component<ImmB>(e));
	CHECK_FALSE(world.has_component<ImmB>(e));
}

TEST_CASE("CommandBuffer add_component on a laundered immutable id is refused at apply",
		  "[ecs][World][immutable][backstop][cmd]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 5 });

	CommandBuffer cb;
	cb.add_component<ImmB>(e.unsafe_mutable_id(), ImmB { 3 });
	cb.apply(world);

	CHECK_FALSE(world.has_component<ImmB>(e));
	CHECK(world.get_component<ImmA>(e)->v == 5);
}

TEST_CASE("CommandBuffer remove_component on a laundered immutable id is refused at apply",
		  "[ecs][World][immutable][backstop][cmd]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 5 }, ImmB { 6 });

	CommandBuffer cb;
	cb.remove_component<ImmB>(e.unsafe_mutable_id());
	cb.apply(world);

	CHECK(world.has_component<ImmB>(e));
}

TEST_CASE("CommandBuffer::create_immutable_entity (serial) finalises an immutable entity",
		  "[ecs][World][immutable][cmd]") {
	World world;
	CommandBuffer cb;
	ImmutableEntityID const e = cb.create_immutable_entity(world, ImmA { 11 });

	// Reserved-but-unfinalised until apply.
	CHECK_FALSE(world.is_alive(e));

	cb.apply(world);

	CHECK(world.is_alive(e));
	REQUIRE(world.get_component<ImmA>(e) != nullptr);
	CHECK(world.get_component<ImmA>(e)->v == 11);
	// The entity is genuinely immutable: a laundered add is refused at runtime.
	CHECK(world.add_component<ImmB>(e.unsafe_mutable_id(), ImmB { 1 }) == nullptr);
	CHECK_FALSE(world.has_component<ImmB>(e));
}

TEST_CASE("Deferred create_immutable_entity + same-buffer add: create runs, add refused",
		  "[ecs][World][immutable][cmd]") {
	World world;
	CommandBuffer cb;
	ImmutableEntityID const e = cb.create_immutable_entity(world, ImmA { 2 });
	cb.add_component<ImmB>(e.unsafe_mutable_id(), ImmB { 9 });
	cb.apply(world);

	CHECK(world.is_alive(e));
	CHECK(world.has_component<ImmA>(e));
	CHECK_FALSE(world.has_component<ImmB>(e));
}

TEST_CASE("A reused slot comes back mutable after an immutable entity is destroyed",
		  "[ecs][World][immutable][lifecycle]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 1 });
	uint32_t const idx = e.index;

	world.destroy_entity(e);
	CHECK_FALSE(world.is_alive(e));

	// Free-list reuse: the next create takes the same slot index and must be mutable again.
	EntityID const m = world.create_entity(ImmA { 2 });
	REQUIRE(m.index == idx);
	CHECK(world.add_component<ImmB>(m, ImmB { 3 }) != nullptr);
	CHECK(world.has_component<ImmB>(m));
}

TEST_CASE("Stale ImmutableEntityID fails is_alive after slot reuse", "[ecs][World][immutable][lifecycle]") {
	World world;
	ImmutableEntityID const e = world.create_immutable_entity(ImmA { 1 });
	world.destroy_entity(e);
	world.create_entity(ImmA { 2 }); // reuses the slot, bumps its generation

	CHECK_FALSE(world.is_alive(e));
}

TEST_CASE("Immutable entity survives sibling churn (re-fetch, not pointer identity)",
		  "[ecs][World][immutable][stability]") {
	World world;
	ImmutableEntityID const keep = world.create_immutable_entity(ImmA { 100 });

	std::vector<EntityID> siblings;
	for (int i = 0; i < 8; ++i) {
		siblings.push_back(world.create_entity(ImmA { i }));
	}
	// Destroying siblings in the same archetype may swap-pop `keep`'s row to a new address — that
	// is permitted: the guarantee is "never changes archetype from its own structural ops", NOT
	// address pinning. So we assert liveness + data via a RE-FETCHED pointer, not pointer identity.
	for (EntityID s : siblings) {
		world.destroy_entity(s);
	}

	REQUIRE(world.is_alive(keep));
	CHECK(world.get_component<ImmA>(keep)->v == 100);
}

namespace {
	// Serial system whose tick receives an ImmutableEntityID per row. It mutates component DATA
	// (allowed) and records the handle's index so the test can confirm the handle the tick saw
	// matches the entity being iterated. A `ctx.cmd.add_component(eid, ...)` here would not compile.
	struct ImmTickSystem : System<ImmTickSystem> {
		void tick(TickContext const& /*ctx*/, ImmutableEntityID eid, ImmTickComp& c) {
			c.v += 1;
			c.seen_index = eid.index;
		}
	};

	// Threaded counterpart — exercises the immutable-handle wrap in the chunk-parallel dispatch.
	struct ImmTickThreaded : SystemThreaded<ImmTickThreaded> {
		void tick(TickContext const& /*ctx*/, ImmutableEntityID eid, ImmTickComp& c) {
			c.v = c.v * 2 + 1;
			c.seen_index = eid.index;
		}
	};
}
ECS_SYSTEM(ImmTickSystem)
ECS_SYSTEM(ImmTickThreaded)

TEST_CASE("System tick can receive ImmutableEntityID handles (serial)", "[ecs][World][immutable][system]") {
	World world;
	std::vector<EntityID> ids;
	for (int i = 0; i < 5; ++i) {
		ids.push_back(world.create_entity(ImmTickComp { i, 0 }));
	}

	world.register_system<ImmTickSystem>();
	world.tick_systems(Date {});

	for (int i = 0; i < 5; ++i) {
		ImmTickComp const* c = world.get_component<ImmTickComp>(ids[i]);
		REQUIRE(c != nullptr);
		CHECK(c->v == i + 1); // data mutation applied
		CHECK(c->seen_index == ids[i].index); // the handle the tick saw matched the entity
	}
}

TEST_CASE("SystemThreaded tick can receive ImmutableEntityID handles", "[ecs][World][immutable][system]") {
	World world;
	world.set_ecs_worker_count(4);
	std::vector<EntityID> ids;
	for (int i = 0; i < 200; ++i) {
		ids.push_back(world.create_entity(ImmTickComp { i, 0 }));
	}

	world.register_system<ImmTickThreaded>();
	world.tick_systems(Date {});

	for (int i = 0; i < 200; ++i) {
		ImmTickComp const* c = world.get_component<ImmTickComp>(ids[i]);
		REQUIRE(c != nullptr);
		CHECK(c->v == i * 2 + 1);
		CHECK(c->seen_index == ids[i].index);
	}
}

namespace {
	// SystemThreaded that spawns immutable entities from inside a tick via the per-chunk
	// CommandBuffer. Mirrors SystemThreadedSpawn.cpp but uses create_immutable_entity.
	struct ImmSpawnSystem : SystemThreaded<ImmSpawnSystem> {
		void tick(TickContext const& ctx, ImmSpawnSource const& src) {
			for (int32_t i = 0; i < src.n; ++i) {
				ctx.cmd.create_immutable_entity(ctx.world, ImmSpawned { src.n });
			}
		}
	};
}
ECS_SYSTEM(ImmSpawnSystem)

namespace {
	struct SpawnResult {
		std::size_t count = 0;
		std::vector<int32_t> from_in_order;
		std::size_t immutable_count = 0;
	};

	SpawnResult run_immutable_spawn(uint32_t worker_count) {
		World world;
		world.set_ecs_worker_count(worker_count);
		for (int32_t i = 1; i <= 10; ++i) {
			world.create_entity(ImmSpawnSource { i });
		}
		world.register_system<ImmSpawnSystem>();
		world.tick_systems(Date {});

		SpawnResult r;
		std::vector<EntityID> spawned_ids;
		world.for_each_with_entity<ImmSpawned>([&](EntityID e, ImmSpawned& s) {
			r.count += 1;
			r.from_in_order.push_back(s.from);
			spawned_ids.push_back(e);
		});
		// Every spawned entity must be immutable: a laundered add is refused (returns nullptr) and
		// the entity does not migrate.
		for (EntityID e : spawned_ids) {
			if (world.add_component<ImmA>(e, ImmA { 1 }) == nullptr && !world.has_component<ImmA>(e)) {
				r.immutable_count += 1;
			}
		}
		return r;
	}
}

TEST_CASE("SystemThreaded can spawn immutable entities via cmd.create_immutable_entity",
		  "[ecs][World][immutable][cmd][system]") {
	SpawnResult const r = run_immutable_spawn(4);

	std::size_t expected = 0;
	for (int32_t i = 1; i <= 10; ++i) {
		expected += static_cast<std::size_t>(i);
	}
	CHECK(r.count == expected); // 55
	CHECK(r.immutable_count == expected); // all spawned entities are immutable
}

TEST_CASE("Immutable in-system spawn is worker-count invariant",
		  "[ecs][World][immutable][cmd][system][determinism]") {
	SpawnResult const baseline = run_immutable_spawn(1);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		SpawnResult const r = run_immutable_spawn(wc);
		CHECK(r.count == baseline.count);
		CHECK(r.immutable_count == baseline.immutable_count);
		REQUIRE(r.from_in_order.size() == baseline.from_in_order.size());
		for (std::size_t i = 0; i < baseline.from_in_order.size(); ++i) {
			CHECK(r.from_in_order[i] == baseline.from_in_order[i]);
		}
	}
}
