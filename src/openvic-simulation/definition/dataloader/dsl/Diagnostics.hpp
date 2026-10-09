#pragma once

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include <fmt/ranges.h>

#include <range/v3/view/transform.hpp>

#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader::diagnostic {
	namespace ast = ovdl::v2script::ast;

	namespace code_type {
		enum code_type_t : std::uint32_t {
			// Expected NodeType
			EXPECT_NODE_FILE_TREE_OR_LIST_VALUE = 0xC0100,
			EXPECT_NODE_FLAT_VALUE = 0xC0101,
			EXPECT_NODE_ASSIGN_STATEMENT = 0xC0102,

			//
			TYPE_CONVERSION_FAIL = 0xC0200,

			// Expect Check
			EXPECT_KEY = 0xC0300,
			EXPECT_VALUE = 0xC0301,

			// Duplicate Check
			FOUND_DUPLICATE_KEY = 0xC0400,
			FOUND_DUPLICATE_NODE = 0xC0401,
			FOUND_DUPLICATE_MAP_KEY = 0xC0402,

			// Unknown Check
			FOUND_UNKNOWN_KEY = 0xC0500,
			FOUND_UNKNOWN_NODE = 0xC0501
		};
	}

	inline void expected_file_tree_or_list_value(dsl::TraverseContext& ctx, ast::Node const* node) {
		ctx.diagnostic.error(node)
		    .with_message("Expected a file tree or list value, found {}", get_kind_name(node->kind()))
		    .with_code(code_type::EXPECT_NODE_FILE_TREE_OR_LIST_VALUE);
	}

	inline void expected_flat_value(dsl::TraverseContext& ctx, ast::Node const* node) {
		ctx.diagnostic.error(node)
		    .with_message("Expected a flat value, found {}", get_kind_name(node->kind()))
		    .with_code(code_type::EXPECT_NODE_FLAT_VALUE);
	}

	inline void expected_assign_statement(dsl::TraverseContext& ctx, ast::Node const* node) {
		ctx.diagnostic.error(node)
		    .with_message("Expected a assign statement, found {}", get_kind_name(node->kind()))
		    .with_code(code_type::EXPECT_NODE_ASSIGN_STATEMENT);
	}

	inline void type_conversion_failed(dsl::TraverseContext& ctx, ast::Node const* node, std::string_view type_name) {
		ctx.diagnostic.error(node)
		    .with_message("Could not convert to type {}", type_name)
		    .with_code(code_type::TYPE_CONVERSION_FAIL);
	}

	inline void expected_keys(
	    dsl::TraverseContext& ctx, ast::Node const* node, std::span<const std::string_view> expected_keys
	) {
		if (expected_keys.size() == 1) {
			ctx.diagnostic.error(node).with_message("Expected key: {}", expected_keys[0]).with_code(code_type::EXPECT_KEY);
			return;
		}

		ctx.diagnostic.error(node)
		    .with_message("Expected keys: [{}]", fmt::join(expected_keys, ", "))
		    .with_code(code_type::EXPECT_KEY);
	}

	inline void expected_values(
	    dsl::TraverseContext& ctx, ast::Node const* node, std::span<const std::string_view> expected_values
	) {
		if (expected_values.size() == 1) {
			ctx.diagnostic.error(node)
			    .with_message("Expected value: {}", expected_values[0])
			    .with_code(code_type::EXPECT_VALUE);
			return;
		}

		ctx.diagnostic.error(node)
		    .with_message("Expected values: [{}]", fmt::join(expected_values, ", "))
		    .with_code(code_type::EXPECT_VALUE);
	}

	inline void found_duplicate_key(
	    dsl::TraverseContext& ctx, ast::Node const* current_node, ast::Node const* prev_node, dataloader::DiagnosticLevel level
	) {
		dryad::visit_node_all(
		    current_node,
		    [&](ast::AssignStatement const* as) {
			    auto prev_key = dryad::visit_node_all(
			        prev_node,
			        [](ast::AssignStatement const* as) {
				        return as->left();
			        },
			        [](ast::Node const* node) {
				        return node;
			        }
			    );

			    if (auto const* fv = dryad::node_try_cast<ast::FlatValue>(as->left()); fv != nullptr) {
				    if (auto const* fv2 = dryad::node_try_cast<ast::FlatValue>(as->right()); fv2 != nullptr) {
					    ctx.diagnostic.report(level, fv)
					        .with_message("Duplicate key with value: [{}, {}]", fv->value().view(), fv2->value().view())
					        .with_code(code_type::FOUND_DUPLICATE_KEY)
					        .hint(prev_key)
					        .with_message("previous key")
					        .with_code(code_type::FOUND_DUPLICATE_KEY);
					    return;
				    }

				    ctx.diagnostic.report(level, fv)
				        .with_message("Duplicate key: {}", fv->value().view())
				        .with_code(code_type::FOUND_DUPLICATE_KEY)
				        .hint(prev_key)
				        .with_message("previous key")
				        .with_code(code_type::FOUND_DUPLICATE_KEY);
			    }
		    },
		    [&](ast::Node const* node) {
			    ctx.diagnostic.report(level, node)
			        .with_message("Duplicate node: {}", get_kind_name(node->kind()))
			        .with_code(code_type::FOUND_DUPLICATE_NODE)
			        .hint(prev_node)
			        .with_message("previous node")
			        .with_code(code_type::FOUND_DUPLICATE_NODE);
		    }
		);
	}

	inline void found_duplicate_map_key(dsl::TraverseContext& ctx, ast::Node const* node, std::string_view key) {
		dryad::visit_node_all(
		    node,
		    [&](ast::AssignStatement const* as) {
			    ctx.diagnostic.warning(as->left())
			        .with_message("Duplicate map key failed: {}", key)
			        .with_code(code_type::FOUND_DUPLICATE_MAP_KEY);
		    },
		    [&](ast::Node const* node) {
			    ctx.diagnostic.warning(node)
			        .with_message("Duplicate map key failed: {}", key)
			        .with_code(code_type::FOUND_DUPLICATE_MAP_KEY);
		    }
		);
	}

	inline void found_unknown_key(dsl::TraverseContext& ctx, ast::Node const* node, dataloader::DiagnosticLevel level) {
		dryad::visit_node_all(
		    node,
		    [&](ast::AssignStatement const* as) {
			    if (auto const* fv = dryad::node_try_cast<ast::FlatValue>(as->left()); fv != nullptr) {
				    if (auto const* fv2 = dryad::node_try_cast<ast::FlatValue>(as->right()); fv2 != nullptr) {
					    ctx.diagnostic.report(level, fv)
					        .with_message("Unknown key with value: [{}, {}]", fv->value().view(), fv2->value().view())
					        .with_code(code_type::FOUND_UNKNOWN_KEY);
					    return;
				    }

				    ctx.diagnostic.report(level, fv)
				        .with_message("Unknown key: {}", fv->value().view())
				        .with_code(code_type::FOUND_UNKNOWN_KEY);
			    }
		    },
		    [&](ast::Node const* node) {
			    ctx.diagnostic.report(level, node)
			        .with_message("Unknown node: {}", get_kind_name(node->kind()))
			        .with_code(code_type::FOUND_UNKNOWN_NODE);
		    }
		);
	}
}
