#include <cstddef>
#include <memory>

// Telemetry structure remains the same
struct AllocationMetrics {
    std::size_t total_allocated_bytes = 0;
    std::size_t total_deallocated_bytes = 0;
    std::size_t allocation_count = 0;
    std::size_t deallocation_count = 0;

    void reset() {
        total_allocated_bytes = 0;
        total_deallocated_bytes = 0;
        allocation_count = 0;
        deallocation_count = 0;
    }

    std::size_t active_bytes() const {
        return total_allocated_bytes - total_deallocated_bytes;
    }
};

// SpyAllocator now wraps an underlying Allocator type (defaults to std::allocator)
template <typename T, typename Allocator = std::allocator<T>>
class SpyAllocator {
public:
	using value_type = T;
	
	// Shared telemetry state across allocator copies
	std::shared_ptr<AllocationMetrics> metrics;
	// The actual allocator doing the heavy lifting
	Allocator upstream;

	// Default Constructor
	SpyAllocator() 
		: metrics(std::make_shared<AllocationMetrics>()), upstream() {}

	// Explicitly pass an existing upstream allocator instance if needed
	explicit SpyAllocator(const Allocator& alloc) 
		: metrics(std::make_shared<AllocationMetrics>()), upstream(alloc) {}

	value_type* allocate(const std::size_t n) {
		// Delegate allocation to the upstream allocator
		value_type* ptr = std::allocator_traits<Allocator>::allocate(upstream, n);
		
		if (metrics && ptr) {
			metrics->total_allocated_bytes += (n * sizeof(value_type));
			metrics->allocation_count++;
		}
		return ptr;
	}

	void deallocate(value_type* const ptr, const std::size_t n) noexcept {
		if (metrics) {
			metrics->total_deallocated_bytes += (n * sizeof(value_type));
			metrics->deallocation_count++;
		}
		// Delegate deallocation to the upstream allocator
		std::allocator_traits<Allocator>::deallocate(upstream, ptr, n);
	}
};