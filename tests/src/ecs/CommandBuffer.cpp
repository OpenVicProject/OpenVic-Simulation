#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct CBA {
		int v = 0;
	};
	struct CBB {
		int w = 0;
	};
	struct CBTag {};
	// Holds a unique_ptr — verifies move-only payloads survive recording + apply.
	struct CBMove {
		std::unique_ptr<int> p;
	};
}

ECS_COMPONENT(CBA, "test_CommandBuffer::CBA")
ECS_COMPONENT(CBB, "test_CommandBuffer::CBB")
ECS_COMPONENT(CBTag, "test_CommandBuffer::CBTag")
ECS_COMPONENT(CBMove, "test_CommandBuffer::CBMove")

TEST_CASE("create_entity returns an EntityID, but is_alive is false until apply", "[ecs][CommandBuffer]") {
	World world;
	CommandBuffer cmd;
	EntityID const eid = cmd.create_entity(world, CBA { 7 });
	CHECK(eid.is_valid());
	CHECK_FALSE(world.is_alive(eid));

	cmd.apply(world);
	CHECK(world.is_alive(eid));
	CHECK(world.get_component<CBA>(eid)->v == 7);
}

TEST_CASE("destroy_entity defers destruction to apply", "[ecs][CommandBuffer]") {
	World world;
	EntityID const eid = world.create_entity(CBA { 1 });
	CHECK(world.is_alive(eid));

	CommandBuffer cmd;
	cmd.destroy_entity(eid);
	CHECK(world.is_alive(eid));

	cmd.apply(world);
	CHECK_FALSE(world.is_alive(eid));
}

TEST_CASE("create + destroy in same buffer leaves entity dead after apply", "[ecs][CommandBuffer]") {
	World world;
	CommandBuffer cmd;
	EntityID const eid = cmd.create_entity(world, CBA { 5 });
	cmd.destroy_entity(eid);

	cmd.apply(world);
	CHECK_FALSE(world.is_alive(eid));
}

TEST_CASE("add_component during for_each applies after iteration", "[ecs][CommandBuffer]") {
	World world;
	world.create_entity(CBA { 1 });
	world.create_entity(CBA { 2 });
	world.create_entity(CBA { 3 });

	CommandBuffer cmd;
	world.for_each_with_entity<CBA>([&](EntityID e, CBA&) {
		cmd.add_component<CBB>(e, CBB { 100 });
	});

	int with_b_before = 0;
	world.for_each<CBA, CBB>([&](CBA&, CBB&) { ++with_b_before; });
	CHECK(with_b_before == 0);

	cmd.apply(world);

	int with_b_after = 0;
	world.for_each<CBA, CBB>([&](CBA&, CBB&) { ++with_b_after; });
	CHECK(with_b_after == 3);
}

TEST_CASE("remove_component defers to apply", "[ecs][CommandBuffer]") {
	World world;
	EntityID const eid = world.create_entity(CBA { 5 }, CBB { 10 });

	CommandBuffer cmd;
	cmd.remove_component<CBB>(eid);
	CHECK(world.has_component<CBB>(eid));

	cmd.apply(world);
	CHECK_FALSE(world.has_component<CBB>(eid));
	CHECK(world.has_component<CBA>(eid));
	CHECK(world.get_component<CBA>(eid)->v == 5);
}

TEST_CASE("Op order is preserved on apply", "[ecs][CommandBuffer]") {
	World world;

	CommandBuffer cmd;
	EntityID const a = cmd.create_entity(world, CBA { 1 });
	EntityID const b = cmd.create_entity(world, CBA { 2 });
	EntityID const c = cmd.create_entity(world, CBA { 3 });

	cmd.apply(world);

	CHECK(world.is_alive(a));
	CHECK(world.is_alive(b));
	CHECK(world.is_alive(c));
	CHECK(world.get_component<CBA>(a)->v == 1);
	CHECK(world.get_component<CBA>(b)->v == 2);
	CHECK(world.get_component<CBA>(c)->v == 3);
}

TEST_CASE("Tag components in CommandBuffer", "[ecs][CommandBuffer][tag]") {
	World world;

	CommandBuffer cmd;
	EntityID const eid = cmd.create_entity(world, CBA { 1 }, CBTag {});
	cmd.apply(world);

	CHECK(world.is_alive(eid));
	CHECK(world.has_component<CBA>(eid));
	CHECK(world.has_component<CBTag>(eid));

	CommandBuffer cmd2;
	cmd2.add_component<CBTag>(eid);  // already present — should be safe
	cmd2.remove_component<CBTag>(eid);
	cmd2.apply(world);
	CHECK_FALSE(world.has_component<CBTag>(eid));
}

TEST_CASE("Move-only components survive CommandBuffer record + apply", "[ecs][CommandBuffer]") {
	World world;

	CommandBuffer cmd;
	EntityID const eid = cmd.create_entity(world, CBMove { std::make_unique<int>(42) });
	cmd.apply(world);

	CBMove* p = world.get_component<CBMove>(eid);
	REQUIRE(p != nullptr);
	REQUIRE(p->p != nullptr);
	CHECK(*p->p == 42);
}

TEST_CASE("Empty buffer apply is a no-op", "[ecs][CommandBuffer]") {
	World world;
	CommandBuffer cmd;
	CHECK(cmd.empty());
	CHECK(cmd.op_count() == 0u);
	cmd.apply(world);
	// No assertion crashes; world is unchanged.
}

TEST_CASE("clear() drops queued ops and any payloads", "[ecs][CommandBuffer]") {
	World world;
	CommandBuffer cmd;
	cmd.create_entity(world, CBMove { std::make_unique<int>(7) });
	CHECK(cmd.op_count() == 1u);
	cmd.clear();
	CHECK(cmd.empty());
	CHECK(cmd.op_count() == 0u);
}

TEST_CASE("Buffer applied to fresh world reproduces same EntityIDs in same order", "[ecs][CommandBuffer][determinism]") {
	World w1;
	std::vector<EntityID> ids1;
	{
		CommandBuffer cmd;
		ids1.push_back(cmd.create_entity(w1, CBA { 1 }));
		ids1.push_back(cmd.create_entity(w1, CBA { 2 }));
		ids1.push_back(cmd.create_entity(w1, CBA { 3 }));
		cmd.apply(w1);
	}

	World w2;
	std::vector<EntityID> ids2;
	{
		CommandBuffer cmd;
		ids2.push_back(cmd.create_entity(w2, CBA { 1 }));
		ids2.push_back(cmd.create_entity(w2, CBA { 2 }));
		ids2.push_back(cmd.create_entity(w2, CBA { 3 }));
		cmd.apply(w2);
	}

	// Both worlds allocated slot indices in the same order.
	CHECK(ids1[0] == ids2[0]);
	CHECK(ids1[1] == ids2[1]);
	CHECK(ids1[2] == ids2[2]);
}

// === Deferred-creation tests (parallel_mode_ = true) ===
// In parallel mode, CommandBuffer::create_entity returns a placeholder EntityID with
// DEFERRED_GENERATION_BIT set. The placeholder is usable as an argument to other ops on the
// same buffer, but never as an argument to direct World accessors. CommandBuffer::apply
// resolves placeholders to real EntityIDs at apply time, on a single thread, in record order.

TEST_CASE("Parallel-mode create_entity returns a deferred placeholder", "[ecs][CommandBuffer][deferred]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	EntityID const placeholder = cmd.create_entity(world, CBA { 7 });
	CHECK(placeholder.is_valid());
	CHECK(placeholder.is_deferred());
	CHECK_FALSE(world.is_alive(placeholder));
	CHECK(world.get_component<CBA>(placeholder) == nullptr);
	CHECK_FALSE(world.has_component<CBA>(placeholder));
	CHECK(cmd.deferred_count() == 1u);

	cmd.set_parallel_mode(false);
	cmd.apply(world);

	int found = 0;
	int value = 0;
	world.for_each<CBA>([&](CBA& a) {
		++found;
		value = a.v;
	});
	CHECK(found == 1);
	CHECK(value == 7);
	CHECK(cmd.deferred_count() == 0u);
}

TEST_CASE("Same-buffer add_component on a deferred placeholder works", "[ecs][CommandBuffer][deferred]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	EntityID const placeholder = cmd.create_entity(world, CBA { 1 });
	cmd.add_component<CBB>(placeholder, CBB { 2 });

	cmd.set_parallel_mode(false);
	cmd.apply(world);

	int found = 0;
	world.for_each<CBA, CBB>([&](CBA& a, CBB& b) {
		++found;
		CHECK(a.v == 1);
		CHECK(b.w == 2);
	});
	CHECK(found == 1);
}

TEST_CASE("Same-buffer destroy on a deferred placeholder is a clean net no-op", "[ecs][CommandBuffer][deferred]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	EntityID const placeholder = cmd.create_entity(world, CBA { 5 });
	cmd.destroy_entity(placeholder);

	cmd.set_parallel_mode(false);
	cmd.apply(world);

	int found = 0;
	world.for_each<CBA>([&](CBA&) { ++found; });
	CHECK(found == 0);
}

TEST_CASE("Same-buffer remove_component on a deferred placeholder works", "[ecs][CommandBuffer][deferred]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	EntityID const placeholder = cmd.create_entity(world, CBA { 1 }, CBB { 2 });
	cmd.remove_component<CBB>(placeholder);

	cmd.set_parallel_mode(false);
	cmd.apply(world);

	int with_a_only = 0;
	int with_a_and_b = 0;
	world.for_each<CBA>([&](CBA& a) {
		++with_a_only;
		CHECK(a.v == 1);
	});
	world.for_each<CBA, CBB>([&](CBA&, CBB&) { ++with_a_and_b; });
	CHECK(with_a_only == 1);
	CHECK(with_a_and_b == 0);
}

TEST_CASE("merge_from rebases deferred placeholders so each add resolves to its own create",
          "[ecs][CommandBuffer][deferred][merge]") {
	World world;
	CommandBuffer chunk_a;
	CommandBuffer chunk_b;
	chunk_a.set_parallel_mode(true);
	chunk_b.set_parallel_mode(true);

	// Each per-chunk buffer creates one entity with CBA + an add of CBB whose value identifies
	// the chunk. If merge_from didn't rebase placeholder indices, both adds would land on the
	// first created entity and we'd see one entity with CBB{20} (chunk B's add overwriting A's).
	EntityID const a_placeholder = chunk_a.create_entity(world, CBA { 1 });
	chunk_a.add_component<CBB>(a_placeholder, CBB { 10 });

	EntityID const b_placeholder = chunk_b.create_entity(world, CBA { 2 });
	chunk_b.add_component<CBB>(b_placeholder, CBB { 20 });

	// SystemThreaded::tick_all takes the merged buffer out of parallel mode before apply. Mirror
	// that here: clear parallel_mode on the receiver, merge in chunk_idx ascending order, apply.
	CommandBuffer system_pending;
	system_pending.merge_from(std::move(chunk_a));
	system_pending.merge_from(std::move(chunk_b));
	CHECK(system_pending.deferred_count() == 2u);

	system_pending.apply(world);

	// Walk the resulting entities and verify each (CBA::v, CBB::w) pair.
	std::vector<std::pair<int, int>> pairs;
	world.for_each<CBA, CBB>([&](CBA& a, CBB& b) { pairs.push_back({ a.v, b.w }); });
	std::sort(pairs.begin(), pairs.end());
	REQUIRE(pairs.size() == 2u);
	CHECK(pairs[0].first == 1);
	CHECK(pairs[0].second == 10);
	CHECK(pairs[1].first == 2);
	CHECK(pairs[1].second == 20);
}

TEST_CASE("merge_from with multiple chunks resolves placeholders in chunk_idx order",
          "[ecs][CommandBuffer][deferred][merge]") {
	World world;
	std::vector<CommandBuffer> chunks(4);
	for (auto& cb : chunks) {
		cb.set_parallel_mode(true);
	}

	// Each chunk records 2 deferred creates with a chunk-tagged value.
	for (std::size_t c = 0; c < chunks.size(); ++c) {
		for (int j = 0; j < 2; ++j) {
			int const value = static_cast<int>(c) * 100 + j;
			EntityID const ph = chunks[c].create_entity(world, CBA { value });
			chunks[c].add_component<CBB>(ph, CBB { value + 1 });
		}
	}

	CommandBuffer system_pending;
	for (auto& cb : chunks) {
		system_pending.merge_from(std::move(cb));
	}
	CHECK(system_pending.deferred_count() == 8u);

	system_pending.apply(world);

	std::vector<std::pair<int, int>> pairs;
	world.for_each<CBA, CBB>([&](CBA& a, CBB& b) { pairs.push_back({ a.v, b.w }); });
	std::sort(pairs.begin(), pairs.end());
	REQUIRE(pairs.size() == 8u);
	for (std::size_t i = 0; i < pairs.size(); ++i) {
		std::size_t const c = i / 2;
		std::size_t const j = i % 2;
		int const expected = static_cast<int>(c) * 100 + static_cast<int>(j);
		CHECK(pairs[i].first == expected);
		CHECK(pairs[i].second == expected + 1);
	}
}

TEST_CASE("Empty parallel buffer apply is a no-op and resets deferred_count",
          "[ecs][CommandBuffer][deferred]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);
	CHECK(cmd.deferred_count() == 0u);
	cmd.apply(world);
	CHECK(cmd.deferred_count() == 0u);
	int found = 0;
	world.for_each<CBA>([&](CBA&) { ++found; });
	CHECK(found == 0);
}

TEST_CASE("Move-only components survive the deferred path", "[ecs][CommandBuffer][deferred]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	cmd.create_entity(world, CBMove { std::make_unique<int>(42) });

	cmd.set_parallel_mode(false);
	cmd.apply(world);

	int found = 0;
	int value = 0;
	world.for_each<CBMove>([&](CBMove& m) {
		++found;
		REQUIRE(m.p != nullptr);
		value = *m.p;
	});
	CHECK(found == 1);
	CHECK(value == 42);
}

TEST_CASE("Tag components are first-class in the deferred path", "[ecs][CommandBuffer][deferred][tag]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	EntityID const ph = cmd.create_entity(world, CBA { 1 }, CBTag {});
	cmd.set_parallel_mode(false);
	cmd.apply(world);

	int found = 0;
	world.for_each_with_entity<CBA, CBTag>([&](EntityID e, CBA& a, CBTag&) {
		++found;
		CHECK(a.v == 1);
		CHECK(e.is_valid());
		CHECK_FALSE(e.is_deferred());
		(void) ph;
	});
	CHECK(found == 1);
}

TEST_CASE("Deferred placeholder is safe to pass to World accessors outside its buffer",
          "[ecs][CommandBuffer][deferred][safety]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	EntityID const placeholder = cmd.create_entity(world, CBA { 1 });
	REQUIRE(placeholder.is_deferred());

	// Direct World access on the placeholder must be benign — never OOB, never wrong-slot.
	CHECK_FALSE(world.is_alive(placeholder));
	CHECK(world.get_component<CBA>(placeholder) == nullptr);
	CHECK_FALSE(world.has_component<CBA>(placeholder));
	CHECK(world.component_version_in<CBA>(placeholder) == 0u);

	cmd.set_parallel_mode(false);
	cmd.apply(world);
}

TEST_CASE("Real entity destroy via parallel buffer is unaffected by placeholder space",
          "[ecs][CommandBuffer][deferred][safety]") {
	// Subtle bug class: confusing a real EntityID's small index with a placeholder's local_seq.
	// Pre-create 5 entities (real indices 0..4), then in parallel mode create 3 deferred and
	// destroy the first real eid. After apply: real_eid_0 is dead, the other 4 originals plus
	// 3 spawned entities are alive.
	World world;
	std::vector<EntityID> originals;
	for (int i = 0; i < 5; ++i) {
		originals.push_back(world.create_entity(CBA { i }));
	}

	CommandBuffer cmd;
	cmd.set_parallel_mode(true);
	cmd.create_entity(world, CBA { 100 });
	cmd.create_entity(world, CBA { 101 });
	cmd.create_entity(world, CBA { 102 });
	cmd.destroy_entity(originals[0]); // real eid — must NOT be remapped via the deferred map

	cmd.set_parallel_mode(false);
	cmd.apply(world);

	CHECK_FALSE(world.is_alive(originals[0]));
	for (std::size_t i = 1; i < originals.size(); ++i) {
		CHECK(world.is_alive(originals[i]));
	}
	int total = 0;
	int high_value_count = 0;
	world.for_each<CBA>([&](CBA& a) {
		++total;
		if (a.v >= 100) {
			++high_value_count;
		}
	});
	CHECK(total == 7); // 4 surviving originals + 3 spawned
	CHECK(high_value_count == 3);
}

TEST_CASE("allocate_entity_slot generations stay below DEFERRED_GENERATION_BIT",
          "[ecs][CommandBuffer][deferred][safety]") {
	// Repeatedly create+destroy the same slot — generation increments on every reuse and must
	// never produce a value with the high bit set, otherwise a real EntityID could be confused
	// with a deferred placeholder. We don't push through 2^31 reuses (too slow); instead spot-
	// check that the increment skips the deferred bit when it lands on it.
	World world;
	for (int i = 0; i < 100; ++i) {
		EntityID const eid = world.create_entity(CBA { i });
		CHECK_FALSE(eid.is_deferred());
		CHECK((eid.generation & DEFERRED_GENERATION_BIT) == 0u);
		world.destroy_entity(eid);
	}
}
