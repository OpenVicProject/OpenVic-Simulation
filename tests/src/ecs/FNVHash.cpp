#include "openvic-simulation/ecs/ComponentTypeID.hpp"

#include <cstdint>
#include <string_view>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct FNVA {};
	struct FNVB {};
	struct FNVNamedSame {};
}

ECS_COMPONENT(FNVA, "test_FNVHash::FNVA")
ECS_COMPONENT(FNVB, "test_FNVHash::FNVB")
ECS_COMPONENT(FNVNamedSame, "test_FNVHash::FNVNamedSame")

TEST_CASE("fnv1a_64 of empty string equals offset basis", "[ecs][FNVHash]") {
	constexpr uint64_t expected = 0xcbf29ce484222325ULL;
	CHECK(fnv1a_64("") == expected);
}

TEST_CASE("fnv1a_64 known input matches known output", "[ecs][FNVHash]") {
	// Verified against an independent FNV-1a-64 implementation.
	// fnv1a_64("foobar") = 0x85944171f73967e8.
	CHECK(fnv1a_64("foobar") == 0x85944171f73967e8ULL);
}

TEST_CASE("fnv1a_64 differs for different inputs", "[ecs][FNVHash]") {
	CHECK(fnv1a_64("a") != fnv1a_64("b"));
	CHECK(fnv1a_64("test_FNVHash::FNVA") != fnv1a_64("test_FNVHash::FNVB"));
}

TEST_CASE("component_type_id_of matches the FNV-1a hash of its registered name", "[ecs][FNVHash]") {
	constexpr component_type_id_t expected_a = fnv1a_64("test_FNVHash::FNVA");
	constexpr component_type_id_t expected_b = fnv1a_64("test_FNVHash::FNVB");
	CHECK(component_type_id_of<FNVA>() == expected_a);
	CHECK(component_type_id_of<FNVB>() == expected_b);
}

TEST_CASE("component_type_id_of differs for distinct registered components", "[ecs][FNVHash]") {
	CHECK(component_type_id_of<FNVA>() != component_type_id_of<FNVB>());
	CHECK(component_type_id_of<FNVA>() != component_type_id_of<FNVNamedSame>());
}

TEST_CASE("component_type_id_of is constexpr (usable in static_assert)", "[ecs][FNVHash]") {
	static_assert(component_type_id_of<FNVA>() == fnv1a_64("test_FNVHash::FNVA"));
	static_assert(component_type_id_of<FNVB>() == fnv1a_64("test_FNVHash::FNVB"));
	CHECK(true);
}

TEST_CASE("ComponentName::value contains the registered string", "[ecs][FNVHash]") {
	CHECK(ComponentName<FNVA>::value == std::string_view { "test_FNVHash::FNVA" });
	CHECK(ComponentName<FNVB>::value == std::string_view { "test_FNVHash::FNVB" });
}
