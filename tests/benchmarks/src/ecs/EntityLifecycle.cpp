#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct LifeA {
		int v = 0;
	};
	struct LifeB {
		int w = 0;
	};
	struct LifeC {
		float x = 0.0f;
	};
	struct LifeD {
		float y = 0.0f;
	};
	struct LifeE {
		int64_t z = 0;
	};
	struct LifeF {
		int64_t q = 0;
	};
	struct LifeG {
		double r = 0.0;
	};
	struct LifeH {
		double s = 0.0;
	};
}

ECS_COMPONENT(LifeA, "bench_EntityLifecycle::LifeA")
ECS_COMPONENT(LifeB, "bench_EntityLifecycle::LifeB")
ECS_COMPONENT(LifeC, "bench_EntityLifecycle::LifeC")
ECS_COMPONENT(LifeD, "bench_EntityLifecycle::LifeD")
ECS_COMPONENT(LifeE, "bench_EntityLifecycle::LifeE")
ECS_COMPONENT(LifeF, "bench_EntityLifecycle::LifeF")
ECS_COMPONENT(LifeG, "bench_EntityLifecycle::LifeG")
ECS_COMPONENT(LifeH, "bench_EntityLifecycle::LifeH")

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}
}

TEST_CASE("create_entity throughput by component count", "[benchmarks][benchmark-ecs][ecs-lifecycle]") {
	ankerl::nanobench::Bench bench;
	bench.title("create_entity (pre-attached components)").unit("entity");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("1 component" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(LifeA { static_cast<int>(i) });
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
		bench.batch(n).run("2 components" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(LifeA { static_cast<int>(i) }, LifeB { 0 });
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
		bench.batch(n).run("4 components" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(LifeA {}, LifeB {}, LifeC {}, LifeD {});
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
		bench.batch(n).run("8 components" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(LifeA {}, LifeB {}, LifeC {}, LifeD {}, LifeE {}, LifeF {}, LifeG {}, LifeH {});
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

TEST_CASE("destroy_entity throughput by traversal order", "[benchmarks][benchmark-ecs][ecs-lifecycle]") {
	ankerl::nanobench::Bench bench;
	bench.title("destroy_entity").unit("entity");

	for (std::size_t n : COUNTS) {
		// Tail-first: pops the global last row of the archetype each time — no swap-pop relocations.
		bench.batch(n).run("tail-first" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(LifeA { static_cast<int>(i) }, LifeB { 0 }));
			}
			for (std::size_t i = n; i > 0; --i) {
				world.destroy_entity(ids[i - 1]);
			}
		});

		// Head-first: every destroy is a swap-pop that relocates the last row into the freed slot.
		bench.batch(n).run("head-first (swap-pop)" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(LifeA { static_cast<int>(i) }, LifeB { 0 }));
			}
			for (std::size_t i = 0; i < n; ++i) {
				world.destroy_entity(ids[i]);
			}
		});

		// Random-order: uses a fixed seed so the bench is repeatable.
		bench.batch(n).run("random order" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(LifeA { static_cast<int>(i) }, LifeB { 0 }));
			}
			std::mt19937 rng { 0xC0FFEEu };
			std::shuffle(ids.begin(), ids.end(), rng);
			for (EntityID id : ids) {
				world.destroy_entity(id);
			}
		});
	}
}

TEST_CASE("create→destroy→create cycle (free-list reuse)", "[benchmarks][benchmark-ecs][ecs-lifecycle]") {
	ankerl::nanobench::Bench bench;
	bench.title("free-list reuse").unit("entity");

	for (std::size_t n : COUNTS) {
		bench.batch(n * 2).run("create+destroy+recreate" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(LifeA {}, LifeB {}));
			}
			for (EntityID id : ids) {
				world.destroy_entity(id);
			}
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(LifeA {}, LifeB {});
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

// Pitfall comparison from ECS.md: pre-attaching components at create_entity time avoids
// archetype migrations. Each post-add path migrates the entity to a new archetype per call —
// "lethal at scale (e.g. 1M order entities/tick)". This bench keeps that cost visible.
TEST_CASE("Pitfall: pre-attach vs post-add archetype migrations", "[benchmarks][benchmark-ecs][ecs-lifecycle][ecs-pitfall]") {
	ankerl::nanobench::Bench bench;
	bench.title("pre-attach vs post-add (4 components)").unit("entity");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("pre-attach (A,B,C,D)" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(LifeA {}, LifeB {}, LifeC {}, LifeD {});
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		bench.batch(n).run("post-add (A)+B+C+D" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				EntityID const eid = world.create_entity(LifeA {});
				world.add_component<LifeB>(eid, LifeB {});
				world.add_component<LifeC>(eid, LifeC {});
				world.add_component<LifeD>(eid, LifeD {});
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}
