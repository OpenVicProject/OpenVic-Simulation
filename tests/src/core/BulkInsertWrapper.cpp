#include <cstddef>
#include <memory>
#include <vector>

#include "openvic-simulation/core/BulkInsertWrapper.hpp"

#include <snitch/snitch.hpp>

#include "SpyAllocator.hpp"

// A simple non-trivially destructible type to test constraint violations if needed, 
// and a trivial one to satisfy emplace_back's requires clause.
struct TrivialPoint {
    int x = 0;
    int y = 0;
};

using namespace OpenVic;

TEST_CASE("bulk_insert_wrapper Constructors", "[bulk_insert_wrapper][bulk_insert_wrapper-constructor]") {
	constexpr bulk_insert_wrapper<std::vector<int>> empty;
	CONSTEXPR_CHECK(empty.empty());
	CONSTEXPR_CHECK(empty.size() == 0);
	CONSTEXPR_CHECK(empty.capacity() == 0);
    CONSTEXPR_CHECK(empty.begin() == empty.end());
    CONSTEXPR_CHECK(empty.cbegin() == empty.cend());
    CONSTEXPR_CHECK(empty.rbegin() == empty.rend());

	constexpr std::size_t expected_size = 3;
	bulk_insert_wrapper<std::vector<int>> filled {
		std::vector<int> { 1, 2, 3 }
	};
	CHECK(!filled.empty());
	CHECK(filled.size() == expected_size);
	CHECK(filled.capacity() >= expected_size);
    CHECK(std::distance(filled.begin(), filled.end()) == expected_size);
    CHECK(std::distance(filled.cbegin(), filled.cend()) == expected_size);
    CHECK(std::distance(filled.rbegin(), filled.rend()) == expected_size);
    CHECK(filled[0] == 1);
}

TEST_CASE("bulk_insert_wrapper make_room does not allocate", "[bulk_insert_wrapper][bulk_insert_wrapper-make_room]") {
	SpyAllocator<int> spy_allocator{};
	bulk_insert_wrapper<
		std::vector<
			int,
			SpyAllocator<int>
		>
	> empty { spy_allocator };
	empty.make_room_for(10);
	
	CHECK(empty.empty());
	CHECK(empty.size() == 0);
	CHECK(empty.capacity() == 0);
    CHECK(empty.begin() == empty.end());
    CHECK(empty.cbegin() == empty.cend());
    CHECK(empty.rbegin() == empty.rend());
	CHECK(spy_allocator.metrics->allocation_count == 0);
}

// correct usage
TEST_CASE("bulk_insert_wrapper make_room + append_range", "[bulk_insert_wrapper][bulk_insert_wrapper-append_range]") {
	SpyAllocator<int> spy_allocator{};
	bulk_insert_wrapper<
		std::vector<
			int,
			SpyAllocator<int>
		>
	> wrapper { spy_allocator };

	std::vector<int> a { 1, 2 };
	std::vector<int> b { 3, 4, 5 };

	wrapper.make_room_for(a.size());
	wrapper.make_room_for(b.size());

	wrapper.append_range(a);
	CHECK(wrapper.size() == a.size());
	CHECK(wrapper.capacity() >= a.size() + b.size());

	wrapper.append_range(b);

	CHECK(wrapper.size() == a.size() + b.size());
	CHECK(spy_allocator.metrics->allocation_count == 1);
}

// cassert prevents testing in debug
#ifdef NDEBUG
// incorrect usage may not crash
TEST_CASE("bulk_insert_wrapper append_range without make_room", "[bulk_insert_wrapper][bulk_insert_wrapper-append_range]") {
	SpyAllocator<int> spy_allocator{};
	bulk_insert_wrapper<
		std::vector<
			int,
			SpyAllocator<int>
		>
	> wrapper { spy_allocator };

	std::vector<int> a { 1, 2 };
	std::vector<int> b { 3, 4, 5 };

	wrapper.append_range(a);
	wrapper.append_range(b);

	CHECK(wrapper.size() == a.size() + b.size());
	CHECK(wrapper.capacity() >= a.size() + b.size());
	CHECK(spy_allocator.metrics->allocation_count <= 2);
}
#endif

TEST_CASE("bulk_insert_wrapper clear", "[bulk_insert_wrapper][bulk_insert_wrapper-clear]") {
	SpyAllocator<int> spy_allocator{};
	bulk_insert_wrapper<
		std::vector<
			int,
			SpyAllocator<int>
		>
	> wrapper { spy_allocator };

	std::vector<int> a { 1, 2 };
	constexpr std::size_t extra_room = 1;

	wrapper.make_room_for(a.size());
	wrapper.make_room_for(extra_room);

	wrapper.append_range(a);
	CHECK(wrapper.size() == a.size());
	CHECK(wrapper.capacity() >= a.size() + extra_room);

	wrapper.clear();
	CHECK(wrapper.empty());
	CHECK(spy_allocator.metrics->allocation_count == 1);
}

TEST_CASE("bulk_insert_wrapper shrink_to_fit", "[bulk_insert_wrapper][bulk_insert_wrapper-shrink_to_fit]") {
	SpyAllocator<int> spy_allocator{};
	bulk_insert_wrapper<
		std::vector<
			int,
			SpyAllocator<int>
		>
	> wrapper { spy_allocator };

	std::vector<int> a { 1, 2 };
	constexpr std::size_t extra_room = 1;

	wrapper.make_room_for(a.size());
	wrapper.make_room_for(extra_room);

	wrapper.append_range(a);
	CHECK(wrapper.size() == a.size());
	const std::size_t capacity_before_shrink = wrapper.capacity();
	CHECK(capacity_before_shrink >= a.size() + extra_room);

	wrapper.shrink_to_fit();
	CHECK(wrapper.size() == a.size());
	// shrink_to_fit() is non-binding, it may or may not actually shrink depending on underlying container implementation.
	// just ensure we don't expand or allocate too often somehow
	CHECK(wrapper.capacity() <= capacity_before_shrink);
	CHECK(spy_allocator.metrics->allocation_count >= 1);
	CHECK(spy_allocator.metrics->allocation_count <= 2);
}