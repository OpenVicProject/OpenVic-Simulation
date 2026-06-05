#include "openvic-simulation/ecs/ChunkSystem.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/QueryFilter.hpp"
#include "openvic-simulation/ecs/SystemAccess.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === System-level archetype exclusion (`using Filters = ecs::Filter<ecs::Without<C>>`). ===
// Behaviour, access semantics, and edge cases for the serial / threaded / chunk dispatch paths.
// The worker-count-invariance determinism gate lives in SystemFiltersWorkerCountInvariance.cpp.

namespace {
	struct SfCore { int64_t v = 0; };
	struct SfOther { int64_t v = 0; };
	struct SfDead {}; // tag: "logically dead" — the component systems below exclude
	struct SfNever {}; // never instantiated — for the "exclude matches everything" edge
}
ECS_COMPONENT(SfCore, "test_SystemFilters::Core")
ECS_COMPONENT(SfOther, "test_SystemFilters::Other")
ECS_COMPONENT(SfDead, "test_SystemFilters::Dead")
ECS_COMPONENT(SfNever, "test_SystemFilters::Never")

namespace {
	// Set before a tick to capture which entities a with-entity system visited.
	std::vector<uint64_t>* g_sf_visited_ids = nullptr;

	// No Filters alias → unfiltered (baseline for the system_filters_t default).
	struct SfPlainSystem : System<SfPlainSystem> {
		void tick(TickContext const& /*ctx*/, SfCore& c) {
			c.v += 1;
		}
	};

	// Excludes SfDead; writes SfCore.
	struct SfExcludeDeadSystem : System<SfExcludeDeadSystem> {
		using Filters = Filter<Without<SfDead>>;
		void tick(TickContext const& /*ctx*/, SfCore& c) {
			c.v += 1;
		}
	};

	// Excludes SfDead; takes EntityID — exercises the with-entity dispatch path.
	struct SfExcludeDeadWithEntity : System<SfExcludeDeadWithEntity> {
		using Filters = Filter<Without<SfDead>>;
		void tick(TickContext const& /*ctx*/, EntityID eid, SfCore& /*c*/) {
			if (g_sf_visited_ids != nullptr) {
				g_sf_visited_ids->push_back(eid.to_uint64());
			}
		}
	};

	// Excludes a component the test never instantiates → should match every archetype.
	struct SfExcludeNeverSystem : System<SfExcludeNeverSystem> {
		using Filters = Filter<Without<SfNever>>;
		void tick(TickContext const& /*ctx*/, SfCore& c) {
			c.v += 1;
		}
	};

	// Threaded, excludes SfDead.
	struct SfExcludeDeadThreaded : SystemThreaded<SfExcludeDeadThreaded> {
		using Filters = Filter<Without<SfDead>>;
		void tick(TickContext const& /*ctx*/, SfCore& c) {
			c.v += 1;
		}
	};

	// ChunkSystem, excludes SfDead.
	struct SfChunkExcludeDead : ChunkSystem<SfChunkExcludeDead, SfCore> {
		using Filters = Filter<Without<SfDead>>;
		void tick_chunk(ChunkView<SfCore> view, TickContext const& /*ctx*/) {
			SfCore* core = view.template array<SfCore>();
			for (std::size_t i = 0; i < view.count(); ++i) {
				core[i].v += 1;
			}
		}
	};
}
ECS_SYSTEM(SfPlainSystem)
ECS_SYSTEM(SfExcludeDeadSystem)
ECS_SYSTEM(SfExcludeDeadWithEntity)
ECS_SYSTEM(SfExcludeNeverSystem)
ECS_SYSTEM(SfExcludeDeadThreaded)
ECS_SYSTEM(SfChunkExcludeDead)

namespace {
	int64_t core_value(World& world, EntityID id) {
		SfCore const* c = world.get_component<SfCore>(id);
		return c != nullptr ? c->v : -1; // -1 sentinel: SfCore missing (never expected here)
	}
}

// === Filter<> / system_filters_t extraction (unit) ===

TEST_CASE("Filter::exclude_ids extracts, sorts, and dedups component ids", "[ecs][SystemFilters]") {
	std::vector<component_type_id_t> const single = Filter<Without<SfDead>>::exclude_ids();
	REQUIRE(single.size() == 1u);
	CHECK(single[0] == component_type_id_of<SfDead>());

	std::vector<component_type_id_t> const none = Filter<>::exclude_ids();
	CHECK(none.empty());

	std::vector<component_type_id_t> const many =
		Filter<Without<SfOther>, Without<SfDead>, Without<SfOther>>::exclude_ids();
	REQUIRE(many.size() == 2u); // SfOther duplicate collapsed
	CHECK(std::is_sorted(many.begin(), many.end()));
	CHECK(std::find(many.begin(), many.end(), component_type_id_of<SfDead>()) != many.end());
	CHECK(std::find(many.begin(), many.end(), component_type_id_of<SfOther>()) != many.end());
}

TEST_CASE("system_filters_t defaults to empty; compute_tick_query_exclude_ids reflects Filters",
          "[ecs][SystemFilters]") {
	CHECK(system_filters_t<SfPlainSystem>::exclude_ids().empty());
	CHECK(SfPlainSystem::compute_tick_query_exclude_ids().empty());

	std::vector<component_type_id_t> const ex = SfExcludeDeadSystem::compute_tick_query_exclude_ids();
	REQUIRE(ex.size() == 1u);
	CHECK(ex[0] == component_type_id_of<SfDead>());

	// The filter must NOT alter the require set — it is still just the tick pack (SfCore).
	std::vector<component_type_id_t> const req = SfExcludeDeadSystem::compute_tick_query_require_ids();
	REQUIRE(req.size() == 1u);
	CHECK(req[0] == component_type_id_of<SfCore>());
}

TEST_CASE("Without filter adds no access; the written component keeps Write access",
          "[ecs][SystemFilters]") {
	std::array<ComponentAccess, 1> const access = SfExcludeDeadSystem::declared_access();
	CHECK(access[0].component_id == component_type_id_of<SfCore>());
	CHECK(access[0].mode == AccessMode::Write);
	// The excluded component is a structural filter only — it must never appear as access (so a
	// system excluding C does not conflict with a writer of C in the scheduler).
	for (ComponentAccess const& a : access) {
		CHECK(a.component_id != component_type_id_of<SfDead>());
	}
}

// === Serial behaviour ===

TEST_CASE("System Without filter skips archetypes carrying the excluded component",
          "[ecs][SystemFilters]") {
	World world;
	EntityID const a = world.create_entity(SfCore { 0 });
	EntityID const b = world.create_entity(SfCore { 0 }, SfDead {});
	EntityID const c = world.create_entity(SfCore { 0 }, SfOther { 9 });
	EntityID const d = world.create_entity(SfCore { 0 }, SfDead {}, SfOther { 9 });

	world.register_system<SfExcludeDeadSystem>();
	world.tick_systems(Date {});

	CHECK(core_value(world, a) == 1); // {Core} — visited
	CHECK(core_value(world, b) == 0); // {Core, Dead} — skipped
	CHECK(core_value(world, c) == 1); // {Core, Other} — visited
	CHECK(core_value(world, d) == 0); // {Core, Dead, Other} — skipped
}

TEST_CASE("With-entity dispatch respects the Without filter", "[ecs][SystemFilters]") {
	World world;
	EntityID const a = world.create_entity(SfCore { 0 });
	EntityID const b = world.create_entity(SfCore { 0 }, SfDead {});
	EntityID const c = world.create_entity(SfCore { 0 }, SfOther { 9 });

	std::vector<uint64_t> visited;
	g_sf_visited_ids = &visited;
	world.register_system<SfExcludeDeadWithEntity>();
	world.tick_systems(Date {});
	g_sf_visited_ids = nullptr;

	CHECK(visited.size() == 2u);
	CHECK(std::find(visited.begin(), visited.end(), a.to_uint64()) != visited.end());
	CHECK(std::find(visited.begin(), visited.end(), c.to_uint64()) != visited.end());
	CHECK(std::find(visited.begin(), visited.end(), b.to_uint64()) == visited.end());
}

TEST_CASE("ChunkSystem Without filter skips excluded archetypes", "[ecs][SystemFilters][ChunkSystem]") {
	World world;
	EntityID const a = world.create_entity(SfCore { 0 });
	EntityID const b = world.create_entity(SfCore { 0 }, SfDead {});
	EntityID const c = world.create_entity(SfCore { 0 }, SfOther { 1 });

	world.register_system<SfChunkExcludeDead>();
	world.tick_systems(Date {});

	CHECK(core_value(world, a) == 1);
	CHECK(core_value(world, b) == 0);
	CHECK(core_value(world, c) == 1);
}

// === Threaded behaviour (single-system stage → dispatch_threaded → collect_matching_chunks) ===

TEST_CASE("SystemThreaded single-system stage respects the Without filter",
          "[ecs][SystemFilters][threaded]") {
	World world;
	world.set_ecs_worker_count(4);

	std::vector<EntityID> live;
	std::vector<EntityID> dead;
	for (int i = 0; i < 200; ++i) {
		live.push_back(world.create_entity(SfCore { 0 }));
		dead.push_back(world.create_entity(SfCore { 0 }, SfDead {}));
	}

	world.register_system<SfExcludeDeadThreaded>();
	world.tick_systems(Date {});

	for (EntityID const& id : live) {
		CHECK(core_value(world, id) == 1);
	}
	for (EntityID const& id : dead) {
		CHECK(core_value(world, id) == 0);
	}
}

// === Edge cases ===

TEST_CASE("Without a never-instantiated component matches every archetype",
          "[ecs][SystemFilters][edge]") {
	World world;
	EntityID const a = world.create_entity(SfCore { 0 });
	EntityID const b = world.create_entity(SfCore { 0 }, SfDead {});
	EntityID const c = world.create_entity(SfCore { 0 }, SfOther { 0 });

	world.register_system<SfExcludeNeverSystem>(); // Without<SfNever>, never present
	world.tick_systems(Date {});

	CHECK(core_value(world, a) == 1);
	CHECK(core_value(world, b) == 1);
	CHECK(core_value(world, c) == 1);
}

TEST_CASE("Without filter excluding every matched archetype visits nothing",
          "[ecs][SystemFilters][edge]") {
	World world;
	EntityID const b1 = world.create_entity(SfCore { 0 }, SfDead {});
	EntityID const b2 = world.create_entity(SfCore { 0 }, SfDead {}, SfOther { 0 });

	world.register_system<SfExcludeDeadSystem>();
	world.tick_systems(Date {});

	CHECK(core_value(world, b1) == 0);
	CHECK(core_value(world, b2) == 0);
}

TEST_CASE("Filtered query re-resolves when a new matching archetype appears (serial)",
          "[ecs][SystemFilters][edge][cache]") {
	World world;
	EntityID const a = world.create_entity(SfCore { 0 });
	EntityID const b = world.create_entity(SfCore { 0 }, SfDead {});

	world.register_system<SfExcludeDeadSystem>();
	world.tick_systems(Date {});
	CHECK(core_value(world, a) == 1);
	CHECK(core_value(world, b) == 0);

	// A brand-new archetype {Core, Other} appears between ticks — the exclude<Dead> query must
	// re-resolve (archetype_epoch bumped) and pick it up.
	EntityID const c = world.create_entity(SfCore { 0 }, SfOther { 0 });
	world.tick_systems(Date {});
	CHECK(core_value(world, a) == 2);
	CHECK(core_value(world, b) == 0);
	CHECK(core_value(world, c) == 1);
}

TEST_CASE("Filtered query re-resolves for a new archetype on the threaded path",
          "[ecs][SystemFilters][edge][cache][threaded]") {
	World world;
	world.set_ecs_worker_count(4);
	EntityID const a = world.create_entity(SfCore { 0 });
	EntityID const b = world.create_entity(SfCore { 0 }, SfDead {});

	world.register_system<SfExcludeDeadThreaded>();
	world.tick_systems(Date {});
	CHECK(core_value(world, a) == 1);
	CHECK(core_value(world, b) == 0);

	EntityID const c = world.create_entity(SfCore { 0 }, SfOther { 0 });
	world.tick_systems(Date {});
	CHECK(core_value(world, a) == 2);
	CHECK(core_value(world, b) == 0);
	CHECK(core_value(world, c) == 1);
}
