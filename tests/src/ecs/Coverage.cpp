// Coverage gap-fill — add tests for public API surfaces not directly hit by the
// thematic test files. Each test below targets a method or scenario that
// otherwise would not have explicit assertion coverage.
#include "openvic-simulation/ecs/Archetype.hpp"
#include "openvic-simulation/ecs/CachedRef.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/Query.hpp"
#include "openvic-simulation/ecs/System.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct CovA {
		int v = 0;
	};
	struct CovB {
		int w = 0;
	};
	struct CovC {
		int x = 0;
	};
	struct CovD {
		int y = 0;
	};
	struct CovE {
		int z = 0;
	};
	struct CovTag {};

	// CountedDtor moved out of TEST_CASE body to file scope so it can be registered as
	// an ECS component via ECS_COMPONENT (FNV-1a hashing requires namespace-scope types).
	struct CountedDtor {
		int* counter = nullptr;
		CountedDtor() = default;
		CountedDtor(int* c) : counter { c } {}
		CountedDtor(CountedDtor&& other) noexcept : counter { other.counter } {
			other.counter = nullptr;
		}
		CountedDtor& operator=(CountedDtor&& other) noexcept {
			if (this != &other) {
				counter = other.counter;
				other.counter = nullptr;
			}
			return *this;
		}
		CountedDtor(CountedDtor const&) = delete;
		CountedDtor& operator=(CountedDtor const&) = delete;
		~CountedDtor() {
			if (counter != nullptr) {
				++(*counter);
			}
		}
	};
	// Checksum enforcement: holds a raw pointer (a test instrument, not sim state) — fold
	// presence only; the address would be nondeterministic.
	inline uint64_t ecs_checksum(CountedDtor const& c, uint64_t seed) {
		return fold_uint64(c.counter != nullptr ? 1u : 0u, seed);
	}
}

ECS_COMPONENT(CovA, "test_Coverage::CovA")
ECS_COMPONENT(CovB, "test_Coverage::CovB")
ECS_COMPONENT(CovC, "test_Coverage::CovC")
ECS_COMPONENT(CovD, "test_Coverage::CovD")
ECS_COMPONENT(CovE, "test_Coverage::CovE")
ECS_COMPONENT(CovTag, "test_Coverage::CovTag")
ECS_COMPONENT(CountedDtor, "test_Coverage::CountedDtor")

// === SystemHandle operator!= ===
TEST_CASE("SystemHandle operator!= is the inverse of ==", "[ecs][System][coverage]") {
	SystemHandle a { 1, 2 };
	SystemHandle b { 1, 2 };
	SystemHandle c { 1, 3 };
	SystemHandle d { 2, 2 };

	CHECK_FALSE(a != b);
	CHECK(a != c);
	CHECK(a != d);
}

// === Many-component archetype ===
TEST_CASE("Entity with five distinct components round-trips correctly", "[ecs][World][coverage]") {
	World world;
	EntityID const eid = world.create_entity(
		CovA { 1 }, CovB { 2 }, CovC { 3 }, CovD { 4 }, CovE { 5 }
	);
	CHECK(world.has_component<CovA>(eid));
	CHECK(world.has_component<CovB>(eid));
	CHECK(world.has_component<CovC>(eid));
	CHECK(world.has_component<CovD>(eid));
	CHECK(world.has_component<CovE>(eid));

	CHECK(world.get_component<CovA>(eid)->v == 1);
	CHECK(world.get_component<CovB>(eid)->w == 2);
	CHECK(world.get_component<CovC>(eid)->x == 3);
	CHECK(world.get_component<CovD>(eid)->y == 4);
	CHECK(world.get_component<CovE>(eid)->z == 5);
}

TEST_CASE("Migration preserves five-component values across add", "[ecs][World][coverage][migration]") {
	World world;
	EntityID const eid = world.create_entity(
		CovA { 11 }, CovB { 22 }, CovC { 33 }, CovD { 44 }
	);
	world.add_component<CovE>(eid, CovE { 55 });

	CHECK(world.get_component<CovA>(eid)->v == 11);
	CHECK(world.get_component<CovB>(eid)->w == 22);
	CHECK(world.get_component<CovC>(eid)->x == 33);
	CHECK(world.get_component<CovD>(eid)->y == 44);
	CHECK(world.get_component<CovE>(eid)->z == 55);
}

// === Removing the last entity from an archetype leaves the archetype empty but functional ===
TEST_CASE("Archetype can be emptied and refilled", "[ecs][World][coverage]") {
	World world;
	EntityID const a = world.create_entity(CovA { 1 }, CovB { 2 });
	EntityID const b = world.create_entity(CovA { 3 }, CovB { 4 });

	world.destroy_entity(a);
	world.destroy_entity(b);

	int count = 0;
	world.for_each<CovA, CovB>([&](CovA&, CovB&) { ++count; });
	CHECK(count == 0);

	// Refill — same archetype, slot-reuse path.
	EntityID const c = world.create_entity(CovA { 5 }, CovB { 6 });
	CHECK(world.is_alive(c));
	CHECK(world.get_component<CovA>(c)->v == 5);
	CHECK(world.get_component<CovB>(c)->w == 6);

	count = 0;
	world.for_each<CovA, CovB>([&](CovA&, CovB&) { ++count; });
	CHECK(count == 1);
}

// === for_each over an empty World ===
TEST_CASE("for_each_with_entity on empty World is safe and a no-op", "[ecs][World][iter][coverage]") {
	World world;
	int count = 0;
	world.for_each_with_entity<CovA>([&](EntityID, CovA&) { ++count; });
	CHECK(count == 0);

	Query q;
	q.with<CovA>().build();
	world.for_each_with_entity<CovA>(q, [&](EntityID, CovA&) { ++count; });
	CHECK(count == 0);
}

// === CachedRef value-type semantics: copyable, default-constructible, comparable via field access ===
TEST_CASE("CachedRef is trivially copyable", "[ecs][CachedRef][coverage]") {
	World world;
	EntityID const eid = world.create_entity(CovA { 42 });
	auto ref1 = CachedRef<CovA>::from(world, eid);

	CachedRef<CovA> ref2 = ref1;     // copy-construct
	CachedRef<CovA> ref3;
	ref3 = ref1;                     // copy-assign

	CHECK(ref1.get(world)->v == 42);
	CHECK(ref2.get(world)->v == 42);
	CHECK(ref3.get(world)->v == 42);
	CHECK(ref1.entity() == ref2.entity());
	CHECK(ref1.entity() == ref3.entity());
}

// === Queries match correctly across many archetypes ===
TEST_CASE("Query selects the intended subset across many archetypes", "[ecs][World][query][coverage]") {
	World world;
	world.create_entity(CovA { 1 });                            // A
	world.create_entity(CovA { 2 }, CovB { 1 });                // AB
	world.create_entity(CovA { 3 }, CovC { 1 });                // AC
	world.create_entity(CovA { 4 }, CovB { 1 }, CovC { 1 });    // ABC
	world.create_entity(CovA { 5 }, CovD { 1 });                // AD
	world.create_entity(CovB { 1 }, CovC { 1 });                // BC (no A)

	// Want all entities with A but not D.
	Query q;
	q.with<CovA>().exclude<CovD>().build();
	int count = 0;
	int sum = 0;
	world.for_each<CovA>(q, [&](CovA& a) {
		++count;
		sum += a.v;
	});
	CHECK(count == 4);          // A, AB, AC, ABC
	CHECK(sum == 1 + 2 + 3 + 4);
}

// === Stress: lots of churn across archetypes ===
TEST_CASE("Stress: many archetype migrations, query stays accurate", "[ecs][World][coverage][stress]") {
	World world;

	std::vector<EntityID> ids;
	for (int i = 0; i < 50; ++i) {
		ids.push_back(world.create_entity(CovA { i }));
	}

	// Promote even-indexed entities to {CovA, CovB}.
	for (int i = 0; i < 50; i += 2) {
		world.add_component<CovB>(ids[i], CovB { i * 10 });
	}

	int with_b = 0;
	world.for_each<CovA, CovB>([&](CovA&, CovB&) { ++with_b; });
	CHECK(with_b == 25);

	int total_a = 0;
	world.for_each<CovA>([&](CovA&) { ++total_a; });
	CHECK(total_a == 50);

	// Now demote half of the {CovA, CovB} entities back.
	for (int i = 0; i < 25; ++i) {
		EntityID e = ids[i * 2];
		if (i % 2 == 0) {
			world.remove_component<CovB>(e);
		}
	}

	int after_b = 0;
	world.for_each<CovA, CovB>([&](CovA&, CovB&) { ++after_b; });
	CHECK(after_b == 12); // 25 → 12 after removing 13 (i=0,2,...,24)

	int after_a = 0;
	world.for_each<CovA>([&](CovA&) { ++after_a; });
	CHECK(after_a == 50);
}

// === World destruction calls component destructors (smoke) ===
TEST_CASE("World destructor reclaims entities and singletons", "[ecs][World][coverage]") {
	int dtor_count = 0;
	{
		World world;
		world.create_entity(CountedDtor { &dtor_count });
		world.create_entity(CountedDtor { &dtor_count });
		world.set_singleton(CountedDtor { &dtor_count });
		// World destructor runs at end of scope.
	}
	CHECK(dtor_count == 3); // 2 entity components + 1 singleton
}

// === Ordering: archetype signature is deterministic regardless of component order at create_entity ===
TEST_CASE("Component order at create_entity is normalised", "[ecs][World][coverage]") {
	World world;
	EntityID const a = world.create_entity(CovA { 1 }, CovB { 2 }, CovC { 3 });
	EntityID const b = world.create_entity(CovB { 5 }, CovC { 6 }, CovA { 4 });
	EntityID const c = world.create_entity(CovC { 9 }, CovA { 7 }, CovB { 8 });

	// All three should land in the same archetype (same signature). Components
	// should be retrievable correctly.
	CHECK(world.get_component<CovA>(a)->v == 1);
	CHECK(world.get_component<CovB>(a)->w == 2);
	CHECK(world.get_component<CovC>(a)->x == 3);
	CHECK(world.get_component<CovA>(b)->v == 4);
	CHECK(world.get_component<CovB>(b)->w == 5);
	CHECK(world.get_component<CovC>(b)->x == 6);
	CHECK(world.get_component<CovA>(c)->v == 7);
	CHECK(world.get_component<CovB>(c)->w == 8);
	CHECK(world.get_component<CovC>(c)->x == 9);

	// All three visible in a single for_each.
	int count = 0;
	world.for_each<CovA, CovB, CovC>([&](CovA&, CovB&, CovC&) { ++count; });
	CHECK(count == 3);
}

// === SystemHandle operator== with itself ===
TEST_CASE("SystemHandle equality is reflexive and matches default INVALID", "[ecs][System][coverage]") {
	SystemHandle a { 5, 7 };
	CHECK(a == a);
	CHECK(SystemHandle {} == INVALID_SYSTEM_HANDLE);
}

// === Tag-only archetype destroy + recreate ===
TEST_CASE("Tag-only archetype handles destroy + create cycles", "[ecs][World][tag][coverage]") {
	World world;
	std::vector<EntityID> ids;
	for (int i = 0; i < 5; ++i) {
		ids.push_back(world.create_entity(CovTag {}));
	}

	for (int i = 0; i < 5; ++i) {
		world.destroy_entity(ids[i]);
	}

	int count = 0;
	world.for_each<CovTag>([&](CovTag&) { ++count; });
	CHECK(count == 0);

	for (int i = 0; i < 3; ++i) {
		world.create_entity(CovTag {});
	}

	count = 0;
	world.for_each<CovTag>([&](CovTag&) { ++count; });
	CHECK(count == 3);
}

// === component_version_in distinguishes per-archetype-column versions ===
TEST_CASE("component_version_in is per-archetype, not global per type", "[ecs][World][version][coverage]") {
	World world;
	EntityID const a = world.create_entity(CovA { 1 });             // archetype {A}
	EntityID const b = world.create_entity(CovA { 2 }, CovB { 3 }); // archetype {A,B}

	uint64_t va_in_a = world.component_version_in<CovA>(a);
	uint64_t va_in_b = world.component_version_in<CovA>(b);

	world.create_entity(CovA { 99 }); // bumps {A} archetype's CovA column

	uint64_t va_in_a2 = world.component_version_in<CovA>(a);
	uint64_t va_in_b2 = world.component_version_in<CovA>(b);

	CHECK(va_in_a2 > va_in_a);   // mutated
	CHECK(va_in_b2 == va_in_b);  // {A,B} archetype untouched
}

// === Empty Query (only require_ids, no exclude) is the same as no Query ===
TEST_CASE("Query-overload matches non-Query for_each on identical require set", "[ecs][World][query][coverage]") {
	World world;
	world.create_entity(CovA { 1 });
	world.create_entity(CovA { 2 }, CovB { 0 });
	world.create_entity(CovA { 3 }, CovB { 0 }, CovC { 0 });

	int n_plain = 0;
	world.for_each<CovA>([&](CovA&) { ++n_plain; });

	Query q;
	q.with<CovA>().build();
	int n_query = 0;
	world.for_each<CovA>(q, [&](CovA&) { ++n_query; });

	CHECK(n_plain == n_query);
	CHECK(n_plain == 3);
}

// === remove_component then add_component is idempotent (round-trip) ===
TEST_CASE("Round-trip remove + add of a component restores all data", "[ecs][World][migration][coverage]") {
	World world;
	EntityID const eid = world.create_entity(CovA { 100 }, CovB { 200 }, CovC { 300 });

	bool removed = world.remove_component<CovB>(eid);
	CHECK(removed);
	CHECK_FALSE(world.has_component<CovB>(eid));

	CovB* added = world.add_component<CovB>(eid, CovB { 200 });
	CHECK(added != nullptr);
	CHECK(added->w == 200);

	CHECK(world.get_component<CovA>(eid)->v == 100);
	CHECK(world.get_component<CovB>(eid)->w == 200);
	CHECK(world.get_component<CovC>(eid)->x == 300);
}

// === Singleton roundtrip via const overload ===
TEST_CASE("Singleton get/set/clear roundtrip with const access", "[ecs][World][singleton][coverage]") {
	World world;
	world.set_singleton(CovA { 13 });

	World const& cw = world;
	CovA const* p = cw.get_singleton<CovA>();
	CHECK(p != nullptr);
	CHECK(p->v == 13);

	bool cleared = world.clear_singleton<CovA>();
	CHECK(cleared);
	CHECK(cw.get_singleton<CovA>() == nullptr);
}
