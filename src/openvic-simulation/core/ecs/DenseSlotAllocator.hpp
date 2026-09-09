#pragma once

#include <cstdint>
#include <vector>

namespace OpenVic::ecs {

	// Sentinel returned by DenseSlotAllocator::allocate on exhaustion (all 2^32 - 1 rows used).
	inline constexpr uint32_t INVALID_DENSE_SLOT = static_cast<uint32_t>(-1);

	// Deterministic free-list allocator handing out dense uint32_t rows into singleton side
	// tables (politics columns, artisan stockpiles — ECS_SIM_ARCHITECTURE §3.2 / §9 item 3).
	// NOT an entity allocator: there are no generations — the durable identity of a row's owner
	// is that entity's EntityID; the row is just packed storage.
	//
	// Determinism contract (lockstep multiplayer): the allocation order is a pure function of
	// the alloc/release call sequence — LIFO reuse of released slots, then high-water growth.
	// Therefore allocate/release may be called ONLY from serial systems (System<> tick bodies),
	// from CommandBuffer-apply-adjacent serial code, or outside ticks — NEVER from SystemThreaded
	// tick bodies, whose execution order is worker-count-dependent. Same discipline as the
	// deferred-create path: structural decisions funnel through a serial point.
	//
	// Release is explicit at the callsite (house rule: no RAII handles — audit-ability wins) and
	// must happen exactly once per live slot. Double release is a contract violation: it is NOT
	// detected per-release (an O(n) scan would make mass end-of-session sweeps O(n^2)); tests and
	// debug sweeps can call debug_validate() for a one-shot O(n log n) invariant check.
	//
	// snapshot/restore mirror World::snapshot_identity / restore_identity: snapshot between
	// ticks, restore into a fresh (or reset) allocator, and subsequent allocations continue
	// exactly as in the never-saved run. Plain serializable struct, no IO/format here.
	// reset() is the end_game_session sweep for the owning side table.
	struct DenseSlotAllocator {
		// Plain serializable image of the allocator's full state.
		struct Snapshot {
			uint32_t next_unallocated = 0;
			// LIFO stack order: back() is the next allocate() result.
			std::vector<uint32_t> free_slots;

			bool operator==(Snapshot const&) const = default;
		};

		// Next slot: top of the free stack if any, else high-water growth. Returns
		// INVALID_DENSE_SLOT (+ error log) on exhaustion.
		uint32_t allocate();

		// Explicit release — call at the callsite, exactly once per live slot (see the contract
		// above). Releasing a slot >= high_water() is logged and ignored (O(1) range check);
		// double-releasing a valid slot is detected only by debug_validate().
		void release(uint32_t slot);

		// Rows ever allocated == the size the owning side table must reserve. Never shrinks
		// except via reset() / restore().
		uint32_t high_water() const {
			return next_unallocated_;
		}

		uint32_t free_count() const {
			return static_cast<uint32_t>(free_slots_.size());
		}

		// Forget everything — the end_game_session sweep for the owning side table.
		void reset();

		// Cannot fail.
		void snapshot(Snapshot& out) const;

		// Validates (every free slot < next_unallocated, no duplicates); on failure returns
		// false + error log and *this is untouched.
		bool restore(Snapshot const& snapshot);

		// One-shot invariant check for tests / debug sweeps: every free slot is in range and
		// appears exactly once. O(n log n).
		bool debug_validate() const;

	private:
		std::vector<uint32_t> free_slots_;
		uint32_t next_unallocated_ = 0;
	};
}
