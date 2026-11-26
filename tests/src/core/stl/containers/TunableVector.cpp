#include "openvic-simulation/core/stl/containers/TunableVector.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_constexpr.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>
#include <snitch/snitch_string.hpp>
#include <snitch/snitch_string_utility.hpp>

using namespace OpenVic::stl;

struct TestStruct {
	int i = 0;
	float f = 0.1f;
	constexpr auto operator<=>(TestStruct const& rhs) const = default;
	constexpr explicit operator int() const {
		return i;
	}
};

namespace snitch {
	template<typename ValueT, typename ColourIntT, typename ColourTraits>
	[[nodiscard]] inline static constexpr bool append( //
		snitch::small_string_span ss, TestStruct const& s
	) noexcept {
		return append(ss, "{", (int64_t)s.i, ",", (int64_t)s.f, "}");
	}
}

using VectorTypes = snitch::type_list<int, float, TestStruct>;

TEMPLATE_LIST_TEST_CASE("tunable_vector Constructor methods", "[tunable_vector][tunable_vector-constructor]", VectorTypes) {
	tunable_vector<TestType> v1;
	CHECK(v1.empty());
	CHECK(v1.size() == 0);

	tunable_vector<TestType> v2(5);
	CHECK(v2.size() == 5);
	for (TestType const& x : v2) {
		CHECK(x == TestType {});
	}

	tunable_vector<TestType> v3(3, TestType { 7 });
	CHECK(v3.size() == 3);
	for (TestType const& x : v3) {
		CHECK(x == TestType { 7 });
	}

	std::vector<TestType> base = { { 1 }, { 2 }, { 3 } };
	tunable_vector<TestType> v4(base);
	CHECK(v4.size() == 3);
	CHECK(v4[0] == TestType { 1 });
	CHECK(v4[1] == TestType { 2 });
	CHECK(v4[2] == TestType { 3 });

	tunable_vector<TestType> v5 { { 1 }, { 2 }, { 3 }, { 4 } };
	CHECK(v5.size() == 4);
	CHECK(v5[3] == TestType { 4 });
}

TEMPLATE_LIST_TEST_CASE("tunable_vector Copy and Move", "[tunable_vector][tunable_vector-copy-move]", VectorTypes) {
	tunable_vector<TestType> v1 { { 1 }, { 2 }, { 3 } };
	tunable_vector<TestType> v2 = v1;
	CHECK(v2.size() == 3);
	CHECK(v2[1] == TestType { 2 });

	tunable_vector<TestType> v3 = std::move(v1);
	CHECK(v3.size() == 3);
	CHECK(v3[2] == TestType { 3 });

	tunable_vector<TestType> v4;
	v4 = v3;
	CHECK(v4.size() == 3);

	tunable_vector<TestType> v5;
	v5 = std::move(v3);
	CHECK(v5.size() == 3);
}

TEST_CASE("tunable_vector Accessors", "[tunable_vector][tunable_vector-access]") {
	tunable_vector<int> v { 10, 20, 30 };

	CHECK(v.front() == 10);
	CHECK(v.back() == 30);
	CHECK(v[1] == 20);

	v[1] = { 99 };
	CHECK(v[1] == 99);
}

TEST_CASE("tunable_vector push_back / emplace_back", "[tunable_vector][tunable_vector-push]") {
	tunable_vector<int> v;

	v.push_back(15);
	CHECK(v.size() == 1);
	CHECK(v[0] == 15);

	v.emplace_back(25);
	CHECK(v.size() == 2);
	CHECK(v[1] == 25);
}

TEST_CASE("tunable_vector reserve_minimum Growth", "[tunable_vector][tunable_vector-reserve]") {
	tunable_vector<int> v;

	// Force growth
	v.push_back(1);
	size_t old_cap = v.capacity();

	for (int i = 0; i < 50; ++i) {
		v.push_back(i);
	}

	CHECK(v.size() == 51);
	CHECK(v.capacity() >= old_cap);
}

TEST_CASE("tunable_vector append_range", "[tunable_vector][tunable_vector-range]") {
	tunable_vector<int> v { 1, 2, 3 };
	std::vector<int> more { 4, 5, 6 };

	v.append_range(more);
	CHECK(v.size() == 6);

	for (int i = 0; i < 6; ++i) {
		CAPTURE(i);
		CHECK(v[i] == i + 1);
	}
}

TEST_CASE("tunable_vector resize", "[tunable_vector][tunable_vector-resize]") {
	tunable_vector<int> v { 1, 2, 3 };
	v.resize(5);

	CHECK(v.size() == 5);
	CHECK(v[0] == 1);
	CHECK(v[1] == 2);
	CHECK(v[2] == 3);
	CHECK(v[3] == int {});
	CHECK(v[4] == int {});

	v.resize(2);
	CHECK(v.size() == 2);
	CHECK(v[0] == 1);
	CHECK(v[1] == 2);
}

TEST_CASE("tunable_vector release()", "[tunable_vector][tunable_vector-release]") {
	tunable_vector<int> v { 1, 2, 3 };
	std::vector<int> raw = std::move(v).release();

	CHECK(raw.size() == 3);
	CHECK(raw[0] == 1);
	CHECK(raw[2] == 3);
}

TEST_CASE("tunable_vector Iterators", "[tunable_vector][tunable_vector-iterators]") {
	tunable_vector<int> v { 1, 2, 3 };

	int sum = 0;
	for (typename tunable_vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
		sum += static_cast<int>(*it);
	}
	CHECK(sum == 6);

	int rsum = 0;
	for (typename tunable_vector<int>::reverse_iterator it = v.rbegin(); it != v.rend(); ++it) {
		rsum += static_cast<int>(*it);
	}
	CHECK(rsum == 6);
}
