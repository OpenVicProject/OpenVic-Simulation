#include "openvic-simulation/ecs/ChunkPool.hpp"

#include <cstddef>
#include <new>

#include "openvic-simulation/ecs/Chunk.hpp"

using namespace OpenVic::ecs;

ChunkPool::~ChunkPool() {
	for (PooledBlock const& blk : free_blocks_) {
		::operator delete(blk.data, std::align_val_t { CHUNK_BLOCK_ALIGN });
		++total_deallocations_;
	}
	free_blocks_.clear();
}

unsigned char* ChunkPool::acquire() {
	if (!free_blocks_.empty()) {
		unsigned char* data = free_blocks_.back().data;
		free_blocks_.pop_back();
		return data;
	}
	++total_allocations_;
	return static_cast<unsigned char*>(
		::operator new(CHUNK_BLOCK_BYTES, std::align_val_t { CHUNK_BLOCK_ALIGN })
	);
}

void ChunkPool::release(unsigned char* data) {
	if (data == nullptr) {
		return;
	}
	if (free_blocks_.size() >= MAX_POOL_SIZE) {
		::operator delete(data, std::align_val_t { CHUNK_BLOCK_ALIGN });
		++total_deallocations_;
		return;
	}
	free_blocks_.push_back({ data, current_tick_ });
}

void ChunkPool::advance_tick() {
	++current_tick_;
	// Swap-pop blocks older than the threshold. free_blocks_.size() is bounded by
	// MAX_POOL_SIZE, so the O(n) scan is trivial.
	std::size_t i = 0;
	while (i < free_blocks_.size()) {
		// current_tick_ - released_at_tick > AGE_THRESHOLD_TICKS
		// released_at_tick <= current_tick_ by construction, so subtraction is safe.
		if (current_tick_ - free_blocks_[i].released_at_tick > AGE_THRESHOLD_TICKS) {
			::operator delete(free_blocks_[i].data, std::align_val_t { CHUNK_BLOCK_ALIGN });
			++total_deallocations_;
			free_blocks_[i] = free_blocks_.back();
			free_blocks_.pop_back();
		} else {
			++i;
		}
	}
}
