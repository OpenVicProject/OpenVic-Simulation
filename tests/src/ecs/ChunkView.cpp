#include "openvic-simulation/ecs/Chunk.hpp"
#include "openvic-simulation/ecs/ChunkView.hpp"
#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/Query.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <set>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct CVA {
		int v = 0;
	};
	struct CVB {
		int w = 0;
	};
	struct CVTag {};
	struct CVDead {};
}

ECS_COMPONENT(CVA, "test_ChunkView::CVA")
ECS_COMPONENT(CVB, "test_ChunkView::CVB")
ECS_COMPONENT(CVTag, "test_ChunkView::CVTag")
ECS_COMPONENT(CVDead, "test_ChunkView::CVDead")

TEST_CASE("for_each_chunk visits every chunk of every matched archetype", "[ecs][ChunkView]") {
	World world;
	world.create_entity(CVA { 1 });
	world.create_entity(CVA { 2 });
	world.create_entity(CVA { 3 });

	int chunk_visits = 0;
	int total_rows = 0;
	world.for_each_chunk<CVA>([&](ChunkView<CVA> view) {
		++chunk_visits;
		total_rows += static_cast<int>(view.count());
	});
	CHECK(chunk_visits == 1);
	CHECK(total_rows == 3);
}

TEST_CASE("ChunkView::count matches the chunk's row count", "[ecs][ChunkView]") {
	World world;
	for (int i = 0; i < 7; ++i) {
		world.create_entity(CVA { i });
	}

	world.for_each_chunk<CVA>([&](ChunkView<CVA> view) {
		CHECK(view.count() == 7u);
	});
}

TEST_CASE("ChunkView::array yields the same values as for_each", "[ecs][ChunkView]") {
	World world;
	for (int i = 0; i < 5; ++i) {
		world.create_entity(CVA { i * 10 });
	}

	std::vector<int> reference;
	world.for_each<CVA>([&](CVA& c) { reference.push_back(c.v); });

	std::vector<int> via_chunk;
	world.for_each_chunk<CVA>([&](ChunkView<CVA> view) {
		CVA* arr = view.array<CVA>();
		for (std::size_t i = 0; i < view.count(); ++i) {
			via_chunk.push_back(arr[i].v);
		}
	});
	CHECK(reference == via_chunk);
}

TEST_CASE("ChunkView::array<Tag> returns nullptr", "[ecs][ChunkView][tag]") {
	World world;
	world.create_entity(CVA { 1 }, CVTag {});
	world.create_entity(CVA { 2 }, CVTag {});

	world.for_each_chunk<CVA, CVTag>([&](ChunkView<CVA, CVTag> view) {
		CHECK(view.array<CVA>() != nullptr);
		CHECK(view.array<CVTag>() == nullptr);
	});
}

TEST_CASE("ChunkView::entities returns matching EntityIDs", "[ecs][ChunkView]") {
	World world;
	EntityID a = world.create_entity(CVA { 1 });
	EntityID b = world.create_entity(CVA { 2 });
	EntityID c = world.create_entity(CVA { 3 });

	std::set<std::uint64_t> seen;
	world.for_each_chunk<CVA>([&](ChunkView<CVA> view) {
		EntityID const* eids = view.entities();
		for (std::size_t i = 0; i < view.count(); ++i) {
			seen.insert(eids[i].to_uint64());
		}
	});
	CHECK(seen.size() == 3u);
	CHECK(seen.count(a.to_uint64()) == 1u);
	CHECK(seen.count(b.to_uint64()) == 1u);
	CHECK(seen.count(c.to_uint64()) == 1u);
}

TEST_CASE("Mutations through ChunkView::array are visible to subsequent for_each", "[ecs][ChunkView]") {
	World world;
	for (int i = 0; i < 4; ++i) {
		world.create_entity(CVA { i });
	}

	world.for_each_chunk<CVA>([&](ChunkView<CVA> view) {
		CVA* arr = view.array<CVA>();
		for (std::size_t i = 0; i < view.count(); ++i) {
			arr[i].v *= 10;
		}
	});

	int sum = 0;
	world.for_each<CVA>([&](CVA& c) { sum += c.v; });
	CHECK(sum == (0 + 10 + 20 + 30));
}

TEST_CASE("for_each_chunk(Query, fn) respects exclude", "[ecs][ChunkView][query]") {
	World world;
	world.create_entity(CVA { 1 });
	world.create_entity(CVA { 2 }, CVDead {});
	world.create_entity(CVA { 3 });
	world.create_entity(CVA { 4 }, CVDead {});

	Query q;
	q.with<CVA>().exclude<CVDead>().build();

	int sum = 0;
	int total = 0;
	world.for_each_chunk<CVA>(q, [&](ChunkView<CVA> view) {
		CVA* arr = view.array<CVA>();
		for (std::size_t i = 0; i < view.count(); ++i) {
			sum += arr[i].v;
			++total;
		}
	});
	CHECK(total == 2);
	CHECK(sum == 1 + 3);
}

TEST_CASE("for_each_chunk on empty world is a no-op", "[ecs][ChunkView]") {
	World world;
	int chunk_visits = 0;
	world.for_each_chunk<CVA>([&](ChunkView<CVA>) { ++chunk_visits; });
	CHECK(chunk_visits == 0);
}
