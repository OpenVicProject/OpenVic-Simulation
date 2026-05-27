#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs/Query.hpp"

#include <algorithm>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic::ecs;

namespace {
	struct QA {};
	struct QB {};
	struct QC {};
	struct QD {};
}

ECS_COMPONENT(QA, "test_Query::QA")
ECS_COMPONENT(QB, "test_Query::QB")
ECS_COMPONENT(QC, "test_Query::QC")
ECS_COMPONENT(QD, "test_Query::QD")

TEST_CASE("Query default-constructed has empty lists", "[ecs][Query]") {
	Query q;
	CHECK(q.require_ids.empty());
	CHECK(q.exclude_ids.empty());
}

TEST_CASE("Query::with appends component ids", "[ecs][Query]") {
	Query q;
	q.with<QA>();
	CHECK(q.require_ids.size() == 1u);
	CHECK(q.require_ids[0] == component_type_id_of<QA>());

	q.with<QB, QC>();
	CHECK(q.require_ids.size() == 3u);
}

TEST_CASE("Query::exclude appends component ids", "[ecs][Query]") {
	Query q;
	q.exclude<QA>();
	CHECK(q.exclude_ids.size() == 1u);
	CHECK(q.exclude_ids[0] == component_type_id_of<QA>());

	q.exclude<QB, QC>();
	CHECK(q.exclude_ids.size() == 3u);
}

TEST_CASE("Query::build sorts require_ids ascending", "[ecs][Query]") {
	Query q;
	q.with<QC, QA, QB>().build();
	CHECK(std::is_sorted(q.require_ids.begin(), q.require_ids.end()));
}

TEST_CASE("Query::build sorts exclude_ids ascending", "[ecs][Query]") {
	Query q;
	q.exclude<QC, QA, QB>().build();
	CHECK(std::is_sorted(q.exclude_ids.begin(), q.exclude_ids.end()));
}

TEST_CASE("Query::build deduplicates require and exclude lists", "[ecs][Query]") {
	Query q;
	q.with<QA, QA, QB, QA>();
	q.exclude<QC, QC, QD, QC>();
	q.build();
	CHECK(q.require_ids.size() == 2u);
	CHECK(q.exclude_ids.size() == 2u);
}

TEST_CASE("Query::build returns *this for chaining", "[ecs][Query]") {
	Query q;
	Query& chained = q.with<QA>().exclude<QB>().build();
	CHECK(&chained == &q);
}

TEST_CASE("Query equality compares both lists", "[ecs][Query]") {
	Query a;
	Query b;
	a.with<QA, QB>().build();
	b.with<QB, QA>().build();
	CHECK(a == b);

	Query c;
	c.with<QA>().build();
	CHECK_FALSE(a == c);

	Query d;
	d.with<QA, QB>().exclude<QC>().build();
	CHECK_FALSE(a == d);
}

TEST_CASE("Query supports require + exclude on same call", "[ecs][Query]") {
	Query q;
	q.with<QA, QB>().exclude<QC, QD>().build();
	CHECK(q.require_ids.size() == 2u);
	CHECK(q.exclude_ids.size() == 2u);
	CHECK(std::is_sorted(q.require_ids.begin(), q.require_ids.end()));
	CHECK(std::is_sorted(q.exclude_ids.begin(), q.exclude_ids.end()));
}

TEST_CASE("Query::build is idempotent", "[ecs][Query]") {
	Query q1;
	q1.with<QA, QB, QC>().build();
	std::vector<component_type_id_t> after_first = q1.require_ids;

	q1.build();
	CHECK(q1.require_ids == after_first);
}
