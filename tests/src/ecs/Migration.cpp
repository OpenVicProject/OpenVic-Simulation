#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <string>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct MA {
		int v = 0;
	};
	struct MB {
		int w = 0;
	};
	struct MC {
		std::string s;
	};
	struct MTag {};
	struct MTag2 {};
}

ECS_COMPONENT(MA, "test_Migration::MA")
ECS_COMPONENT(MB, "test_Migration::MB")
ECS_COMPONENT(MC, "test_Migration::MC")
ECS_COMPONENT(MTag, "test_Migration::MTag")
ECS_COMPONENT(MTag2, "test_Migration::MTag2")

TEST_CASE("add_component on dead entity returns nullptr", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	world.destroy_entity(eid);

	MA* p = world.add_component<MA>(eid, MA { 2 });
	CHECK(p == nullptr);
	CHECK(world.add_component<MB>(eid, MB { 3 }) == nullptr);
}

TEST_CASE("add_component on invalid EntityID returns nullptr", "[ecs][World][migration]") {
	World world;
	CHECK(world.add_component<MA>(INVALID_ENTITY_ID, MA { 0 }) == nullptr);
	CHECK(world.add_component<MA>(EntityID { 999, 1 }, MA { 0 }) == nullptr);
}

TEST_CASE("add_component when entity already has C replaces value", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	MA* original = world.get_component<MA>(eid);

	MA* p = world.add_component<MA>(eid, MA { 99 });
	CHECK(p != nullptr);
	CHECK(p->v == 99);
	CHECK(p == original); // same slot, replaced in place
	CHECK(world.get_component<MA>(eid)->v == 99);
}

TEST_CASE("add_component migrates entity to new archetype", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 7 });
	CHECK_FALSE(world.has_component<MB>(eid));

	MB* added = world.add_component<MB>(eid, MB { 13 });
	CHECK(added != nullptr);
	CHECK(added->w == 13);
	CHECK(world.has_component<MA>(eid));
	CHECK(world.has_component<MB>(eid));

	// Original A value preserved.
	CHECK(world.get_component<MA>(eid)->v == 7);
}

TEST_CASE("add_component preserves non-trivial component values during migration", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MC { "hello world" });

	world.add_component<MA>(eid, MA { 42 });
	CHECK(world.get_component<MC>(eid)->s == "hello world");
	CHECK(world.get_component<MA>(eid)->v == 42);
}

TEST_CASE("add_component preserves siblings via swap-pop relocation", "[ecs][World][migration]") {
	World world;
	EntityID const a = world.create_entity(MA { 100 });
	EntityID const b = world.create_entity(MA { 200 });
	EntityID const c = world.create_entity(MA { 300 });

	// Migrate `a` (the first row) to a new archetype. `c` (last row) should be relocated.
	world.add_component<MB>(a, MB { 1 });

	CHECK(world.is_alive(a));
	CHECK(world.is_alive(b));
	CHECK(world.is_alive(c));

	CHECK(world.get_component<MA>(a)->v == 100);
	CHECK(world.get_component<MA>(b)->v == 200);
	CHECK(world.get_component<MA>(c)->v == 300);

	CHECK(world.has_component<MB>(a));
	CHECK_FALSE(world.has_component<MB>(b));
	CHECK_FALSE(world.has_component<MB>(c));
}

TEST_CASE("add_component default-construct overload", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	MB* p = world.add_component<MB>(eid);
	CHECK(p != nullptr);
	CHECK(p->w == 0); // default
}

TEST_CASE("add_component returns nullptr for tag types but adds to archetype", "[ecs][World][migration][tag]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	CHECK_FALSE(world.has_component<MTag>(eid));

	MTag* p = world.add_component<MTag>(eid);
	CHECK(p == nullptr); // tag has no data slot
	CHECK(world.has_component<MTag>(eid));
}

TEST_CASE("Multiple sequential add_components work", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });

	world.add_component<MB>(eid, MB { 2 });
	world.add_component<MC>(eid, MC { "x" });
	world.add_component<MTag>(eid);

	CHECK(world.has_component<MA>(eid));
	CHECK(world.has_component<MB>(eid));
	CHECK(world.has_component<MC>(eid));
	CHECK(world.has_component<MTag>(eid));

	CHECK(world.get_component<MA>(eid)->v == 1);
	CHECK(world.get_component<MB>(eid)->w == 2);
	CHECK(world.get_component<MC>(eid)->s == "x");
}

TEST_CASE("remove_component on dead entity returns false", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 }, MB { 2 });
	world.destroy_entity(eid);
	CHECK_FALSE(world.remove_component<MA>(eid));
}

TEST_CASE("remove_component on invalid EntityID returns false", "[ecs][World][migration]") {
	World world;
	CHECK_FALSE(world.remove_component<MA>(INVALID_ENTITY_ID));
	CHECK_FALSE(world.remove_component<MA>(EntityID { 99, 1 }));
}

TEST_CASE("remove_component for missing component returns false", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	CHECK_FALSE(world.remove_component<MB>(eid));
}

TEST_CASE("remove_component for sole component returns false (use destroy_entity)", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	CHECK_FALSE(world.remove_component<MA>(eid));
	CHECK(world.is_alive(eid));
	CHECK(world.has_component<MA>(eid));
}

TEST_CASE("remove_component migrates entity, preserving remaining components", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 5 }, MB { 50 }, MC { "keep" });

	bool removed = world.remove_component<MB>(eid);
	CHECK(removed);
	CHECK(world.has_component<MA>(eid));
	CHECK_FALSE(world.has_component<MB>(eid));
	CHECK(world.has_component<MC>(eid));

	CHECK(world.get_component<MA>(eid)->v == 5);
	CHECK(world.get_component<MC>(eid)->s == "keep");
}

TEST_CASE("remove_component preserves siblings via swap-pop", "[ecs][World][migration]") {
	World world;
	EntityID const a = world.create_entity(MA { 10 }, MB { 100 });
	EntityID const b = world.create_entity(MA { 20 }, MB { 200 });
	EntityID const c = world.create_entity(MA { 30 }, MB { 300 });

	world.remove_component<MB>(a);

	CHECK(world.is_alive(a));
	CHECK(world.is_alive(b));
	CHECK(world.is_alive(c));

	CHECK(world.get_component<MA>(a)->v == 10);
	CHECK(world.get_component<MA>(b)->v == 20);
	CHECK(world.get_component<MA>(c)->v == 30);

	CHECK_FALSE(world.has_component<MB>(a));
	CHECK(world.get_component<MB>(b)->w == 200);
	CHECK(world.get_component<MB>(c)->w == 300);
}

TEST_CASE("remove_component on tag component succeeds", "[ecs][World][migration][tag]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	world.add_component<MTag>(eid);
	CHECK(world.has_component<MTag>(eid));

	bool removed = world.remove_component<MTag>(eid);
	CHECK(removed);
	CHECK_FALSE(world.has_component<MTag>(eid));
	CHECK(world.has_component<MA>(eid));
}

TEST_CASE("add then remove returns entity to original archetype", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });

	world.add_component<MB>(eid, MB { 2 });
	CHECK(world.has_component<MB>(eid));

	world.remove_component<MB>(eid);
	CHECK_FALSE(world.has_component<MB>(eid));
	CHECK(world.has_component<MA>(eid));
	CHECK(world.get_component<MA>(eid)->v == 1);
}

TEST_CASE("EntityID remains valid across migration", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	uint32_t const original_index = eid.index;
	uint32_t const original_gen = eid.generation;

	world.add_component<MB>(eid, MB { 2 });
	CHECK(world.is_alive(eid));
	CHECK(eid.index == original_index);
	CHECK(eid.generation == original_gen);

	world.remove_component<MB>(eid);
	CHECK(world.is_alive(eid));
	CHECK(eid.index == original_index);
	CHECK(eid.generation == original_gen);
}

TEST_CASE("Multiple migrations of the same entity work in sequence", "[ecs][World][migration]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });

	for (int i = 0; i < 5; ++i) {
		world.add_component<MB>(eid, MB { 100 + i });
		CHECK(world.has_component<MB>(eid));
		world.remove_component<MB>(eid);
		CHECK_FALSE(world.has_component<MB>(eid));
	}
	CHECK(world.is_alive(eid));
	CHECK(world.get_component<MA>(eid)->v == 1);
}

TEST_CASE("add_component<Tag> twice on same entity is no-op", "[ecs][World][migration][tag]") {
	World world;
	EntityID const eid = world.create_entity(MA { 1 });
	world.add_component<MTag>(eid);
	world.add_component<MTag>(eid); // already present — no-op
	CHECK(world.has_component<MTag>(eid));
}

TEST_CASE("Migration with a tag in the source archetype works", "[ecs][World][migration][tag]") {
	World world;
	EntityID const eid = world.create_entity(MA { 5 }, MTag {});
	CHECK(world.has_component<MTag>(eid));

	world.add_component<MB>(eid, MB { 7 });
	CHECK(world.has_component<MA>(eid));
	CHECK(world.has_component<MB>(eid));
	CHECK(world.has_component<MTag>(eid));
	CHECK(world.get_component<MA>(eid)->v == 5);
	CHECK(world.get_component<MB>(eid)->w == 7);
}
