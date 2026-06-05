#include "openvic-simulation/ecs/ChunkView.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/Query.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct IterA {
		int v = 0;
	};
	struct IterB {
		float w = 0.0f;
	};
	struct IterC {
		int64_t x = 0;
	};
	struct IterDead {}; // tag
}

ECS_COMPONENT(IterA, "bench_Iteration::IterA")
ECS_COMPONENT(IterB, "bench_Iteration::IterB")
ECS_COMPONENT(IterC, "bench_Iteration::IterC")
ECS_COMPONENT(IterDead, "bench_Iteration::IterDead")

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}

	// Single-archetype world: every entity carries (IterA, IterB).
	void populateSingleArchetype(World& world, std::size_t n) {
		for (std::size_t i = 0; i < n; ++i) {
			world.create_entity(IterA { static_cast<int>(i) }, IterB { static_cast<float>(i) });
		}
	}

	// Three archetypes splitting the population across {A}, {A,B}, {A,B,C}. Forces
	// archetype-walk overhead during iteration that the single-archetype case skips.
	void populateMultiArchetype(World& world, std::size_t n) {
		std::size_t const third = n / 3;
		std::size_t const rest = n - 2 * third;
		for (std::size_t i = 0; i < third; ++i) {
			world.create_entity(IterA { static_cast<int>(i) });
		}
		for (std::size_t i = 0; i < third; ++i) {
			world.create_entity(IterA { static_cast<int>(i) }, IterB { static_cast<float>(i) });
		}
		for (std::size_t i = 0; i < rest; ++i) {
			world.create_entity(
				IterA { static_cast<int>(i) }, IterB { static_cast<float>(i) }, IterC { static_cast<int64_t>(i) }
			);
		}
	}
}

TEST_CASE("for_each over single-archetype world", "[benchmarks][benchmark-ecs][ecs-iter]") {
	ankerl::nanobench::Bench bench;
	bench.title("for_each (single archetype, IterA+IterB)").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populateSingleArchetype(world, n);

		bench.batch(n).run("for_each<IterA>" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each<IterA>([&](IterA& a) { acc += a.v; });
			ankerl::nanobench::doNotOptimizeAway(acc);
		});

		bench.batch(n).run("for_each<IterA,IterB>" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each<IterA, IterB>([&](IterA& a, IterB& b) {
				acc += a.v + static_cast<int64_t>(b.w);
			});
			ankerl::nanobench::doNotOptimizeAway(acc);
		});

		bench.batch(n).run("for_each_with_entity<IterA,IterB>" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each_with_entity<IterA, IterB>([&](EntityID eid, IterA& a, IterB&) {
				acc += a.v + eid.index;
			});
			ankerl::nanobench::doNotOptimizeAway(acc);
		});
	}
}

TEST_CASE("for_each over multi-archetype world", "[benchmarks][benchmark-ecs][ecs-iter]") {
	ankerl::nanobench::Bench bench;
	bench.title("for_each (3 archetypes)").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populateMultiArchetype(world, n);

		bench.batch(n).run("for_each<IterA> (all 3)" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each<IterA>([&](IterA& a) { acc += a.v; });
			ankerl::nanobench::doNotOptimizeAway(acc);
		});

		// for_each<A,B> matches only 2/3 of the population — measures archetype-rejection overhead.
		bench.batch(n).run("for_each<IterA,IterB> (matches 2/3)" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each<IterA, IterB>([&](IterA& a, IterB& b) {
				acc += a.v + static_cast<int64_t>(b.w);
			});
			ankerl::nanobench::doNotOptimizeAway(acc);
		});

		// for_each<A,B,C> matches only 1/3.
		bench.batch(n).run("for_each<IterA,IterB,IterC> (matches 1/3)" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each<IterA, IterB, IterC>([&](IterA& a, IterB&, IterC& c) {
				acc += a.v + c.x;
			});
			ankerl::nanobench::doNotOptimizeAway(acc);
		});
	}
}

TEST_CASE("for_each_chunk over single-archetype world", "[benchmarks][benchmark-ecs][ecs-iter]") {
	ankerl::nanobench::Bench bench;
	bench.title("for_each_chunk (tight inner loop)").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		populateSingleArchetype(world, n);

		bench.batch(n).run("for_each_chunk<IterA,IterB>" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each_chunk<IterA, IterB>([&](ChunkView<IterA, IterB> view) {
				IterA* a = view.array<IterA>();
				IterB* b = view.array<IterB>();
				std::size_t const count = view.count();
				for (std::size_t i = 0; i < count; ++i) {
					acc += a[i].v + static_cast<int64_t>(b[i].w);
				}
			});
			ankerl::nanobench::doNotOptimizeAway(acc);
		});
	}
}

// Query with exclude<Tag>: measures the cost of rejecting tagged-as-dead entities.
TEST_CASE("for_each with Query::exclude<Tag>", "[benchmarks][benchmark-ecs][ecs-iter]") {
	ankerl::nanobench::Bench bench;
	bench.title("Query exclude<Tag>").unit("entity");

	for (std::size_t n : COUNTS) {
		World world;
		// Half the entities also carry the IterDead tag.
		for (std::size_t i = 0; i < n; ++i) {
			if (i % 2 == 0) {
				world.create_entity(IterA { static_cast<int>(i) }, IterB { 0.0f });
			} else {
				world.create_entity(IterA { static_cast<int>(i) }, IterB { 0.0f }, IterDead {});
			}
		}

		Query query;
		query.with<IterA, IterB>().exclude<IterDead>().build();

		bench.batch(n).run("Query exclude<IterDead>" + suffix(n), [&] {
			int64_t acc = 0;
			world.for_each<IterA, IterB>(query, [&](IterA& a, IterB&) { acc += a.v; });
			ankerl::nanobench::doNotOptimizeAway(acc);
		});
	}
}
