#include "Codegen.hpp"

#include <cctype>
#include <string_view>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include <range/v3/algorithm/equal.hpp>

#include "tsl/ordered_map.h"
#include <lauf/asm/builder.h>

using namespace OpenVic::Vm;
using namespace ovdl::v2script::ast;
using namespace ovdl::v2script;

using namespace std::string_view_literals;

bool ichar_equals(char a, char b) {
	return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

void Codegen::generate_effect_from(Parser const& parser, Node* node) {
	bool is_prepping_args = false;
	tsl::ordered_map<std::string_view, std::string_view> prepped_arguments;

	dryad::visit_tree(
		node, //
		[&](dryad::child_visitor<NodeKind> visitor, AssignStatement* statement) {
			auto const* left = dryad::node_try_cast<FlatValue>(statement->left());
			if (!left) {
				return;
			}

			auto const* right = dryad::node_try_cast<FlatValue>(statement->right());
			if (!right) {
				is_prepping_args = true; // TODO: determine if scope or list-arg effect
				visitor(right);
			} else if (ranges::equal(right->value().view(), "yes"sv, ichar_equals)) {
				// TODO: insert vic2 bytecode scope object address here
				// TODO: calls a builtin for Victoria 2 effects?
			} else if (!ranges::equal(right->value().view(), "no"sv, ichar_equals)) {
				// TODO: single argument execution
			}
		},
		[&](FlatValue* value) {
			// TODO: handle right side
		}
	);
}

void Codegen::generate_condition_from(Parser const& parser, Node* node) {
	bool is_prepping_args = false;
	tsl::ordered_map<std::string_view, std::string_view> prepped_arguments;

	dryad::visit_tree(
		node, //
		[&](dryad::child_visitor<NodeKind> visitor, AssignStatement* statement) {
			auto const* left = dryad::node_try_cast<FlatValue>(statement->left());
			if (!left) {
				return;
			}

			auto const* right = dryad::node_try_cast<FlatValue>(statement->right());
			if (!right) {
				is_prepping_args = true; // TODO: determine if scope or list-arg effect
				visitor(right);
			} else if (ranges::equal(right->value().view(), "yes"sv, ichar_equals)) {
				// TODO: insert vic2 bytecode scope object address here
				// TODO: calls a builtin for Victoria 2 triggers?
			} else if (!ranges::equal(right->value().view(), "no"sv, ichar_equals)) {
				// TODO: single argument execution
			}
		},
		[&](FlatValue* value) {
			// TODO: handle right side
		}
	);
}
