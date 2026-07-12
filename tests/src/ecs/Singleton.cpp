#include "openvic-simulation/ecs/World.hpp"

#include <string>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct Config {
		int tick_rate = 0;
		int max_units = 0;
	};
	struct Banner {
		std::string text;
	};
	// Checksum enforcement: heap-holding singleton — walk size then bytes in index order.
	inline uint64_t ecs_checksum(Banner const& banner, uint64_t seed) {
		uint64_t h = fold_uint64(banner.text.size(), seed);
		return fnv1a_64_bytes(banner.text.data(), banner.text.size(), h);
	}
	struct EmptyTagS {};
	struct CompS {
		int v = 0;
	};
}

ECS_COMPONENT(Config, "test_Singleton::Config")
ECS_COMPONENT(Banner, "test_Singleton::Banner")
ECS_COMPONENT(EmptyTagS, "test_Singleton::EmptyTagS")
ECS_COMPONENT(CompS, "test_Singleton::CompS")

TEST_CASE("get_singleton returns nullptr when unset", "[ecs][World][singleton]") {
	World world;
	CHECK(world.get_singleton<Config>() == nullptr);

	World const& cw = world;
	CHECK(cw.get_singleton<Config>() == nullptr);
}

TEST_CASE("set_singleton stores value and returns pointer", "[ecs][World][singleton]") {
	World world;
	Config* p = world.set_singleton(Config { 30, 1000 });
	CHECK(p != nullptr);
	CHECK(p->tick_rate == 30);
	CHECK(p->max_units == 1000);

	Config* g = world.get_singleton<Config>();
	CHECK(g == p);
	CHECK(g->tick_rate == 30);
}

TEST_CASE("set_singleton default-constructs when called without value", "[ecs][World][singleton]") {
	World world;
	Config* p = world.set_singleton<Config>();
	CHECK(p != nullptr);
	CHECK(p->tick_rate == 0);
	CHECK(p->max_units == 0);
}

TEST_CASE("set_singleton overwrites an existing value in place", "[ecs][World][singleton]") {
	World world;
	Config* original = world.set_singleton(Config { 30, 1000 });
	Config* updated = world.set_singleton(Config { 60, 2000 });
	CHECK(updated == original); // same allocation
	CHECK(updated->tick_rate == 60);
	CHECK(updated->max_units == 2000);
}

TEST_CASE("set_singleton (default) on already-set replaces with default", "[ecs][World][singleton]") {
	World world;
	world.set_singleton(Config { 60, 2000 });
	Config* p = world.set_singleton<Config>();
	CHECK(p->tick_rate == 0);
	CHECK(p->max_units == 0);
}

TEST_CASE("clear_singleton removes the stored value", "[ecs][World][singleton]") {
	World world;
	world.set_singleton(Config { 60, 100 });
	CHECK(world.get_singleton<Config>() != nullptr);

	bool cleared = world.clear_singleton<Config>();
	CHECK(cleared);
	CHECK(world.get_singleton<Config>() == nullptr);
}

TEST_CASE("clear_singleton when unset returns false", "[ecs][World][singleton]") {
	World world;
	CHECK_FALSE(world.clear_singleton<Config>());
}

TEST_CASE("singletons of different types are independent", "[ecs][World][singleton]") {
	World world;
	world.set_singleton(Config { 30, 1000 });
	world.set_singleton(Banner { "hello" });

	CHECK(world.get_singleton<Config>()->tick_rate == 30);
	CHECK(world.get_singleton<Banner>()->text == "hello");

	world.clear_singleton<Config>();
	CHECK(world.get_singleton<Config>() == nullptr);
	CHECK(world.get_singleton<Banner>() != nullptr);
}

TEST_CASE("singleton with non-trivial dtor is destroyed correctly", "[ecs][World][singleton]") {
	World world;
	{
		World scratch;
		scratch.set_singleton(Banner { std::string(64, 'X') });
		CHECK(scratch.get_singleton<Banner>()->text.size() == 64u);
		// scratch's destructor must call ~Banner() — no leak detector here, but exercise the path.
	}
	world.set_singleton(Banner { "still alive" });
	CHECK(world.get_singleton<Banner>()->text == "still alive");
}

TEST_CASE("singleton tag (zero-size) types work", "[ecs][World][singleton][tag]") {
	World world;
	EmptyTagS* p = world.set_singleton<EmptyTagS>();
	CHECK(p != nullptr);

	EmptyTagS* g = world.get_singleton<EmptyTagS>();
	CHECK(g == p);

	bool cleared = world.clear_singleton<EmptyTagS>();
	CHECK(cleared);
	CHECK(world.get_singleton<EmptyTagS>() == nullptr);
}

TEST_CASE("set_singleton accepts rvalues and move-constructs", "[ecs][World][singleton]") {
	World world;
	Banner b { "moved" };
	Banner* p = world.set_singleton(std::move(b));
	CHECK(p->text == "moved");
}

TEST_CASE("get_singleton const overload returns const pointer", "[ecs][World][singleton]") {
	World world;
	world.set_singleton(Config { 30, 0 });
	World const& cw = world;
	Config const* p = cw.get_singleton<Config>();
	CHECK(p != nullptr);
	CHECK(p->tick_rate == 30);
}

TEST_CASE("singletons survive entity / archetype operations", "[ecs][World][singleton]") {
	World world;
	world.set_singleton(Config { 30, 1000 });

	auto e = world.create_entity(CompS { 1 });
	world.add_component<Banner>(e, Banner { "x" });
	world.destroy_entity(e);

	Config* p = world.get_singleton<Config>();
	CHECK(p != nullptr);
	CHECK(p->tick_rate == 30);
}
