#pragma once

#include <atomic>
#include <cstddef>

#include <foonathan/memory/tracking.hpp>

namespace OpenVic::utility {
	struct MemoryTracker {
		void on_node_allocation(void* mem, std::size_t size, std::size_t) {
			uint64_t new_mem_usage = _memory_usage.fetch_add(size, std::memory_order_acq_rel) + size;
			[&] {
				while (true) {
					uint64_t tmp = _max_memory_usage.load(std::memory_order_acquire);
					if (tmp >= new_mem_usage) {
						return tmp; // already greater, or equal
					}

					if (_max_memory_usage.compare_exchange_weak(tmp, new_mem_usage, std::memory_order_acq_rel)) {
						return new_mem_usage;
					}
				}
			}();
		}

		void on_array_allocation(void* mem, std::size_t count, std::size_t size, std::size_t alignment) {
			on_node_allocation(mem, size * count, alignment);
		}

		void on_node_deallocation(void* ptr, std::size_t size, std::size_t) {
			_memory_usage.fetch_sub(size, std::memory_order_acq_rel);
		}

		void on_array_deallocation(void* ptr, std::size_t count, std::size_t size, std::size_t alignment) {
			on_node_deallocation(ptr, count * size, alignment);
		}

		static uint64_t get_memory_usage() {
			return _memory_usage.load(std::memory_order_acquire);
		}

		static uint64_t get_max_memory_usage() {
			return _max_memory_usage.load(std::memory_order_acquire);
		}

	private:
		inline static std::atomic_uint64_t _memory_usage = 0;
		inline static std::atomic_uint64_t _max_memory_usage = 0;
	};
}

namespace OpenVic::memory {
	template<class RawAllocator>
	using tracker =
#ifdef DEBUG_ENABLED
		foonathan::memory::tracked_allocator<OpenVic::utility::MemoryTracker, RawAllocator>
#else
		RawAllocator
#endif
		;
}
