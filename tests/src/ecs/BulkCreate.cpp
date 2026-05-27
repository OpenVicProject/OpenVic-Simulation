#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

// === Bulk entity creation (ECS_SIM_ARCHITECTURE §9 item 4) ===
// The load-bearing property under test: a bulk create yields the IDENTICAL end state —
// including identical EntityID assignment — as the equivalent create_entity loop. EntityID
// assignment feeds the per-entity RNG stream (§8), so any divergence would be a
// simulation-behavior change.

namespace {
	struct BCA {
		int v = 0;
	};
	struct BCB {
		int w = 0;
	};
	struct BCTag {};
	// Holds a unique_ptr — verifies move-only payloads survive the bulk staging blocks.
	struct BCMove {
		std::unique_ptr<int> p;
	};
	// Checksum enforcement: heap-holding type needs a custom hash — fold presence then the
	// pointee's value, never the address.
	inline uint64_t ecs_checksum(BCMove const& m, uint64_t seed) {
		uint64_t h = fold_uint64(m.p != nullptr ? 1u : 0u, seed);
		if (m.p != nullptr) {
			h = fold_uint64(static_cast<uint64_t>(static_cast<uint32_t>(*m.p)), h);
		}
		return h;
	}
	// Instance-counting component (non-trivially-copyable, so the bulk paths take the
	// element-loop move path, not memcpy). `live` tracks constructed-minus-destroyed; tests
	// compare deltas to prove no leak / double-destroy through staging blocks.
	struct BCCounted {
		static inline int live = 0;
		int v = 0;

		BCCounted() {
			++live;
		}
		explicit BCCounted(int value) : v { value } {
			++live;
		}
		BCCounted(BCCounted const& other) : v { other.v } {
			++live;
		}
		BCCounted(BCCounted&& other) noexcept : v { other.v } {
			++live;
		}
		BCCounted& operator=(BCCounted const&) = default;
		BCCounted& operator=(BCCounted&&) = default;
		~BCCounted() {
			--live;
		}
	};
	// Checksum enforcement: user-provided ctors make the type non-trivially-copyable, so the
	// byte path is unavailable — fold the one data member.
	inline uint64_t ecs_checksum(BCCounted const& c, uint64_t seed) {
		return fold_uint64(static_cast<uint64_t>(static_cast<uint32_t>(c.v)), seed);
	}
}

ECS_COMPONENT(BCA, "test_BulkCreate::BCA")
ECS_COMPONENT(BCB, "test_BulkCreate::BCB")
ECS_COMPONENT(BCTag, "test_BulkCreate::BCTag")
ECS_COMPONENT(BCMove, "test_BulkCreate::BCMove")
ECS_COMPONENT(BCCounted, "test_BulkCreate::BCCounted")

namespace {
	// Collects (EntityID, BCA::v) pairs in iteration order — iteration order reflects
	// archetype/chunk/row packing, so equal sequences mean equal packing AND equal ids.
	std::vector<std::pair<uint64_t, int>> collect_bca(World& world) {
		std::vector<std::pair<uint64_t, int>> result;
		world.for_each_with_entity<BCA>([&](EntityID e, BCA& a) {
			result.push_back({ e.to_uint64(), a.v });
		});
		return result;
	}
}

TEST_CASE("Direct bulk create matches the equivalent create_entity loop", "[ecs][BulkCreate]") {
	std::size_t const count = 100;

	World loop_world;
	std::vector<EntityID> loop_ids;
	for (std::size_t i = 0; i < count; ++i) {
		loop_ids.push_back(loop_world.create_entity(
			BCA { static_cast<int>(i) }, BCB { static_cast<int>(i * 2) }, BCTag {}
		));
	}

	World bulk_world;
	std::vector<BCA> as;
	std::vector<BCB> bs;
	for (std::size_t i = 0; i < count; ++i) {
		as.push_back(BCA { static_cast<int>(i) });
		bs.push_back(BCB { static_cast<int>(i * 2) });
	}
	std::vector<EntityID> bulk_ids(count);
	REQUIRE(bulk_world.create_entities<BCA, BCB, BCTag>(count, bulk_ids, as, bs));

	// Identical id assignment, element-wise (the RNG-stream guarantee).
	REQUIRE(bulk_ids.size() == loop_ids.size());
	for (std::size_t i = 0; i < count; ++i) {
		CHECK(bulk_ids[i] == loop_ids[i]);
	}

	// Identical component values and tag membership.
	for (std::size_t i = 0; i < count; ++i) {
		REQUIRE(bulk_world.get_component<BCA>(bulk_ids[i]) != nullptr);
		CHECK(bulk_world.get_component<BCA>(bulk_ids[i])->v == static_cast<int>(i));
		CHECK(bulk_world.get_component<BCB>(bulk_ids[i])->w == static_cast<int>(i * 2));
		CHECK(bulk_world.has_component<BCTag>(bulk_ids[i]));
		CHECK_FALSE(bulk_world.is_immutable(bulk_ids[i]));
	}

	// Identical packing: iteration order sequences match.
	CHECK(collect_bca(loop_world) == collect_bca(bulk_world));
}

TEST_CASE("Direct bulk create fills the partial tail chunk first, then whole chunks", "[ecs][BulkCreate]") {
	// Pre-create singles so the tail chunk is partial, then bulk-create enough to cross
	// several chunk boundaries (BCA-only archetype: 16 KB / (8 + 4) ≈ 1365 rows per chunk).
	std::size_t const presize = 7;
	std::size_t const count = 3000;

	World loop_world;
	World bulk_world;
	for (std::size_t i = 0; i < presize; ++i) {
		loop_world.create_entity(BCA { static_cast<int>(i) });
		bulk_world.create_entity(BCA { static_cast<int>(i) });
	}

	std::vector<EntityID> loop_ids;
	for (std::size_t i = 0; i < count; ++i) {
		loop_ids.push_back(loop_world.create_entity(BCA { 1000 + static_cast<int>(i) }));
	}

	std::vector<BCA> as;
	for (std::size_t i = 0; i < count; ++i) {
		as.push_back(BCA { 1000 + static_cast<int>(i) });
	}
	std::vector<EntityID> bulk_ids(count);
	REQUIRE(bulk_world.create_entities<BCA>(count, bulk_ids, as));

	for (std::size_t i = 0; i < count; ++i) {
		CHECK(bulk_ids[i] == loop_ids[i]);
	}
	CHECK(collect_bca(loop_world) == collect_bca(bulk_world));
}

TEST_CASE("Direct bulk create pops the free list exactly like the loop", "[ecs][BulkCreate]") {
	// Seed both worlds with an identical free list, then create more entities than the free
	// list holds — bulk and loop must reuse the same slots in the same order and then grow
	// identically.
	World loop_world;
	World bulk_world;
	std::vector<EntityID> seed_loop;
	std::vector<EntityID> seed_bulk;
	for (std::size_t i = 0; i < 10; ++i) {
		seed_loop.push_back(loop_world.create_entity(BCA { static_cast<int>(i) }));
		seed_bulk.push_back(bulk_world.create_entity(BCA { static_cast<int>(i) }));
	}
	for (std::size_t i : { 1, 3, 4, 8 }) {
		loop_world.destroy_entity(seed_loop[i]);
		bulk_world.destroy_entity(seed_bulk[i]);
	}

	std::size_t const count = 8;
	std::vector<EntityID> loop_ids;
	for (std::size_t i = 0; i < count; ++i) {
		loop_ids.push_back(loop_world.create_entity(BCA { 100 + static_cast<int>(i) }));
	}

	std::vector<BCA> as;
	for (std::size_t i = 0; i < count; ++i) {
		as.push_back(BCA { 100 + static_cast<int>(i) });
	}
	std::vector<EntityID> bulk_ids(count);
	REQUIRE(bulk_world.create_entities<BCA>(count, bulk_ids, as));

	for (std::size_t i = 0; i < count; ++i) {
		CHECK(bulk_ids[i] == loop_ids[i]);
	}
	CHECK(collect_bca(loop_world) == collect_bca(bulk_world));
}

TEST_CASE("Bulk create with count == 0 is a no-op", "[ecs][BulkCreate]") {
	World world;
	std::vector<EntityID> empty_out;
	CHECK(world.create_entities<BCA>(0, empty_out));

	int found = 0;
	world.for_each<BCA>([&](BCA&) { ++found; });
	CHECK(found == 0);

	CommandBuffer cmd;
	CHECK(cmd.create_entities<BCA>(world, 0, empty_out));
	CHECK(cmd.op_count() == 0u);
	CHECK(cmd.empty());
}

TEST_CASE("Bulk create refuses mismatched out_ids / span lengths", "[ecs][BulkCreate]") {
	World world;
	std::vector<BCA> as(5);
	std::vector<EntityID> short_out(3); // != count
	CHECK_FALSE(world.create_entities<BCA>(5, short_out, as));

	std::vector<EntityID> out(5);
	std::vector<BCA> short_span(4); // != count
	CHECK_FALSE(world.create_entities<BCA>(5, out, short_span));

	int found = 0;
	world.for_each<BCA>([&](BCA&) { ++found; });
	CHECK(found == 0);

	CommandBuffer cmd;
	CHECK_FALSE(cmd.create_entities<BCA>(world, 5, short_out, as));
	CHECK(cmd.op_count() == 0u);
}

TEST_CASE("Bulk create handles tags and default-construction", "[ecs][BulkCreate][tag]") {
	World world;

	// Tag-only signature, no spans.
	std::size_t const tag_count = 10;
	std::vector<EntityID> tag_ids(tag_count);
	REQUIRE(world.create_entities<BCTag>(tag_count, tag_ids));
	for (EntityID const eid : tag_ids) {
		CHECK(world.is_alive(eid));
		CHECK(world.has_component<BCTag>(eid));
	}

	// Data + tag signature with default-construction (no spans at all).
	std::size_t const def_count = 6;
	std::vector<EntityID> def_ids(def_count);
	REQUIRE((world.create_entities<BCA, BCTag>(def_count, def_ids)));
	for (EntityID const eid : def_ids) {
		REQUIRE(world.get_component<BCA>(eid) != nullptr);
		CHECK(world.get_component<BCA>(eid)->v == 0);
		CHECK(world.has_component<BCTag>(eid));
	}

	// Same through the CommandBuffer.
	CommandBuffer cmd;
	std::vector<EntityID> cb_ids(def_count);
	REQUIRE((cmd.create_entities<BCB, BCTag>(world, def_count, cb_ids)));
	cmd.apply(world);
	for (EntityID const eid : cb_ids) {
		REQUIRE(world.get_component<BCB>(eid) != nullptr);
		CHECK(world.get_component<BCB>(eid)->w == 0);
		CHECK(world.has_component<BCTag>(eid));
	}
}

TEST_CASE("Move-only components survive bulk create on all three paths", "[ecs][BulkCreate]") {
	std::size_t const count = 5;

	// Direct path.
	{
		World world;
		std::vector<BCMove> vals;
		for (std::size_t i = 0; i < count; ++i) {
			vals.push_back(BCMove { std::make_unique<int>(static_cast<int>(i)) });
		}
		std::vector<EntityID> ids(count);
		REQUIRE(world.create_entities<BCMove>(count, ids, vals));
		for (std::size_t i = 0; i < count; ++i) {
			BCMove* m = world.get_component<BCMove>(ids[i]);
			REQUIRE(m != nullptr);
			REQUIRE(m->p != nullptr);
			CHECK(*m->p == static_cast<int>(i));
			CHECK(vals[i].p == nullptr); // input spans are moved-from
		}
	}

	// CommandBuffer serial path.
	{
		World world;
		std::vector<BCMove> vals;
		for (std::size_t i = 0; i < count; ++i) {
			vals.push_back(BCMove { std::make_unique<int>(static_cast<int>(i)) });
		}
		std::vector<EntityID> ids(count);
		CommandBuffer cmd;
		REQUIRE(cmd.create_entities<BCMove>(world, count, ids, vals));
		cmd.apply(world);
		for (std::size_t i = 0; i < count; ++i) {
			BCMove* m = world.get_component<BCMove>(ids[i]);
			REQUIRE(m != nullptr);
			REQUIRE(m->p != nullptr);
			CHECK(*m->p == static_cast<int>(i));
		}
	}

	// CommandBuffer parallel path.
	{
		World world;
		std::vector<BCMove> vals;
		for (std::size_t i = 0; i < count; ++i) {
			vals.push_back(BCMove { std::make_unique<int>(static_cast<int>(i)) });
		}
		std::vector<EntityID> phs(count);
		CommandBuffer cmd;
		cmd.set_parallel_mode(true);
		REQUIRE(cmd.create_entities<BCMove>(world, count, phs, vals));
		CHECK(cmd.deferred_count() == count);
		for (EntityID const ph : phs) {
			CHECK(ph.is_deferred());
		}
		cmd.set_parallel_mode(false);
		cmd.apply(world);

		std::vector<int> seen;
		world.for_each<BCMove>([&](BCMove& m) {
			REQUIRE(m.p != nullptr);
			seen.push_back(*m.p);
		});
		std::sort(seen.begin(), seen.end());
		REQUIRE(seen.size() == count);
		for (std::size_t i = 0; i < count; ++i) {
			CHECK(seen[i] == static_cast<int>(i));
		}
	}
}

TEST_CASE("Bulk staging blocks neither leak nor double-destroy", "[ecs][BulkCreate]") {
	int const before = BCCounted::live;
	std::size_t const count = 16;

	// clear() without apply destroys staged values exactly once.
	{
		World world;
		CommandBuffer cmd;
		{
			std::vector<BCCounted> vals;
			vals.reserve(count);
			for (std::size_t i = 0; i < count; ++i) {
				vals.emplace_back(static_cast<int>(i));
			}
			std::vector<EntityID> ids(count);
			REQUIRE(cmd.create_entities<BCCounted>(world, count, ids, vals));
			// vals (moved-from husks) + staged block both still count as live instances.
			CHECK(BCCounted::live == before + 2 * static_cast<int>(count));
		}
		cmd.clear();
		CHECK(BCCounted::live == before);
	}
	CHECK(BCCounted::live == before);

	// Record + apply + world teardown destroys everything exactly once.
	{
		World world;
		CommandBuffer cmd;
		{
			std::vector<BCCounted> vals;
			vals.reserve(count);
			for (std::size_t i = 0; i < count; ++i) {
				vals.emplace_back(static_cast<int>(i));
			}
			std::vector<EntityID> ids(count);
			REQUIRE(cmd.create_entities<BCCounted>(world, count, ids, vals));
			cmd.apply(world);
		}
		// Only the world's copies remain.
		CHECK(BCCounted::live == before + static_cast<int>(count));
	}
	CHECK(BCCounted::live == before);
}

TEST_CASE("CB serial bulk: out_ids are real and usable for same-buffer ops", "[ecs][BulkCreate]") {
	World world;
	CommandBuffer cmd;

	std::size_t const count = 4;
	std::vector<BCA> as;
	for (std::size_t i = 0; i < count; ++i) {
		as.push_back(BCA { static_cast<int>(i) });
	}
	std::vector<EntityID> ids(count);
	REQUIRE(cmd.create_entities<BCA>(world, count, ids, as));
	for (EntityID const eid : ids) {
		CHECK(eid.is_valid());
		CHECK_FALSE(eid.is_deferred());
		CHECK_FALSE(world.is_alive(eid)); // reserved-but-unfinalised until apply
	}

	cmd.add_component<BCB>(ids[1], BCB { 42 });
	cmd.destroy_entity(ids[0]);
	cmd.apply(world);

	CHECK_FALSE(world.is_alive(ids[0]));
	CHECK(world.is_alive(ids[1]));
	REQUIRE(world.get_component<BCB>(ids[1]) != nullptr);
	CHECK(world.get_component<BCB>(ids[1])->w == 42);
	CHECK(world.is_alive(ids[2]));
	CHECK(world.is_alive(ids[3]));
}

TEST_CASE("CB serial bulk: a slot destroyed between record and apply is skipped without leaking",
          "[ecs][BulkCreate]") {
	int const before = BCCounted::live;
	std::size_t const count = 6;
	std::size_t const dropped = 2;

	{
		World world;
		CommandBuffer cmd;
		std::vector<EntityID> ids(count);
		{
			std::vector<BCCounted> vals;
			vals.reserve(count);
			for (std::size_t i = 0; i < count; ++i) {
				vals.emplace_back(static_cast<int>(i));
			}
			REQUIRE(cmd.create_entities<BCCounted>(world, count, ids, vals));
		}

		// Dropping a reserved slot before apply forces the cold per-entity fallback in
		// finalize_reserved_entities_bulk — only the dropped slot is skipped, exactly as a
		// loop of single creates would behave.
		world.destroy_entity(ids[dropped]);

		cmd.apply(world);

		CHECK_FALSE(world.is_alive(ids[dropped]));
		int found = 0;
		world.for_each<BCCounted>([&](BCCounted&) { ++found; });
		CHECK(found == static_cast<int>(count) - 1);
		for (std::size_t i = 0; i < count; ++i) {
			if (i == dropped) {
				continue;
			}
			REQUIRE(world.get_component<BCCounted>(ids[i]) != nullptr);
			CHECK(world.get_component<BCCounted>(ids[i])->v == static_cast<int>(i));
		}
		// The skipped slot's staged value was destroyed on the fallback path.
		CHECK(BCCounted::live == before + static_cast<int>(count) - 1);
	}
	CHECK(BCCounted::live == before);
}

TEST_CASE("CB parallel bulk: sequential placeholders interleave correctly with single creates",
          "[ecs][BulkCreate][deferred]") {
	World world;
	CommandBuffer cmd;
	cmd.set_parallel_mode(true);

	// bulk(3) → placeholders 0,1,2; single → 3; bulk(2) → 4,5.
	std::vector<BCA> first_vals { BCA { 10 }, BCA { 11 }, BCA { 12 } };
	std::vector<EntityID> first(3);
	REQUIRE(cmd.create_entities<BCA>(world, 3, first, first_vals));
	EntityID const single = cmd.create_entity(world, BCA { 20 });
	std::vector<BCA> second_vals { BCA { 30 }, BCA { 31 } };
	std::vector<EntityID> second(2);
	REQUIRE(cmd.create_entities<BCA>(world, 2, second, second_vals));

	CHECK(cmd.deferred_count() == 6u);
	for (std::size_t i = 0; i < 3; ++i) {
		CHECK(first[i].is_deferred());
		CHECK(first[i].index == static_cast<uint32_t>(i));
	}
	CHECK(single.is_deferred());
	CHECK(single.index == 3u);
	CHECK(second[0].index == 4u);
	CHECK(second[1].index == 5u);

	// Adds referencing batch placeholders at offsets 0, mid, count-1, plus the single.
	cmd.add_component<BCB>(first[0], BCB { 100 });
	cmd.add_component<BCB>(first[2], BCB { 102 });
	cmd.add_component<BCB>(single, BCB { 200 });
	cmd.add_component<BCB>(second[1], BCB { 301 });

	cmd.set_parallel_mode(false);
	cmd.apply(world);

	std::vector<std::pair<int, int>> pairs; // (BCA::v, BCB::w)
	world.for_each<BCA, BCB>([&](BCA& a, BCB& b) { pairs.push_back({ a.v, b.w }); });
	std::sort(pairs.begin(), pairs.end());
	REQUIRE(pairs.size() == 4u);
	CHECK(pairs[0] == std::pair<int, int> { 10, 100 });
	CHECK(pairs[1] == std::pair<int, int> { 12, 102 });
	CHECK(pairs[2] == std::pair<int, int> { 20, 200 });
	CHECK(pairs[3] == std::pair<int, int> { 31, 301 });

	int total = 0;
	world.for_each<BCA>([&](BCA&) { ++total; });
	CHECK(total == 6);
}

TEST_CASE("merge_from rebases bulk placeholder ranges across buffers", "[ecs][BulkCreate][deferred][merge]") {
	World world;
	CommandBuffer chunk_a;
	CommandBuffer chunk_b;
	chunk_a.set_parallel_mode(true);
	chunk_b.set_parallel_mode(true);

	// Each buffer records bulk(2) + a single + adds referencing its own placeholders. Without
	// correct range rebasing, chunk B's adds would land on chunk A's entities.
	std::vector<BCA> a_vals { BCA { 10 }, BCA { 11 } };
	std::vector<EntityID> a_bulk(2);
	REQUIRE(chunk_a.create_entities<BCA>(world, 2, a_bulk, a_vals));
	EntityID const a_single = chunk_a.create_entity(world, BCA { 12 });
	chunk_a.add_component<BCB>(a_bulk[1], BCB { 110 });
	chunk_a.add_component<BCB>(a_single, BCB { 120 });

	std::vector<BCA> b_vals { BCA { 20 }, BCA { 21 } };
	std::vector<EntityID> b_bulk(2);
	REQUIRE(chunk_b.create_entities<BCA>(world, 2, b_bulk, b_vals));
	EntityID const b_single = chunk_b.create_entity(world, BCA { 22 });
	chunk_b.add_component<BCB>(b_bulk[0], BCB { 200 });
	chunk_b.add_component<BCB>(b_single, BCB { 220 });

	CommandBuffer system_pending;
	system_pending.merge_from(std::move(chunk_a));
	system_pending.merge_from(std::move(chunk_b));
	CHECK(system_pending.deferred_count() == 6u);

	system_pending.apply(world);

	std::vector<std::pair<int, int>> pairs;
	world.for_each<BCA, BCB>([&](BCA& a, BCB& b) { pairs.push_back({ a.v, b.w }); });
	std::sort(pairs.begin(), pairs.end());
	REQUIRE(pairs.size() == 4u);
	CHECK(pairs[0] == std::pair<int, int> { 11, 110 });
	CHECK(pairs[1] == std::pair<int, int> { 12, 120 });
	CHECK(pairs[2] == std::pair<int, int> { 20, 200 });
	CHECK(pairs[3] == std::pair<int, int> { 22, 220 });

	int total = 0;
	world.for_each<BCA>([&](BCA&) { ++total; });
	CHECK(total == 6);
}

TEST_CASE("CB bulk yields identical ids and end state to the single-create loop", "[ecs][BulkCreate][determinism]") {
	std::size_t const count = 50;

	// Serial recording mode.
	{
		World loop_world;
		std::vector<EntityID> loop_ids;
		{
			CommandBuffer cmd;
			for (std::size_t i = 0; i < count; ++i) {
				loop_ids.push_back(cmd.create_entity(loop_world, BCA { static_cast<int>(i) }));
			}
			cmd.apply(loop_world);
		}

		World bulk_world;
		std::vector<EntityID> bulk_ids(count);
		{
			CommandBuffer cmd;
			std::vector<BCA> as;
			for (std::size_t i = 0; i < count; ++i) {
				as.push_back(BCA { static_cast<int>(i) });
			}
			REQUIRE(cmd.create_entities<BCA>(bulk_world, count, bulk_ids, as));
			cmd.apply(bulk_world);
		}

		for (std::size_t i = 0; i < count; ++i) {
			CHECK(bulk_ids[i] == loop_ids[i]);
		}
		CHECK(collect_bca(loop_world) == collect_bca(bulk_world));
	}

	// Parallel recording mode (placeholders resolved at apply).
	{
		World loop_world;
		{
			CommandBuffer cmd;
			cmd.set_parallel_mode(true);
			for (std::size_t i = 0; i < count; ++i) {
				cmd.create_entity(loop_world, BCA { static_cast<int>(i) });
			}
			cmd.set_parallel_mode(false);
			cmd.apply(loop_world);
		}

		World bulk_world;
		{
			CommandBuffer cmd;
			cmd.set_parallel_mode(true);
			std::vector<BCA> as;
			for (std::size_t i = 0; i < count; ++i) {
				as.push_back(BCA { static_cast<int>(i) });
			}
			std::vector<EntityID> phs(count);
			REQUIRE(cmd.create_entities<BCA>(bulk_world, count, phs, as));
			cmd.set_parallel_mode(false);
			cmd.apply(bulk_world);
		}

		CHECK(collect_bca(loop_world) == collect_bca(bulk_world));
	}
}

TEST_CASE("Immutable bulk create stamps every slot on both paths", "[ecs][BulkCreate][immutable]") {
	std::size_t const count = 8;

	// Direct path.
	{
		World world;
		std::vector<BCA> as;
		for (std::size_t i = 0; i < count; ++i) {
			as.push_back(BCA { static_cast<int>(i) });
		}
		std::vector<ImmutableEntityID> ids(count);
		REQUIRE(world.create_immutable_entities<BCA>(count, ids, as));
		for (ImmutableEntityID const id : ids) {
			CHECK(world.is_alive(id));
			CHECK(world.is_immutable(id));
		}
		// Runtime backstop: structural mutation through a laundered EntityID is refused
		// (logs an error by design).
		CHECK(world.add_component<BCB>(ids[0].unsafe_mutable_id(), BCB { 1 }) == nullptr);
	}

	// CommandBuffer path (parallel mode, so placeholders + apply-time stamping).
	{
		World world;
		CommandBuffer cmd;
		cmd.set_parallel_mode(true);
		std::vector<BCA> as;
		for (std::size_t i = 0; i < count; ++i) {
			as.push_back(BCA { static_cast<int>(i) });
		}
		std::vector<ImmutableEntityID> phs(count);
		REQUIRE(cmd.create_immutable_entities<BCA>(world, count, phs, as));
		for (ImmutableEntityID const ph : phs) {
			CHECK(ph.is_deferred());
		}
		cmd.set_parallel_mode(false);
		cmd.apply(world);

		int found = 0;
		world.for_each_with_entity<BCA>([&](EntityID e, BCA&) {
			++found;
			CHECK(world.is_immutable(e));
		});
		CHECK(found == static_cast<int>(count));
	}
}

TEST_CASE("Bulk-created entities destroy normally and bump column versions", "[ecs][BulkCreate]") {
	World world;
	std::size_t const count = 20;
	std::vector<BCA> as;
	for (std::size_t i = 0; i < count; ++i) {
		as.push_back(BCA { static_cast<int>(i) });
	}
	std::vector<EntityID> ids(count);
	REQUIRE(world.create_entities<BCA>(count, ids, as));

	uint64_t const version_before = world.component_version_in<BCA>(ids[0]);
	CHECK(version_before > 0u);

	// A later bulk append to the same archetype bumps the column version (CachedRef
	// revalidation contract).
	std::vector<BCA> more { BCA { 100 }, BCA { 101 } };
	std::vector<EntityID> more_ids(2);
	REQUIRE(world.create_entities<BCA>(2, more_ids, more));
	CHECK(world.component_version_in<BCA>(ids[0]) > version_before);

	// Destroy half (head-first to force swap-pops across the bulk-created rows).
	for (std::size_t i = 0; i < count / 2; ++i) {
		world.destroy_entity(ids[i]);
	}
	for (std::size_t i = 0; i < count; ++i) {
		CHECK(world.is_alive(ids[i]) == (i >= count / 2));
	}
	int found = 0;
	world.for_each<BCA>([&](BCA&) { ++found; });
	CHECK(found == static_cast<int>(count / 2 + 2));
}

TEST_CASE("snapshot_identity refuses while a serial bulk create is un-applied", "[ecs][BulkCreate][identity]") {
	World world;
	CommandBuffer cmd;
	std::vector<BCA> as { BCA { 1 }, BCA { 2 } };
	std::vector<EntityID> ids(2);
	REQUIRE(cmd.create_entities<BCA>(world, 2, ids, as));

	// The reserved-but-unfinalised slots must block an identity snapshot (logs an error).
	WorldIdentitySnapshot snapshot;
	CHECK_FALSE(world.snapshot_identity(snapshot));

	cmd.apply(world);
	CHECK(world.snapshot_identity(snapshot));
	CHECK(snapshot.slots.size() == 2u);
}

// === Worker-count invariance (the multiplayer-determinism merge gate) ===
// A SystemThreaded spawner recording cmd.create_entities must produce a bit-identical
// post-tick state at any worker count, AND identical to the equivalent single-create
// spawner — the direct test that bulk adoption is never a simulation-behavior change.

namespace {
	struct BCSeed {
		int64_t seed = 0;
	};
	struct BCSpawned {
		int64_t derived = 0;
	};
}
ECS_COMPONENT(BCSeed, "test_BulkCreate::BCSeed")
ECS_COMPONENT(BCSpawned, "test_BulkCreate::BCSpawned")

namespace {
	struct BCBulkSpawner : SystemThreaded<BCBulkSpawner> {
		void tick(TickContext const& ctx, EntityID, BCSeed const& s) {
			std::array<BCSpawned, 3> vals {
				BCSpawned { s.seed * 31 + 1 },
				BCSpawned { s.seed * 31 + 2 },
				BCSpawned { s.seed * 31 + 3 }
			};
			std::array<EntityID, 3> out {};
			ctx.cmd.create_entities<BCSpawned>(ctx.world, 3, out, vals);
		}
	};

	struct BCLoopSpawner : SystemThreaded<BCLoopSpawner> {
		void tick(TickContext const& ctx, EntityID, BCSeed const& s) {
			ctx.cmd.create_entity(ctx.world, BCSpawned { s.seed * 31 + 1 });
			ctx.cmd.create_entity(ctx.world, BCSpawned { s.seed * 31 + 2 });
			ctx.cmd.create_entity(ctx.world, BCSpawned { s.seed * 31 + 3 });
		}
	};
}
ECS_SYSTEM(BCBulkSpawner)
ECS_SYSTEM(BCLoopSpawner)

namespace {
	template<typename SpawnerT>
	int64_t bulk_spawn_and_digest(uint32_t worker_count, std::size_t seed_count, int tick_count) {
		World world;
		world.set_ecs_worker_count(worker_count);

		for (std::size_t i = 0; i < seed_count; ++i) {
			world.create_entity(BCSeed { static_cast<int64_t>((i * 17) % 251 + 1) });
		}

		world.register_system<SpawnerT>();

		for (int t = 0; t < tick_count; ++t) {
			world.tick_systems(Date {});
		}

		// Digest includes EntityIDs — equality across spawners proves identical id
		// assignment, not just identical values.
		int64_t digest = 0;
		world.for_each_with_entity<BCSpawned>([&](EntityID e, BCSpawned& s) {
			digest = digest * 1000003 + s.derived;
			digest ^= static_cast<int64_t>(e.to_uint64());
		});
		return digest;
	}
}

TEST_CASE("Threaded bulk spawner: digest is identical across worker counts",
          "[ecs][determinism][BulkCreate][deferred]") {
	std::size_t const seeds = 300;
	int const ticks = 1;
	int64_t const baseline = bulk_spawn_and_digest<BCBulkSpawner>(1, seeds, ticks);

	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		int64_t const result = bulk_spawn_and_digest<BCBulkSpawner>(wc, seeds, ticks);
		CHECK(result == baseline);
	}
}

TEST_CASE("Threaded bulk spawner produces the identical end state to the single-create spawner",
          "[ecs][determinism][BulkCreate][deferred]") {
	std::size_t const seeds = 300;
	for (int ticks : { 1, 3 }) {
		int64_t const loop_digest = bulk_spawn_and_digest<BCLoopSpawner>(4, seeds, ticks);
		int64_t const bulk_digest = bulk_spawn_and_digest<BCBulkSpawner>(4, seeds, ticks);
		CHECK(bulk_digest == loop_digest);
	}
}
