#include <thread>

#include "openvic-simulation/types/fixed_point/Atomic.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

#include "Helper.hpp" // IWYU pragma: keep
#include "Numeric.hpp" // IWYU pragma: keep
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("atomic_fixed_point_t Constructor methods", "[atomic_fixed_point_t][atomic_fixed_point_t-constructor]") {
	CHECK(atomic_fixed_point_t {} == fixed_point_t::_0());
	CHECK(atomic_fixed_point_t { fixed_point_t::_1() } == fixed_point_t::_1());
}

TEST_CASE("atomic_fixed_point_t Operators", "[atomic_fixed_point_t][atomic_fixed_point_t-operators]") {
	atomic_fixed_point_t a { fixed_point_t::_0() };

	CHECK(a++ == fixed_point_t::_0());
	CHECK(a == fixed_point_t::_1());

	CHECK(++a == fixed_point_t::_2());

	CHECK(a-- == fixed_point_t::_2());
	CHECK(a == fixed_point_t::_1());

	CHECK(--a == fixed_point_t::_0());

	CHECK((a = fixed_point_t::_10()) == fixed_point_t::_10());
	CHECK(a == fixed_point_t::_10());
}

TEST_CASE("atomic_fixed_point_t Atomic Behavior", "[atomic_fixed_point_t][atomic_fixed_point_t-atomic-behavior]") {
	atomic_fixed_point_t b { fixed_point_t::_0() };
	std::vector<std::thread> pool;
	for (size_t outer_index = 0; outer_index < 16; ++outer_index) {
		pool.emplace_back([&b]() {
			for (size_t inner_index = 1024; inner_index; --inner_index) {
				++b;
			}
		});
	}

	for (std::thread& thread : pool) {
		thread.join();
	}
	CHECK(b == fixed_point_t { 16 * 1024 });
}
