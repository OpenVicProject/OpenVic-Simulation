#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <nanobench.h>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct ImmBenchValue {
		int64_t v = 0;
	};
	struct ImmBenchDelta {
		int64_t d = 0;
	};
}

ECS_COMPONENT(ImmBenchValue, "bench_ImmutableEntity::ImmBenchValue")
ECS_COMPONENT(ImmBenchDelta, "bench_ImmutableEntity::ImmBenchDelta")

namespace {
	// Identical work, identical signature shape — only the per-row handle type differs. These two
	// systems exist so the bench can show that yielding an ImmutableEntityID from the tick driver
	// costs nothing versus the established EntityID path.
	struct EidTick : System<EidTick> {
		void tick(TickContext const& /*ctx*/, EntityID eid, ImmBenchValue& v, ImmBenchDelta const& d) {
			v.v = v.v * 31 + d.d * 7 + static_cast<int64_t>(eid.index & 1u);
		}
	};
	struct ImmIdTick : System<ImmIdTick> {
		void tick(TickContext const& /*ctx*/, ImmutableEntityID eid, ImmBenchValue& v, ImmBenchDelta const& d) {
			v.v = v.v * 31 + d.d * 7 + static_cast<int64_t>(eid.index & 1u);
		}
	};
}

ECS_SYSTEM(EidTick)
ECS_SYSTEM(ImmIdTick)

namespace {
	constexpr std::size_t COUNTS[] = { 1000, 10000, 100000 };

	std::string suffix(std::size_t n) {
		return " N=" + std::to_string(n);
	}
}

// create_immutable_entity sets one extra slot bool versus create_entity. This bench confirms that
// difference is in the noise — the structural cost (archetype lookup, row reserve, placement-new)
// dominates.
TEST_CASE("create_immutable_entity vs create_entity throughput", "[benchmarks][benchmark-ecs][ecs-immutable]") {
	ankerl::nanobench::Bench bench;
	bench.title("create_immutable_entity vs create_entity").unit("entity");

	for (std::size_t n : COUNTS) {
		bench.batch(n).run("create_entity (mutable)" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(ImmBenchValue { static_cast<int64_t>(i) }, ImmBenchDelta { 3 });
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});

		bench.batch(n).run("create_immutable_entity" + suffix(n), [&] {
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_immutable_entity(ImmBenchValue { static_cast<int64_t>(i) }, ImmBenchDelta { 3 });
			}
			ankerl::nanobench::doNotOptimizeAway(world);
		});
	}
}

// The immutable-handle tick path wraps the yielded EntityID into an ImmutableEntityID via a
// compile-time branch. This bench confirms the wrap folds away — the two systems should tick at
// the same rate.
TEST_CASE("System tick: EntityID handle vs ImmutableEntityID handle", "[benchmarks][benchmark-ecs][ecs-immutable]") {
	ankerl::nanobench::Bench bench;
	bench.title("tick handle type (EntityID vs ImmutableEntityID)").unit("entity");

	for (std::size_t n : COUNTS) {
		{
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(
					ImmBenchValue { static_cast<int64_t>(i + 1) },
					ImmBenchDelta { static_cast<int64_t>((i * 17) % 13 + 1) }
				);
			}
			world.register_system<EidTick>();
			bench.batch(n).run("tick(EntityID)" + suffix(n), [&] {
				world.tick_systems(Date {});
			});
		}
		{
			World world;
			for (std::size_t i = 0; i < n; ++i) {
				world.create_entity(
					ImmBenchValue { static_cast<int64_t>(i + 1) },
					ImmBenchDelta { static_cast<int64_t>((i * 17) % 13 + 1) }
				);
			}
			world.register_system<ImmIdTick>();
			bench.batch(n).run("tick(ImmutableEntityID)" + suffix(n), [&] {
				world.tick_systems(Date {});
			});
		}
	}
}
