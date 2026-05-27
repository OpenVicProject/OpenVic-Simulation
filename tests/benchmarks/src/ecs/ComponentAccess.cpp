#include "openvic-simulation/ecs/CachedRef.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct AccA {
		int v = 0;
	};
	struct AccB {
		float w = 0.0f;
	};
	// One float, no padding — author-asserted byte hash for the checksum enforcement.
	ECS_CHECKSUM_BYTES(AccB)
	struct AccTag {};

	struct AccSingleton {
		int64_t counter = 0;
	};
}

ECS_COMPONENT(AccA, "bench_ComponentAccess::AccA")
ECS_COMPONENT(AccB, "bench_ComponentAccess::AccB")
ECS_COMPONENT(AccTag, "bench_ComponentAccess::AccTag")
ECS_COMPONENT(AccSingleton, "bench_ComponentAccess::AccSingleton")

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}
}

TEST_CASE("get_component hit (sequential vs random)", "[benchmarks][benchmark-ecs][ecs-access]") {
	ankerl::nanobench::Bench bench;
	bench.title("get_component hit").unit("lookup");

	for (std::size_t n : COUNTS) {
		World world;
		std::vector<EntityID> ids;
		ids.reserve(n);
		for (std::size_t i = 0; i < n; ++i) {
			ids.push_back(world.create_entity(AccA { static_cast<int>(i) }, AccB { 0.0f }));
		}


		// Sequential id order — matches the create order, friendliest to any per-slot prefetching.
		bench.batch(n).run("sequential ids" + suffix(n), [&] {
			int64_t acc = 0;
			for (EntityID id : ids) {
				AccA const* a = world.get_component<AccA>(id);
				acc += a->v;
			}
			ankerl::nanobench::doNotOptimizeAway(acc);
		});

		// Random id order — defeats sequential prefetching.
		std::vector<EntityID> shuffled = ids;
		std::mt19937 rng { 0xBADC0DE };
		std::shuffle(shuffled.begin(), shuffled.end(), rng);
		bench.batch(n).run("random ids" + suffix(n), [&] {
			int64_t acc = 0;
			for (EntityID id : shuffled) {
				AccA const* a = world.get_component<AccA>(id);
				acc += a->v;
			}
			ankerl::nanobench::doNotOptimizeAway(acc);
		});
	}
}

TEST_CASE("get_component miss (entity does not carry C)", "[benchmarks][benchmark-ecs][ecs-access]") {
	ankerl::nanobench::Bench bench;
	bench.title("get_component miss").unit("lookup");

	for (std::size_t n : COUNTS) {
		World world;
		std::vector<EntityID> ids;
		ids.reserve(n);
		// Entities carry AccA only — get_component<AccB> always misses.
		for (std::size_t i = 0; i < n; ++i) {
			ids.push_back(world.create_entity(AccA { static_cast<int>(i) }));
		}

		bench.batch(n).run("get_component<AccB> miss" + suffix(n), [&] {
			std::size_t miss_count = 0;
			for (EntityID id : ids) {
				if (world.get_component<AccB>(id) == nullptr) {
					++miss_count;
				}
			}
			ankerl::nanobench::doNotOptimizeAway(miss_count);
		});
	}
}

TEST_CASE("has_component<Tag>", "[benchmarks][benchmark-ecs][ecs-access]") {
	ankerl::nanobench::Bench bench;
	bench.title("has_component<Tag>").unit("lookup");

	for (std::size_t n : COUNTS) {
		World world;
		std::vector<EntityID> ids;
		ids.reserve(n);
		// Half tagged, half not — branch mix exercises both paths.
		for (std::size_t i = 0; i < n; ++i) {
			if (i % 2 == 0) {
				ids.push_back(world.create_entity(AccA { static_cast<int>(i) }));
			} else {
				ids.push_back(world.create_entity(AccA { static_cast<int>(i) }, AccTag {}));
			}
		}

		bench.batch(n).run("has_component<AccTag>" + suffix(n), [&] {
			std::size_t tagged = 0;
			for (EntityID id : ids) {
				if (world.has_component<AccTag>(id)) {
					++tagged;
				}
			}
			ankerl::nanobench::doNotOptimizeAway(tagged);
		});
	}
}

TEST_CASE("get_singleton", "[benchmarks][benchmark-ecs][ecs-access]") {
	ankerl::nanobench::Bench bench;
	bench.title("get_singleton").unit("lookup");

	World world;
	world.set_singleton<AccSingleton>();

	constexpr std::size_t iters = 100000;
	bench.batch(iters).run("get_singleton<AccSingleton> x N", [&] {
		int64_t acc = 0;
		for (std::size_t i = 0; i < iters; ++i) {
			AccSingleton* s = world.get_singleton<AccSingleton>();
			acc += s->counter + static_cast<int64_t>(i);
		}
		ankerl::nanobench::doNotOptimizeAway(acc);
	});
}

TEST_CASE("CachedRef::get vs get_component", "[benchmarks][benchmark-ecs][ecs-access]") {
	ankerl::nanobench::Bench bench;
	bench.title("CachedRef vs get_component").unit("lookup");

	for (std::size_t n : COUNTS) {
		World world;
		std::vector<EntityID> ids;
		std::vector<CachedRef<AccA>> refs;
		ids.reserve(n);
		refs.reserve(n);
		for (std::size_t i = 0; i < n; ++i) {
			EntityID const eid = world.create_entity(AccA { static_cast<int>(i) }, AccB { 0.0f });
			ids.push_back(eid);
			refs.push_back(CachedRef<AccA>::from(world, eid));
		}

		bench.batch(n).run("world.get_component<AccA>" + suffix(n), [&] {
			int64_t acc = 0;
			for (EntityID id : ids) {
				AccA const* a = world.get_component<AccA>(id);
				acc += a->v;
			}
			ankerl::nanobench::doNotOptimizeAway(acc);
		});

		bench.batch(n).run("CachedRef<AccA>::get" + suffix(n), [&] {
			int64_t acc = 0;
			for (CachedRef<AccA>& ref : refs) {
				AccA* a = ref.get(world);
				acc += a->v;
			}
			ankerl::nanobench::doNotOptimizeAway(acc);
		});
	}
}
