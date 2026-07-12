#include "openvic-simulation/ecs/CommandBuffer.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <cstdint>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct ParPos {
		int64_t x = 0;
	};
	struct ParVel {
		int64_t dx = 0;
	};
}
ECS_COMPONENT(ParPos, "test_IntraSystemParallel::Pos")
ECS_COMPONENT(ParVel, "test_IntraSystemParallel::Vel")

namespace {
	struct ParMover : SystemThreaded<ParMover> {
		void tick(TickContext const& /*ctx*/, ParPos& p, ParVel const& v) {
			p.x += v.dx;
		}
	};
}
ECS_SYSTEM(ParMover)

TEST_CASE("SystemThreaded touches each row exactly once",
          "[ecs][IntraSystemParallel]") {
	for (uint32_t wc : { 1u, 2u, 4u, 8u, 16u }) {
		World world;
		world.set_ecs_worker_count(wc);

		std::vector<EntityID> ids;
		std::size_t const N = 500;
		ids.reserve(N);
		for (std::size_t i = 0; i < N; ++i) {
			ids.push_back(world.create_entity(
				ParPos { static_cast<int64_t>(i) },
				ParVel { 7 }
			));
		}

		world.register_system<ParMover>();
		world.tick_systems(Date {});

		// Each row should have advanced by exactly 7.
		for (std::size_t i = 0; i < N; ++i) {
			ParPos const* p = world.get_component<ParPos>(ids[i]);
			REQUIRE(p != nullptr);
			CHECK(p->x == static_cast<int64_t>(i) + 7);
		}
	}
}

TEST_CASE("SystemThreaded result is identical across worker counts",
          "[ecs][IntraSystemParallel][determinism]") {
	auto run = [](uint32_t wc) {
		World world;
		world.set_ecs_worker_count(wc);
		std::vector<EntityID> ids;
		std::size_t const N = 250;
		ids.reserve(N);
		for (std::size_t i = 0; i < N; ++i) {
			ids.push_back(world.create_entity(
				ParPos { static_cast<int64_t>(i + 1) },
				ParVel { static_cast<int64_t>((i * 13) % 17 + 1) }
			));
		}
		world.register_system<ParMover>();
		for (int t = 0; t < 5; ++t) {
			world.tick_systems(Date {});
		}
		int64_t digest = 0;
		for (EntityID const& id : ids) {
			ParPos const* p = world.get_component<ParPos>(id);
			REQUIRE(p != nullptr);
			digest = digest * 1000003 + p->x;
		}
		return digest;
	};

	int64_t baseline = run(1);
	for (uint32_t wc : { 2u, 4u, 8u, 16u }) {
		CHECK(run(wc) == baseline);
	}
}
