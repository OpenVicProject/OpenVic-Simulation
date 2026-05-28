#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct MigA {
		int v = 0;
	};
	struct MigB {
		int w = 0;
	};
	struct MigC {
		float x = 0.0f;
	};
	struct MigTag {}; // zero-size — migrates archetype but stores no per-row data
}

ECS_COMPONENT(MigA, "bench_Migration::MigA")
ECS_COMPONENT(MigB, "bench_Migration::MigB")
ECS_COMPONENT(MigC, "bench_Migration::MigC")
ECS_COMPONENT(MigTag, "bench_Migration::MigTag")

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}
}

// Per-entity archetype migration cost: add a component to every entity, one at a time.
// Each add forces a row migration {A} → {A,B}, copying the existing column data plus the
// new column. Repeated across N entities, this is the hot path that ECS.md flags as
// "lethal at scale".
TEST_CASE("add_component (archetype migration)", "[benchmarks][benchmark-ecs][ecs-migration]") {
	ankerl::nanobench::Bench bench;
	bench.title("add_component archetype migration").unit("migration");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("add 1 component" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(MigA { static_cast<int>(i) }));
			}
			for (EntityID id : ids) {
				world.add_component<MigB>(id, MigB { 0 });
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		// Tag adds skip per-row data copy for the tag column but still migrate the archetype.
		bench.batch(n).run("add tag (zero-size)" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(MigA { static_cast<int>(i) }));
			}
			for (EntityID id : ids) {
				world.add_component<MigTag>(id);
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

// Per-entity remove_component: shrinks the archetype signature, also forces a migration.
TEST_CASE("remove_component (archetype migration)", "[benchmarks][benchmark-ecs][ecs-migration]") {
	ankerl::nanobench::Bench bench;
	bench.title("remove_component archetype migration").unit("migration");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("remove 1 of 2 components" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(MigA { static_cast<int>(i) }, MigB { 0 }));
			}
			for (EntityID id : ids) {
				world.remove_component<MigB>(id);
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		bench.batch(n).run("remove tag (zero-size)" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(MigA { static_cast<int>(i) }, MigTag {}));
			}
			for (EntityID id : ids) {
				world.remove_component<MigTag>(id);
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

// Toggle a tag on/off across the entire population — measures back-and-forth migration cost.
// Repeated archetype transitions force the row to bounce between two archetypes per cycle.
TEST_CASE("add+remove tag toggle cycle", "[benchmarks][benchmark-ecs][ecs-migration]") {
	ankerl::nanobench::Bench bench;
	bench.title("toggle cycle (add + remove tag)").unit("migration");

	for (std::size_t n : COUNTS) {
		// 2 migrations per entity per cycle (add then remove).
		bench.batch(n * 2).run("toggle MigTag" + suffix(n), [&] {
			World world;
			std::vector<EntityID> ids;
			ids.reserve(n);
			for (std::size_t i = 0; i < n; ++i) {
				ids.push_back(world.create_entity(MigA { static_cast<int>(i) }));
			}
			for (EntityID id : ids) {
				world.add_component<MigTag>(id);
			}
			for (EntityID id : ids) {
				world.remove_component<MigTag>(id);
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

// Migration cost depends on the size of the row being copied. Heavier components mean
// more bytes moved per migration; this scenario fixes N at 10k and varies the destination
// archetype's row width.
TEST_CASE("add_component cost vs row width", "[benchmarks][benchmark-ecs][ecs-migration]") {
	ankerl::nanobench::Bench bench;
	bench.title("row-width sensitivity").unit("migration");
	constexpr std::size_t n = 10000;

	bench.batch(n).run("{A} → {A,B}", [&] {
		World world;
		std::vector<EntityID> ids;
		ids.reserve(n);
		for (std::size_t i = 0; i < n; ++i) {
			ids.push_back(world.create_entity(MigA {}));
		}
		for (EntityID id : ids) {
			world.add_component<MigB>(id, MigB {});
		}
		ankerl::nanobench::doNotOptimizeAway(world);
	});

	bench.batch(n).run("{A,B} → {A,B,C}", [&] {
		World world;
		std::vector<EntityID> ids;
		ids.reserve(n);
		for (std::size_t i = 0; i < n; ++i) {
			ids.push_back(world.create_entity(MigA {}, MigB {}));
		}
		for (EntityID id : ids) {
			world.add_component<MigC>(id, MigC {});
		}
		ankerl::nanobench::doNotOptimizeAway(world);
	});

	bench.batch(n).run("{A,B,C} → {A,B,C,MigTag}", [&] {
		World world;
		std::vector<EntityID> ids;
		ids.reserve(n);
		for (std::size_t i = 0; i < n; ++i) {
			ids.push_back(world.create_entity(MigA {}, MigB {}, MigC {}));
		}
		for (EntityID id : ids) {
			world.add_component<MigTag>(id);
		}
		ankerl::nanobench::doNotOptimizeAway(world);
	});
}
