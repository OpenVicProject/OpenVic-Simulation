#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace OpenVic::ecs {

	// Pool of fixed-size 16 KB aligned blocks matching DataChunk's layout (size = CHUNK_BLOCK_BYTES,
	// alignment = CHUNK_BLOCK_ALIGN — see Chunk.hpp). Owned by World; single-threaded — structural
	// mutations are serialised on the main tick thread, so no synchronisation here.
	//
	// Released blocks are pushed LIFO so a ping-pong archetype reuses warm memory. Aging policy:
	// blocks whose release tick falls more than AGE_THRESHOLD_TICKS behind the current tick are
	// freed on the next advance_tick. A working set that keeps acquiring + releasing every tick
	// refreshes its released_at_tick on each cycle and never ages out. A truly idle archetype's
	// chunks all drain to the OS after AGE_THRESHOLD_TICKS ticks of disuse.
	//
	// MAX_POOL_SIZE caps the cached block count. Releases above the cap go straight to
	// ::operator delete so a one-off burst can't lock down megabytes for the aging window.
	class ChunkPool {
	public:
		static constexpr std::size_t MAX_POOL_SIZE = 64;
		static constexpr uint64_t AGE_THRESHOLD_TICKS = 256;

		ChunkPool() = default;
		ChunkPool(ChunkPool const&) = delete;
		ChunkPool& operator=(ChunkPool const&) = delete;
		ChunkPool(ChunkPool&&) = delete;
		ChunkPool& operator=(ChunkPool&&) = delete;
		~ChunkPool();

		// Returns a CHUNK_BLOCK_BYTES-sized, CHUNK_BLOCK_ALIGN-aligned block. Pops from the
		// free list if any block is cached; otherwise calls ::operator new and increments
		// total_allocations_.
		unsigned char* acquire();

		// Returns a block to the pool. If the free list is at MAX_POOL_SIZE, frees the block
		// immediately via ::operator delete and increments total_deallocations_. Passing
		// nullptr is a no-op.
		void release(unsigned char* data);

		// Increments the tick counter and frees any cached block whose release tick is
		// older than AGE_THRESHOLD_TICKS. Called once per World tick from tick_systems.
		void advance_tick();

		// Test / diagnostic accessors. Used by ChunkPool tests to assert pool behaviour and
		// by integration tests to verify allocator round-trips through the pool.
		std::size_t pooled_count() const {
			return free_blocks_.size();
		}
		uint64_t total_allocations() const {
			return total_allocations_;
		}
		uint64_t total_deallocations() const {
			return total_deallocations_;
		}
		uint64_t current_tick() const {
			return current_tick_;
		}

	private:
		struct PooledBlock {
			unsigned char* data;
			uint64_t released_at_tick;
		};

		std::vector<PooledBlock> free_blocks_;
		uint64_t current_tick_ = 0;
		uint64_t total_allocations_ = 0;
		uint64_t total_deallocations_ = 0;
	};
}
