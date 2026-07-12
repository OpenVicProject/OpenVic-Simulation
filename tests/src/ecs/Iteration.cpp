#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/Query.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <set>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct IA {
		int v = 0;
	};
	struct IB {
		int w = 0;
	};
	struct IC {
		int x = 0;
	};
	struct IDead {}; // tag for "logically dead"
}

ECS_COMPONENT(IA, "test_Iteration::IA")
ECS_COMPONENT(IB, "test_Iteration::IB")
ECS_COMPONENT(IC, "test_Iteration::IC")
ECS_COMPONENT(IDead, "test_Iteration::IDead")

TEST_CASE("for_each visits all entities of a single-component archetype", "[ecs][World][iter]") {
	World world;
	for (int i = 0; i < 5; ++i) {
		world.create_entity(IA { i });
	}

	int sum = 0;
	int count = 0;
	world.for_each<IA>([&](IA& a) {
		sum += a.v;
		++count;
	});
	CHECK(count == 5);
	CHECK(sum == 0 + 1 + 2 + 3 + 4);
}

TEST_CASE("for_each visits zero entities when no archetype matches", "[ecs][World][iter]") {
	World world;
	int count = 0;
	world.for_each<IA>([&](IA&) { ++count; });
	CHECK(count == 0);
}

TEST_CASE("for_each visits only matching archetype superset", "[ecs][World][iter]") {
	World world;
	world.create_entity(IA { 1 });
	world.create_entity(IA { 2 }, IB { 20 });
	world.create_entity(IB { 99 });

	int a_only_count = 0;
	world.for_each<IA>([&](IA& a) {
		(void) a;
		++a_only_count;
	});
	CHECK(a_only_count == 2); // both IA-only and IA+IB

	int both_count = 0;
	world.for_each<IA, IB>([&](IA&, IB&) { ++both_count; });
	CHECK(both_count == 1);
}

TEST_CASE("for_each_with_entity passes the correct EntityID", "[ecs][World][iter]") {
	World world;
	EntityID const a = world.create_entity(IA { 7 });
	EntityID const b = world.create_entity(IA { 11 });

	std::set<uint64_t> seen;
	world.for_each_with_entity<IA>([&](EntityID e, IA&) { seen.insert(e.to_uint64()); });

	CHECK(seen.count(a.to_uint64()) == 1u);
	CHECK(seen.count(b.to_uint64()) == 1u);
	CHECK(seen.size() == 2u);
}

TEST_CASE("for_each lambda can mutate components in place", "[ecs][World][iter]") {
	World world;
	for (int i = 0; i < 4; ++i) {
		world.create_entity(IA { i });
	}
	world.for_each<IA>([](IA& a) { a.v *= 2; });

	int sum = 0;
	world.for_each<IA>([&](IA& a) { sum += a.v; });
	CHECK(sum == (0 + 2 + 4 + 6));
}

TEST_CASE("Query overload of for_each respects exclude", "[ecs][World][iter][query]") {
	World world;
	world.create_entity(IA { 1 });
	world.create_entity(IA { 2 }, IDead {});
	world.create_entity(IA { 3 });
	world.create_entity(IA { 4 }, IDead {});

	Query q;
	q.with<IA>().exclude<IDead>().build();

	int sum = 0;
	int count = 0;
	world.for_each<IA>(q, [&](IA& a) {
		sum += a.v;
		++count;
	});
	CHECK(count == 2);
	CHECK(sum == 1 + 3);
}

TEST_CASE("Query overload of for_each_with_entity respects exclude", "[ecs][World][iter][query]") {
	World world;
	EntityID const a = world.create_entity(IA { 1 });
	world.create_entity(IA { 2 }, IDead {});
	EntityID const c = world.create_entity(IA { 3 });

	Query q;
	q.with<IA>().exclude<IDead>().build();

	std::set<uint64_t> seen;
	world.for_each_with_entity<IA>(q, [&](EntityID e, IA&) { seen.insert(e.to_uint64()); });
	CHECK(seen.size() == 2u);
	CHECK(seen.count(a.to_uint64()) == 1u);
	CHECK(seen.count(c.to_uint64()) == 1u);
}

TEST_CASE("for_each works repeatedly (cached query)", "[ecs][World][iter][cache]") {
	World world;
	world.create_entity(IA { 1 });
	world.create_entity(IA { 2 });

	for (int round = 0; round < 3; ++round) {
		int count = 0;
		world.for_each<IA>([&](IA&) { ++count; });
		CHECK(count == 2);
	}
}

TEST_CASE("Query cache is invalidated when a new archetype is created", "[ecs][World][iter][cache]") {
	World world;
	EntityID const a = world.create_entity(IA { 1 });
	(void) a;

	int count_before = 0;
	world.for_each<IA>([&](IA&) { ++count_before; });
	CHECK(count_before == 1);

	// New archetype: {IA, IB}. Cached "IA" query should now also include this.
	world.create_entity(IA { 2 }, IB { 0 });

	int count_after = 0;
	world.for_each<IA>([&](IA&) { ++count_after; });
	CHECK(count_after == 2);
}

TEST_CASE("for_each iterates a multi-archetype query correctly", "[ecs][World][iter]") {
	World world;
	world.create_entity(IA { 10 });
	world.create_entity(IA { 20 }, IB { 1 });
	world.create_entity(IA { 30 }, IC { 1 });
	world.create_entity(IA { 40 }, IB { 1 }, IC { 1 });
	world.create_entity(IB { 1 }, IC { 1 }); // no IA — should not match

	int sum = 0;
	int count = 0;
	world.for_each<IA>([&](IA& a) {
		sum += a.v;
		++count;
	});
	CHECK(count == 4);
	CHECK(sum == 10 + 20 + 30 + 40);
}

TEST_CASE("Query exclude with multiple components", "[ecs][World][iter][query]") {
	World world;
	world.create_entity(IA { 1 });
	world.create_entity(IA { 2 }, IB { 0 });
	world.create_entity(IA { 3 }, IC { 0 });
	world.create_entity(IA { 4 }, IB { 0 }, IC { 0 });

	Query q;
	q.with<IA>().exclude<IB, IC>().build();

	int sum = 0;
	int count = 0;
	world.for_each<IA>(q, [&](IA& a) {
		sum += a.v;
		++count;
	});
	CHECK(count == 1);
	CHECK(sum == 1);
}

TEST_CASE("Query with empty require_ids matches all archetypes that don't carry excluded", "[ecs][World][iter][query]") {
	// Note: for_each<Cs...>(query, fn) requires at least one C in the lambda; you can't
	// pass an empty set on the call site. So a "match all" query needs at least one anchor
	// component. We pick IA as the anchor here.
	World world;
	world.create_entity(IA { 1 });
	world.create_entity(IA { 2 }, IB { 0 });
	Query q;
	q.with<IA>().build();
	int count = 0;
	world.for_each<IA>(q, [&](IA&) { ++count; });
	CHECK(count == 2);
}

TEST_CASE("for_each is safe to call from within another for_each (read-only)", "[ecs][World][iter]") {
	World world;
	world.create_entity(IA { 1 });
	world.create_entity(IA { 2 });
	world.create_entity(IA { 3 });

	int outer_count = 0;
	int inner_total = 0;
	world.for_each<IA>([&](IA&) {
		++outer_count;
		world.for_each<IA>([&](IA& a) { inner_total += a.v; });
	});
	CHECK(outer_count == 3);
	CHECK(inner_total == 3 * (1 + 2 + 3));
}
