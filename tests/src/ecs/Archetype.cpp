#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct Position {
		int32_t x;
		int32_t y;
	};
	struct Velocity {
		float dx;
		float dy;
	};
	struct EmptyTag {};
	struct AnotherTag {};
}

ECS_COMPONENT(Position, "test_Archetype::Position")
ECS_COMPONENT(Velocity, "test_Archetype::Velocity")
ECS_COMPONENT(EmptyTag, "test_Archetype::EmptyTag")
ECS_COMPONENT(AnotherTag, "test_Archetype::AnotherTag")

TEST_CASE("ColumnVTable for non-empty type carries size/align/thunks", "[ecs][Archetype][ColumnVTable]") {
	ColumnVTable const& v = column_vtable_for<Position>();
	CHECK(v.size == sizeof(Position));
	CHECK(v.align == alignof(Position));
	CHECK(v.move_construct != nullptr);
	CHECK(v.destroy != nullptr);
}

TEST_CASE("ColumnVTable for empty type has size==0 and align==0", "[ecs][Archetype][ColumnVTable]") {
	ColumnVTable const& v = column_vtable_for<EmptyTag>();
	CHECK(v.size == 0u);
	CHECK(v.align == 0u);
	CHECK(v.move_construct != nullptr);
	CHECK(v.destroy != nullptr);
}

TEST_CASE("ColumnVTable instances are stable per type", "[ecs][Archetype][ColumnVTable]") {
	ColumnVTable const* v1 = &column_vtable_for<Position>();
	ColumnVTable const* v2 = &column_vtable_for<Position>();
	CHECK(v1 == v2);
}

TEST_CASE("Archetype column_index_for finds existing and returns NO_COLUMN_INDEX for missing", "[ecs][Archetype]") {
	Archetype arch;
	component_type_id_t const pos_id = component_type_id_of<Position>();
	component_type_id_t const vel_id = component_type_id_of<Velocity>();
	component_type_id_t const missing_id = component_type_id_of<EmptyTag>();

	std::vector<component_type_id_t> sig = { pos_id, vel_id };
	std::sort(sig.begin(), sig.end());
	arch.signature = sig;

	std::size_t i_pos = arch.column_index_for(pos_id);
	std::size_t i_vel = arch.column_index_for(vel_id);
	std::size_t i_miss = arch.column_index_for(missing_id);

	CHECK(i_pos != NO_COLUMN_INDEX);
	CHECK(i_vel != NO_COLUMN_INDEX);
	CHECK(i_miss == NO_COLUMN_INDEX);
	CHECK(arch.signature[i_pos] == pos_id);
	CHECK(arch.signature[i_vel] == vel_id);
}

TEST_CASE("Archetype has_component matches column_index_for", "[ecs][Archetype]") {
	Archetype arch;
	component_type_id_t const pos_id = component_type_id_of<Position>();
	component_type_id_t const tag_id = component_type_id_of<EmptyTag>();

	std::vector<component_type_id_t> sig = { pos_id };
	std::sort(sig.begin(), sig.end());
	arch.signature = sig;

	CHECK(arch.has_component(pos_id));
	CHECK_FALSE(arch.has_component(tag_id));
}

TEST_CASE("Archetype matches_all on subset / superset / disjoint", "[ecs][Archetype]") {
	Archetype arch;
	component_type_id_t const a = component_type_id_of<Position>();
	component_type_id_t const b = component_type_id_of<Velocity>();
	component_type_id_t const c = component_type_id_of<EmptyTag>();
	component_type_id_t const d = component_type_id_of<AnotherTag>();

	std::vector<component_type_id_t> sig = { a, b, c };
	std::sort(sig.begin(), sig.end());
	arch.signature = sig;

	auto sorted = [](std::vector<component_type_id_t> v) {
		std::sort(v.begin(), v.end());
		return v;
	};

	CHECK(arch.matches_all({}));                            // empty required
	CHECK(arch.matches_all(sorted({ a })));                 // subset
	CHECK(arch.matches_all(sorted({ a, b })));              // subset
	CHECK(arch.matches_all(sorted({ a, b, c })));           // exact
	CHECK_FALSE(arch.matches_all(sorted({ a, d })));        // requires missing
	CHECK_FALSE(arch.matches_all(sorted({ d })));           // entirely disjoint
	CHECK_FALSE(arch.matches_all(sorted({ a, b, c, d })));  // superset of arch
}

TEST_CASE("Archetype matches_none on disjoint / overlap / empty", "[ecs][Archetype]") {
	Archetype arch;
	component_type_id_t const a = component_type_id_of<Position>();
	component_type_id_t const b = component_type_id_of<Velocity>();
	component_type_id_t const c = component_type_id_of<EmptyTag>();
	component_type_id_t const d = component_type_id_of<AnotherTag>();

	std::vector<component_type_id_t> sig = { a, b };
	std::sort(sig.begin(), sig.end());
	arch.signature = sig;

	auto sorted = [](std::vector<component_type_id_t> v) {
		std::sort(v.begin(), v.end());
		return v;
	};

	CHECK(arch.matches_none({}));                       // empty exclude
	CHECK(arch.matches_none(sorted({ c })));            // disjoint
	CHECK(arch.matches_none(sorted({ c, d })));         // disjoint
	CHECK_FALSE(arch.matches_none(sorted({ a })));      // overlap
	CHECK_FALSE(arch.matches_none(sorted({ a, c })));   // partial overlap
	CHECK_FALSE(arch.matches_none(sorted({ a, b })));   // full overlap
}
