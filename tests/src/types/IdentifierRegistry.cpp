#include "openvic-simulation/types/IdentifierRegistry.hpp"

#include <string_view>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>
#include <dryad/node_map.hpp>
#include <dryad/tree.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

#include "Helper.hpp"
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ast;
using namespace std::string_view_literals;

namespace {
	/* Minimal AST builder, mirroring tests/src/dataloader/NodeTools.cpp, for feeding
	 * identifier/string nodes into NodeTools callbacks without parsing files. */
	struct Ast : ovdl::SymbolIntern {
		dryad::tree<OpenVic::ast::FileTree> tree;
		symbol_interner_type symbol_interner;

		template<typename T, typename... Args>
		T* create_with_intern(std::string_view view, Args&&... args) {
			auto intern = symbol_interner.intern(view.data(), view.size());
			return tree.template create<T>(intern, DRYAD_FWD(args)...);
		}
	};

	/* Lightweight registry item carrying both a string identifier and a typed index. */
	struct TestItem : HasIdentifier, HasIndex<TestItem, good_index_t> {
		TestItem(index_t new_index, std::string_view new_identifier)
			: HasIdentifier { new_identifier }, HasIndex<TestItem, good_index_t> { new_index } {}
		TestItem(TestItem&&) = default;
	};
}

TEST_CASE("IdentifierRegistry add and retrieve", "[IdentifierRegistry]") {
	IdentifierRegistry<TestItem> registry { "test" };

	CHECK(registry.emplace_item("alpha", good_index_t { 0 }, "alpha"));
	CHECK(registry.emplace_item("beta", good_index_t { 1 }, "beta"));
	CHECK(registry.size() == 2);

	TestItem const* alpha = registry.get_item_by_identifier("alpha");
	REQUIRE(alpha != nullptr);
	CHECK(alpha->get_identifier() == "alpha"sv);
	CHECK(alpha->index == good_index_t { 0 });

	TestItem const* beta = registry.get_item_by_identifier("beta");
	REQUIRE(beta != nullptr);
	CHECK(beta->index == good_index_t { 1 });

	CHECK(registry.get_item_by_identifier("gamma") == nullptr);
	CHECK(registry.has_identifier("alpha"));
	CHECK_FALSE(registry.has_identifier("gamma"));
}

TEST_CASE("IdentifierRegistry rejects duplicate identifier", "[IdentifierRegistry]") {
	IdentifierRegistry<TestItem> registry { "test" };

	CHECK(registry.emplace_item("alpha", good_index_t { 0 }, "alpha"));
	/* Duplicate uses the default duplicate_fail_callback, which returns false and logs an error. */
	CHECK_FALSE(registry.emplace_item("alpha", good_index_t { 1 }, "alpha"));
	CHECK(registry.size() == 1);
}

TEST_CASE("IdentifierRegistry lock blocks further adds", "[IdentifierRegistry]") {
	IdentifierRegistry<TestItem> registry { "test", false };

	CHECK(registry.emplace_item("alpha", good_index_t { 0 }, "alpha"));
	CHECK_FALSE(registry.is_locked());

	registry.lock();
	CHECK(registry.is_locked());

	/* Adding after lock fails. */
	CHECK_FALSE(registry.emplace_item("beta", good_index_t { 1 }, "beta"));
	CHECK(registry.size() == 1);
}

TEST_CASE("IdentifierRegistry expect_item_identifier", "[IdentifierRegistry]") {
	IdentifierRegistry<TestItem> registry { "test" };
	REQUIRE(registry.emplace_item("alpha", good_index_t { 0 }, "alpha"));

	Ast ast;
	auto* alpha_id = ast.create_with_intern<IdentifierValue>("alpha"sv);

	bool invoked = false;
	CHECK(registry.expect_item_identifier(
		[&invoked](TestItem const& item) -> bool {
			invoked = true;
			CHECK_RETURN_BOOL(item.get_identifier() == "alpha"sv);
		}
	)(alpha_id));
	CHECK(invoked);
}

TEST_CASE("IdentifierRegistry expect_item_index", "[IdentifierRegistry]") {
	IdentifierRegistry<TestItem> registry { "test" };
	REQUIRE(registry.emplace_item("alpha", good_index_t { 0 }, "alpha"));
	REQUIRE(registry.emplace_item("beta", good_index_t { 1 }, "beta"));

	Ast ast;
	auto* beta_id = ast.create_with_intern<IdentifierValue>("beta"sv);
	auto* unknown_id = ast.create_with_intern<IdentifierValue>("gamma"sv);

	/* Known identifier delivers the item's typed index. */
	good_index_t result { 0 };
	bool invoked = false;
	CHECK(registry.expect_item_index(
		[&result, &invoked](good_index_t index) -> bool {
			invoked = true;
			result = index;
			return true;
		}
	)(beta_id));
	CHECK(invoked);
	CHECK(result == good_index_t { 1 });

	/* Unknown identifier: callback not invoked, returns false without warn. */
	invoked = false;
	CHECK_FALSE(registry.expect_item_index(
		[&invoked](good_index_t) -> bool {
			invoked = true;
			return true;
		}
	)(unknown_id));
	CHECK_FALSE(invoked);

	/* Unknown identifier with warn = true returns true, callback still not invoked. */
	invoked = false;
	CHECK(registry.expect_item_index(
		[&invoked](good_index_t) -> bool {
			invoked = true;
			return true;
		},
		true
	)(unknown_id));
	CHECK_FALSE(invoked);
}

TEST_CASE("IdentifierRegistry expect_item_index_str", "[IdentifierRegistry]") {
	IdentifierRegistry<TestItem> registry { "test" };
	REQUIRE(registry.emplace_item("alpha", good_index_t { 0 }, "alpha"));
	REQUIRE(registry.emplace_item("beta", good_index_t { 1 }, "beta"));

	/* Known key resolves to its typed index via the string path. */
	good_index_t result { 0 };
	bool invoked = false;
	CHECK(registry.expect_item_index_str(
		[&result, &invoked](good_index_t index) -> bool {
			invoked = true;
			result = index;
			return true;
		}
	)("beta"sv));
	CHECK(invoked);
	CHECK(result == good_index_t { 1 });

	/* Unknown key: callback not invoked, returns false without warn. */
	invoked = false;
	CHECK_FALSE(registry.expect_item_index_str(
		[&invoked](good_index_t) -> bool {
			invoked = true;
			return true;
		}
	)("gamma"sv));
	CHECK_FALSE(invoked);

	/* Unknown key with warn = true returns true, callback still not invoked. */
	invoked = false;
	CHECK(registry.expect_item_index_str(
		[&invoked](good_index_t) -> bool {
			invoked = true;
			return true;
		},
		false, true
	)("gamma"sv));
	CHECK_FALSE(invoked);
}
