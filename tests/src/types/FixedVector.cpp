#include "openvic-simulation/types/FixedVector.hpp"

#include <string>
#include <tuple>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::_detail;

// A simple test type to demonstrate a more complex object.
// It has a multi-argument constructor to test emplace_back.
struct ComplexType {
public:
    int index;
    std::string name;

    ComplexType(int new_index, std::string name_val) : index {new_index}, name(std::move(name_val)) {}

    // Equality operator for assertions
    bool operator==(const ComplexType& other) const {
        return index == other.index && name == other.name;
    }
};

// A type that is explicitly non-copyable and non-movable.
struct NonCopyableNonMovable {
public:
    int value;

    // A simple constructor
    NonCopyableNonMovable(int new_value) : value {new_value} {}

    // Explicitly delete copy and move operations
    NonCopyableNonMovable(const NonCopyableNonMovable&) = delete;
    NonCopyableNonMovable& operator=(const NonCopyableNonMovable&) = delete;
    NonCopyableNonMovable(NonCopyableNonMovable&&) = delete;
    NonCopyableNonMovable& operator=(NonCopyableNonMovable&&) = delete;

    bool operator==(const NonCopyableNonMovable& other) const {
        return value == other.value;
    }
};

// A custom type to verify constructors and destructors are called correctly.
struct DestructionCounter {
public:
    static int constructor_count;
    static int destructor_count;

    DestructionCounter() { constructor_count++; }
    ~DestructionCounter() { destructor_count++; }

    // Must be non-copyable/non-movable to be safe to use in a fixed vector
    // and for better testing of in-place construction.
    DestructionCounter(const DestructionCounter&) = delete;
    DestructionCounter(DestructionCounter&&) = delete;
    DestructionCounter& operator=(const DestructionCounter&) = delete;
    DestructionCounter& operator=(DestructionCounter&&) = delete;
};

// Define the static member variables outside the class.
// This is necessary to allocate storage for them and prevent the linker error.
int DestructionCounter::constructor_count = 0;
int DestructionCounter::destructor_count = 0;

// A simple test case to check the basic constructor and accessors with a simple type.
TEST_CASE("FixedVector Construction and basic accessors","[FixedVector]") {
	constexpr size_t capacity = 5;
	FixedVector<int> vec(capacity);

	// Initial state check
	CHECK(vec.size() == 0);
	CHECK(vec.capacity() == capacity);
	CHECK(vec.begin() == vec.end());
}

// Test the generator constructor with a single value return.
TEST_CASE("FixedVector Generator constructor (single value)","[FixedVector]") {
	constexpr size_t capacity = 3;
	FixedVector<ComplexType> vec(capacity, [](const size_t i)->auto {
		return std::make_tuple(static_cast<int>(i), std::to_string(i));
	});

	REQUIRE(vec.size() == capacity);
	REQUIRE(vec.capacity() == capacity);

	for (size_t i = 0; i < capacity; ++i) {
		CHECK(vec[i] == ComplexType{static_cast<int>(i), std::to_string(i)});
	}
}

// Test the generator constructor with a tuple return.
TEST_CASE("FixedVector Generator constructor (tuple)","[FixedVector]") {
	constexpr size_t capacity = 3;
	FixedVector<ComplexType> vec(capacity, [](const size_t i)->auto {
		return std::make_tuple(static_cast<int>(i), std::to_string(i));
	});

	REQUIRE(vec.size() == capacity);
	REQUIRE(vec.capacity() == capacity);

	for (size_t i = 0; i < capacity; ++i) {
		CHECK(vec[i] == ComplexType{static_cast<int>(i), std::to_string(i)});
	}
}

// Test emplace_back, pop_back, and clear with a complex type.
TEST_CASE("FixedVector Manipulation (emplace_back, pop_back, clear)","[FixedVector]") {
	constexpr size_t capacity = 4;
	FixedVector<ComplexType> vec(capacity);

	// Emplace elements with multiple arguments
	vec.emplace_back(1, "hello");
	REQUIRE(vec.size() == 1);
	CHECK(vec.back() == ComplexType{1, "hello"});

	// Emplace up to capacity
	vec.emplace_back(2, "world");
	vec.emplace_back(3, "!");
	vec.emplace_back(4, "done");
	REQUIRE(vec.size() == 4);
	CHECK(vec.back() == ComplexType{4, "done"});

	// Test emplace_back on a full vector (should not change size)
	auto it = vec.emplace_back(5, "oops");
	CHECK(vec.size() == 4);
	CHECK(it == vec.end());
	CHECK(vec.back() == ComplexType{4, "done"}); // The last element is unchanged

	// Test pop_back
	vec.pop_back();
	REQUIRE(vec.size() == 3);
	CHECK(vec.back() == ComplexType{3, "!"});

	vec.pop_back();
	REQUIRE(vec.size() == 2);
	CHECK(vec.back() == ComplexType{2, "world"});

	// Test clear
	vec.clear();
	CHECK(vec.size() == 0);
	CHECK(vec.begin() == vec.end());

	// Test pop_back on an empty vector
	vec.pop_back();
	CHECK(vec.size() == 0);
}

// Test that accessor methods return references to the same memory location
TEST_CASE("FixedVector Accessor reference consistency","[FixedVector]") {
	constexpr size_t capacity = 3;
	FixedVector<int> vec(capacity);

	vec.emplace_back(1);
	CHECK(&vec.front() == &vec.back());
	CHECK(&vec.front() == &vec[0]);

	vec.emplace_back(2);
	CHECK(&vec.front() == &vec[0]);
	CHECK(&vec.back() == &vec[vec.size() - 1]);

	vec.emplace_back(3);
	CHECK(&vec.front() == &vec[0]);
	CHECK(&vec.back() == &vec[vec.size() - 1]);
}

// Test with a non-copyable and non-movable type to ensure in-place construction.
TEST_CASE("FixedVector Non-copyable, non-movable type","[FixedVector]") {
	constexpr size_t capacity = 2;
	FixedVector<NonCopyableNonMovable> vec(capacity);

	// This should work because emplace_back constructs in place
	const int value0 = 1;
	auto it0 = vec.emplace_back(value0);
	REQUIRE(vec.size() == 1);
	CHECK(vec.back().value == value0);
	CHECK(&vec.back() == it0);

	const int value1 = 2;
	auto it1 = vec.emplace_back(value1);
	REQUIRE(vec.size() == 2);
	CHECK(vec.back().value == value1);
	CHECK(&vec.back() == it1);
}

TEST_CASE("FixedVector Destruction, Clear, and Refill","[FixedVector]") {
	// Reset the counters for a clean test
	DestructionCounter::constructor_count = 0;
	DestructionCounter::destructor_count = 0;

	constexpr size_t capacity = 3;

	{
		// Use a scope to test the FixedVector's destructor
		FixedVector<DestructionCounter> vec(capacity);

		// Emplace a few objects
		vec.emplace_back();
		vec.emplace_back();
		CHECK(vec.size() == 2);
		CHECK(DestructionCounter::constructor_count == 2);
		CHECK(DestructionCounter::destructor_count == 0);

		// Test clear()
		vec.clear();
		CHECK(vec.size() == 0);
		CHECK(DestructionCounter::destructor_count == 2); // All elements should be destructed
		
		// Test refilling after a clear
		vec.emplace_back();
		CHECK(vec.size() == 1);
		CHECK(DestructionCounter::constructor_count == 3); // A new object should be constructed
	} // `vec` goes out of scope here, and its destructor should be called

	// After the scope, the last element should also be destructed
	CHECK(DestructionCounter::destructor_count == 3);
}