#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

// === EntityID save-stability: identity-layer snapshot/restore ===
// World::snapshot_identity captures the slot table (generations, immutability, free-list
// order); World::restore_identity + restore_entity reconstruct every entity at its original
// (index, generation) in a fresh World, and subsequent allocations continue exactly as in the
// never-saved run. See ECS_SIM_ARCHITECTURE §9 item 3.

namespace {
	struct IdsA {
		int64_t v = 0;
	};
	struct IdsB {
		int64_t w = 0;
	};
}
ECS_COMPONENT(IdsA, "test_IdentitySnapshot::IdsA")
ECS_COMPONENT(IdsB, "test_IdentitySnapshot::IdsB")

TEST_CASE("snapshot/restore: empty world round-trips", "[ecs][identity]") {
	World original;
	WorldIdentitySnapshot snap;
	REQUIRE(original.snapshot_identity(snap));
	CHECK(snap.slots.empty());
	CHECK(snap.free_list.empty());

	World restored;
	REQUIRE(restored.restore_identity(snap));

	// Next allocation in both worlds is the fresh-growth {0, 1}.
	EntityID const a = original.create_entity(IdsA { 1 });
	EntityID const b = restored.create_entity(IdsA { 1 });
	CHECK(a == EntityID { 0, 1 });
	CHECK(b == a);
}

TEST_CASE("live entities reconstruct at original (index, generation); stale handles stay dead",
          "[ecs][identity]") {
	World original;

	// 8 entities across two archetypes, mixing mutable and immutable creation.
	EntityID const e0 = original.create_entity(IdsA { 10 });
	EntityID const e1 = original.create_entity(IdsB { 11 });
	ImmutableEntityID const imm2 = original.create_immutable_entity(IdsA { 12 });
	EntityID const e3 = original.create_entity(IdsA { 13 }, IdsB { 113 });
	EntityID const e4 = original.create_entity(IdsB { 14 });
	ImmutableEntityID const imm5 = original.create_immutable_entity(IdsB { 15 });
	EntityID const e6 = original.create_entity(IdsA { 16 });
	EntityID const e7 = original.create_entity(IdsA { 17 });

	// Scattered destroys (LIFO chain head = last destroyed), then a reuse + re-destroy so the
	// free list carries a generation-2 slot.
	original.destroy_entity(e4);
	original.destroy_entity(e1);
	original.destroy_entity(e6);
	EntityID const e8 = original.create_entity(IdsA { 18 }); // reuses slot 6 at generation 2
	CHECK(e8 == EntityID { 6, 2 });
	original.destroy_entity(e8);

	WorldIdentitySnapshot snap;
	REQUIRE(original.snapshot_identity(snap));
	REQUIRE(snap.slots.size() == 8);
	CHECK(snap.free_list == std::vector<uint32_t> { 6, 1, 4 });
	CHECK(snap.slots[2].immutable);
	CHECK(snap.slots[5].immutable);
	CHECK_FALSE(snap.slots[0].immutable);
	CHECK(snap.slots[6].generation == 2);

	// Restore into a fresh World; recreate every live slot in canonical slot-index order.
	World restored;
	REQUIRE(restored.restore_identity(snap));
	REQUIRE(restored.restore_entity(e0, IdsA { 10 }));
	REQUIRE(restored.restore_entity(EntityID { imm2.index, imm2.generation }, IdsA { 12 }));
	REQUIRE(restored.restore_entity(e3, IdsA { 13 }, IdsB { 113 }));
	REQUIRE(restored.restore_entity(EntityID { imm5.index, imm5.generation }, IdsB { 15 }));
	REQUIRE(restored.restore_entity(e7, IdsA { 17 }));

	// Every saved live id is alive at its original (index, generation) with matching values.
	CHECK(restored.is_alive(e0));
	CHECK(restored.is_alive(e3));
	CHECK(restored.is_alive(e7));
	CHECK(restored.is_alive(imm2));
	CHECK(restored.is_alive(imm5));
	REQUIRE(restored.get_component<IdsA>(e0) != nullptr);
	CHECK(restored.get_component<IdsA>(e0)->v == 10);
	REQUIRE(restored.get_component<IdsA>(e3) != nullptr);
	CHECK(restored.get_component<IdsA>(e3)->v == 13);
	REQUIRE(restored.get_component<IdsB>(e3) != nullptr);
	CHECK(restored.get_component<IdsB>(e3)->w == 113);
	REQUIRE(restored.get_component<IdsA>(imm2) != nullptr);
	CHECK(restored.get_component<IdsA>(imm2)->v == 12);

	// Every destroyed id stays dead, including the generation-2 reuse.
	CHECK_FALSE(restored.is_alive(e1));
	CHECK_FALSE(restored.is_alive(e4));
	CHECK_FALSE(restored.is_alive(e6));
	CHECK_FALSE(restored.is_alive(e8));

	// Immutability matches per id.
	CHECK(restored.is_immutable(imm2));
	CHECK(restored.is_immutable(imm5));
	CHECK_FALSE(restored.is_immutable(e0));
	CHECK_FALSE(restored.is_immutable(e3));

	// Round-trip: re-snapshotting the fully finalised restored world reproduces the snapshot.
	WorldIdentitySnapshot resnap;
	REQUIRE(restored.snapshot_identity(resnap));
	CHECK(resnap == snap);
}

TEST_CASE("restored allocator continues exactly as the never-saved run", "[ecs][identity]") {
	World original;

	std::vector<EntityID> ids;
	for (int64_t i = 0; i < 6; ++i) {
		ids.push_back(original.create_entity(IdsA { i }));
	}
	original.destroy_entity(ids[4]);
	original.destroy_entity(ids[1]);
	original.destroy_entity(ids[2]);

	WorldIdentitySnapshot snap;
	REQUIRE(original.snapshot_identity(snap));

	World restored;
	REQUIRE(restored.restore_identity(snap));
	REQUIRE(restored.restore_entity(ids[0], IdsA { 0 }));
	REQUIRE(restored.restore_entity(ids[3], IdsA { 3 }));
	REQUIRE(restored.restore_entity(ids[5], IdsA { 5 }));

	// Identical scripted op sequence on the continued original and the restored world: drains
	// the 3-slot free list (pop order + tail hand-off + generation bump), grows fresh slots,
	// frees again, reuses again. The returned EntityID sequences must match element-wise.
	struct Script {
		static std::vector<EntityID> run(World& world) {
			std::vector<EntityID> out;
			for (int64_t i = 0; i < 5; ++i) {
				out.push_back(world.create_entity(IdsA { 100 + i }));
			}
			world.destroy_entity(out[1]);
			world.destroy_entity(out[3]);
			for (int64_t i = 0; i < 5; ++i) {
				out.push_back(world.create_entity(IdsB { 200 + i }));
			}
			world.destroy_entity(out[5]);
			for (int64_t i = 0; i < 3; ++i) {
				out.push_back(world.create_entity(IdsA { 300 + i }));
			}
			return out;
		}
	};

	std::vector<EntityID> const continued = Script::run(original);
	std::vector<EntityID> const replayed = Script::run(restored);
	REQUIRE(continued.size() == replayed.size());
	for (std::size_t i = 0; i < continued.size(); ++i) {
		CHECK(continued[i] == replayed[i]);
	}
}

TEST_CASE("snapshot refuses while a CommandBuffer holds un-applied creates", "[ecs][identity]") {
	World world;
	world.create_entity(IdsA { 1 });

	CommandBuffer cb;
	EntityID const pending = cb.create_entity(world, IdsA { 2 }); // reserves a slot immediately
	CHECK(pending.is_valid());

	WorldIdentitySnapshot snap;
	CHECK_FALSE(world.snapshot_identity(snap));

	cb.apply(world);
	REQUIRE(world.snapshot_identity(snap));
	CHECK(snap.slots.size() == 2);
	CHECK(snap.free_list.empty());
}

TEST_CASE("restore_identity validates the snapshot and the target world", "[ecs][identity]") {
	WorldIdentitySnapshot valid;
	valid.slots.resize(2);
	valid.slots[0].generation = 1;
	valid.slots[1].generation = 3;
	valid.free_list = { 1 };

	{
		// Non-fresh target world.
		World world;
		world.create_entity(IdsA { 1 });
		CHECK_FALSE(world.restore_identity(valid));
	}
	{
		// Free index out of range.
		WorldIdentitySnapshot snap = valid;
		snap.free_list = { 5 };
		World world;
		CHECK_FALSE(world.restore_identity(snap));
		CHECK(world.create_entity(IdsA { 1 }) == EntityID { 0, 1 }); // world untouched
	}
	{
		// Duplicate free index.
		WorldIdentitySnapshot snap = valid;
		snap.free_list = { 1, 1 };
		World world;
		CHECK_FALSE(world.restore_identity(snap));
	}
	{
		// Generation 0 (the invalid sentinel).
		WorldIdentitySnapshot snap = valid;
		snap.slots[0].generation = 0;
		World world;
		CHECK_FALSE(world.restore_identity(snap));
	}
	{
		// Generation with the deferred-placeholder bit.
		WorldIdentitySnapshot snap = valid;
		snap.slots[0].generation = DEFERRED_GENERATION_BIT | 1;
		World world;
		CHECK_FALSE(world.restore_identity(snap));
	}
	{
		// The valid snapshot restores.
		World world;
		CHECK(world.restore_identity(valid));
	}
}

TEST_CASE("immutable flag survives restore and still gates structural ops", "[ecs][identity][immutable]") {
	World original;
	ImmutableEntityID const imm = original.create_immutable_entity(IdsA { 42 });

	WorldIdentitySnapshot snap;
	REQUIRE(original.snapshot_identity(snap));
	REQUIRE(snap.slots.size() == 1);
	CHECK(snap.slots[0].immutable);

	World restored;
	REQUIRE(restored.restore_identity(snap));
	EntityID const laundered = EntityID { imm.index, imm.generation };
	REQUIRE(restored.restore_entity(laundered, IdsA { 42 }));

	ImmutableEntityID const handle = ImmutableEntityID { imm.index, imm.generation };
	CHECK(restored.is_alive(handle));
	CHECK(restored.is_immutable(handle));

	// Direct structural mutation refused (runtime backstop on the restored flag).
	CHECK(restored.add_component<IdsB>(laundered, IdsB { 5 }) == nullptr);
	CHECK_FALSE(restored.has_component<IdsB>(handle));

	// CommandBuffer structural mutation refused at apply.
	CommandBuffer cb;
	cb.add_component(laundered, IdsB { 6 });
	cb.apply(restored);
	CHECK_FALSE(restored.has_component<IdsB>(handle));

	// Destroy + recreate: the allocate-time immutability reset still operates on restored slots.
	restored.destroy_entity(handle);
	EntityID const reused = restored.create_entity(IdsA { 1 });
	CHECK(reused == EntityID { 0, 2 });
	CHECK_FALSE(restored.is_immutable(reused));
	CHECK(restored.add_component<IdsB>(reused, IdsB { 7 }) != nullptr);
}

TEST_CASE("restore_entity validates its target slot", "[ecs][identity]") {
	World original;
	EntityID const live = original.create_entity(IdsA { 1 });
	EntityID const dead = original.create_entity(IdsA { 2 });
	original.destroy_entity(dead);

	WorldIdentitySnapshot snap;
	REQUIRE(original.snapshot_identity(snap));

	World restored;
	REQUIRE(restored.restore_identity(snap));

	CHECK_FALSE(restored.restore_entity(EntityID { 0, 99 }, IdsA { 1 })); // wrong generation
	CHECK_FALSE(restored.restore_entity(dead, IdsA { 2 })); // free-at-snapshot slot
	CHECK_FALSE(restored.restore_entity(EntityID { 7, 1 }, IdsA { 1 })); // out of range
	CHECK_FALSE(restored.restore_entity(EntityID { 0, DEFERRED_GENERATION_BIT }, IdsA { 1 })); // deferred id
	CHECK_FALSE(restored.is_alive(live)); // nothing finalised yet

	REQUIRE(restored.restore_entity(live, IdsA { 1 }));
	CHECK(restored.is_alive(live));
	CHECK_FALSE(restored.restore_entity(live, IdsA { 1 })); // already finalised
}

TEST_CASE("single free slot: self-pointing tail restores", "[ecs][identity]") {
	World original;
	EntityID const e = original.create_entity(IdsA { 1 });
	original.destroy_entity(e);

	WorldIdentitySnapshot snap;
	REQUIRE(original.snapshot_identity(snap));
	CHECK(snap.free_list == std::vector<uint32_t> { 0 });

	World restored;
	REQUIRE(restored.restore_identity(snap)); // no live slots to finalise

	// Both worlds: reuse slot 0 at generation 2, then fresh growth at {1, 1}.
	EntityID const a1 = original.create_entity(IdsA { 2 });
	EntityID const b1 = restored.create_entity(IdsA { 2 });
	CHECK(a1 == EntityID { 0, 2 });
	CHECK(b1 == a1);

	EntityID const a2 = original.create_entity(IdsA { 3 });
	EntityID const b2 = restored.create_entity(IdsA { 3 });
	CHECK(a2 == EntityID { 1, 1 });
	CHECK(b2 == a2);
}
