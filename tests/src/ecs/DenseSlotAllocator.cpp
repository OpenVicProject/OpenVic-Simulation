#include "openvic-simulation/ecs/DenseSlotAllocator.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

// === Deterministic dense-slot allocator for singleton side tables ===
// Allocation order is a pure function of the alloc/release call sequence: LIFO reuse of
// released slots, then high-water growth. See the header for the determinism contract
// (serial-only alloc/release) and the snapshot/restore mirror of World's identity layer.

TEST_CASE("dense growth then LIFO reuse, exact sequence", "[ecs][DenseSlotAllocator]") {
	DenseSlotAllocator alloc;
	for (uint32_t expected = 0; expected < 5; ++expected) {
		CHECK(alloc.allocate() == expected);
	}
	CHECK(alloc.high_water() == 5);

	alloc.release(2);
	alloc.release(0);
	CHECK(alloc.free_count() == 2);

	CHECK(alloc.allocate() == 0); // LIFO: last released first
	CHECK(alloc.allocate() == 2);
	CHECK(alloc.allocate() == 5); // free stack drained — high-water growth
	CHECK(alloc.high_water() == 6);
	CHECK(alloc.free_count() == 0);
}

TEST_CASE("snapshot/restore continues exactly as the never-saved run", "[ecs][DenseSlotAllocator]") {
	DenseSlotAllocator original;
	for (int i = 0; i < 8; ++i) {
		(void) original.allocate();
	}
	original.release(3);
	original.release(6);
	original.release(1);
	(void) original.allocate(); // 1 — leaves [3, 6] on the stack
	original.release(7);

	DenseSlotAllocator::Snapshot snap;
	original.snapshot(snap);

	DenseSlotAllocator restored;
	REQUIRE(restored.restore(snap));

	// Identical further script on both: drains the free stack, grows fresh, releases again.
	struct Script {
		static std::vector<uint32_t> run(DenseSlotAllocator& alloc) {
			std::vector<uint32_t> out;
			for (int i = 0; i < 5; ++i) {
				out.push_back(alloc.allocate());
			}
			alloc.release(out[2]);
			alloc.release(out[0]);
			for (int i = 0; i < 4; ++i) {
				out.push_back(alloc.allocate());
			}
			return out;
		}
	};

	std::vector<uint32_t> const continued = Script::run(original);
	std::vector<uint32_t> const replayed = Script::run(restored);
	CHECK(continued == replayed);

	DenseSlotAllocator::Snapshot end_original;
	DenseSlotAllocator::Snapshot end_restored;
	original.snapshot(end_original);
	restored.snapshot(end_restored);
	CHECK(end_original == end_restored);

	// Restore into a reset (non-fresh) allocator also works.
	DenseSlotAllocator reused;
	(void) reused.allocate();
	reused.reset();
	CHECK(reused.restore(snap));
}

TEST_CASE("restore validates and leaves state untouched on failure", "[ecs][DenseSlotAllocator]") {
	DenseSlotAllocator alloc;
	(void) alloc.allocate();
	(void) alloc.allocate();

	{
		// Free slot >= next_unallocated.
		DenseSlotAllocator::Snapshot bad;
		bad.next_unallocated = 4;
		bad.free_slots = { 1, 4 };
		CHECK_FALSE(alloc.restore(bad));
		CHECK(alloc.high_water() == 2);
		CHECK(alloc.free_count() == 0);
	}
	{
		// Duplicate free slot.
		DenseSlotAllocator::Snapshot bad;
		bad.next_unallocated = 4;
		bad.free_slots = { 1, 1 };
		CHECK_FALSE(alloc.restore(bad));
		CHECK(alloc.high_water() == 2);
	}
	{
		// Valid snapshot restores.
		DenseSlotAllocator::Snapshot good;
		good.next_unallocated = 4;
		good.free_slots = { 1, 3 };
		REQUIRE(alloc.restore(good));
		CHECK(alloc.allocate() == 3); // back() of the restored stack
		CHECK(alloc.allocate() == 1);
		CHECK(alloc.allocate() == 4);
	}
}

TEST_CASE("reset forgets everything (the end_game_session sweep)", "[ecs][DenseSlotAllocator]") {
	DenseSlotAllocator alloc;
	for (int i = 0; i < 6; ++i) {
		(void) alloc.allocate();
	}
	alloc.release(4);
	alloc.release(2);

	alloc.reset();
	CHECK(alloc.high_water() == 0);
	CHECK(alloc.free_count() == 0);
	CHECK(alloc.allocate() == 0);
	CHECK(alloc.high_water() == 1);

	DenseSlotAllocator::Snapshot snap;
	alloc.snapshot(snap);
	CHECK(snap.next_unallocated == 1);
	CHECK(snap.free_slots.empty());
}

TEST_CASE("release range-check ignores never-allocated slots; debug_validate catches double release",
          "[ecs][DenseSlotAllocator]") {
	DenseSlotAllocator alloc;
	for (int i = 0; i < 3; ++i) {
		(void) alloc.allocate();
	}

	alloc.release(99); // never allocated — logged and ignored
	CHECK(alloc.free_count() == 0);
	CHECK(alloc.debug_validate());

	alloc.release(1);
	CHECK(alloc.debug_validate());
	alloc.release(1); // contract violation — not caught per-release...
	CHECK_FALSE(alloc.debug_validate()); // ...but visible to the one-shot invariant check
}
