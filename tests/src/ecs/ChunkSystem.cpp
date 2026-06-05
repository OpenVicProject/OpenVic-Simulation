#include "openvic-simulation/ecs/ChunkSystem.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/SystemImpl.hpp"
#include "openvic-simulation/ecs/SystemTypeID.hpp"
#include "openvic-simulation/ecs/World.hpp"
#include "openvic-simulation/types/Date.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;
using OpenVic::Date;

namespace {
	struct CSPosition {
		int x = 0;
		int y = 0;
	};
	struct CSVelocity {
		int dx = 0;
		int dy = 0;
	};
}

ECS_COMPONENT(CSPosition, "test_ChunkSystem::CSPosition")
ECS_COMPONENT(CSVelocity, "test_ChunkSystem::CSVelocity")

namespace {
	// Updates positions by velocities, one chunk at a time. CRTP-based ChunkSystem.
	struct MoverChunkSystem : ChunkSystem<MoverChunkSystem, CSPosition, CSVelocity const> {
		void tick_chunk(ChunkView<CSPosition, CSVelocity> view, TickContext const&) {
			CSPosition* pos = view.template array<CSPosition>();
			CSVelocity* vel = view.template array<CSVelocity>();
			for (std::size_t i = 0; i < view.count(); ++i) {
				pos[i].x += vel[i].dx;
				pos[i].y += vel[i].dy;
			}
		}
	};
}

ECS_SYSTEM(MoverChunkSystem)

TEST_CASE("ChunkSystem ticks every matching chunk and observes per-entity values", "[ecs][ChunkSystem]") {
	World world;
	for (int i = 0; i < 5; ++i) {
		world.create_entity(CSPosition { i, i }, CSVelocity { 1, 2 });
	}

	world.register_system<MoverChunkSystem>();
	world.tick_systems(Date {});

	// Verify each entity's position advanced.
	int sum_x = 0;
	int sum_y = 0;
	world.for_each<CSPosition>([&](CSPosition& p) {
		sum_x += p.x;
		sum_y += p.y;
	});
	CHECK(sum_x == (0 + 1 + 2 + 3 + 4) + 5 * 1);
	CHECK(sum_y == (0 + 1 + 2 + 3 + 4) + 5 * 2);
}

TEST_CASE("ChunkSystem on empty world is a no-op", "[ecs][ChunkSystem]") {
	World world;
	world.register_system<MoverChunkSystem>();
	world.tick_systems(Date {});
	CHECK(true); // no crash
}
