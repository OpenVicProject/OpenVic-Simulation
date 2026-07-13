#include "openvic-simulation/ecs/DenseSlotAllocator.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic::ecs;

uint32_t DenseSlotAllocator::allocate() {
	if (!free_slots_.empty()) {
		uint32_t const slot = free_slots_.back();
		free_slots_.pop_back();
		return slot;
	}
	if (next_unallocated_ == INVALID_DENSE_SLOT) {
		spdlog::error_s("DenseSlotAllocator::allocate failed: all {} rows exhausted", INVALID_DENSE_SLOT);
		return INVALID_DENSE_SLOT;
	}
	return next_unallocated_++;
}

void DenseSlotAllocator::release(uint32_t slot) {
	if (slot >= next_unallocated_) {
		spdlog::error_s(
			"DenseSlotAllocator::release ignored: slot {} was never allocated (high water {})",
			slot, next_unallocated_
		);
		return;
	}
	free_slots_.push_back(slot);
}

void DenseSlotAllocator::reset() {
	free_slots_.clear();
	next_unallocated_ = 0;
}

void DenseSlotAllocator::snapshot(Snapshot& out) const {
	out.next_unallocated = next_unallocated_;
	out.free_slots = free_slots_;
}

bool DenseSlotAllocator::restore(Snapshot const& snapshot) {
	std::vector<uint32_t> sorted = snapshot.free_slots;
	std::sort(sorted.begin(), sorted.end());
	for (std::size_t i = 0; i < sorted.size(); ++i) {
		if (sorted[i] >= snapshot.next_unallocated) {
			spdlog::error_s(
				"DenseSlotAllocator::restore refused: free slot {} >= next_unallocated {}",
				sorted[i], snapshot.next_unallocated
			);
			return false;
		}
		if (i > 0 && sorted[i] == sorted[i - 1]) {
			spdlog::error_s("DenseSlotAllocator::restore refused: duplicate free slot {}", sorted[i]);
			return false;
		}
	}
	next_unallocated_ = snapshot.next_unallocated;
	free_slots_ = snapshot.free_slots;
	return true;
}

bool DenseSlotAllocator::debug_validate() const {
	std::vector<uint32_t> sorted = free_slots_;
	std::sort(sorted.begin(), sorted.end());
	for (std::size_t i = 0; i < sorted.size(); ++i) {
		if (sorted[i] >= next_unallocated_) {
			return false;
		}
		if (i > 0 && sorted[i] == sorted[i - 1]) {
			return false;
		}
	}
	return true;
}
