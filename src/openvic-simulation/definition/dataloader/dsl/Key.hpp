#pragma once

#include <string_view>
#include <type_traits>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include "openvic-simulation/core/string/StringLiteral.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Assignment.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Diagnostics.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader::dsl {
	template<string_literal Key, rule Rule>
	struct _key : Rule {
		ovdl::symbol<> symbol;

		void event_prepare(TraverseContext& ctx) {
			symbol = ctx.find_intern(Key);
			if constexpr (rule_has_prepare<Rule>) {
				Rule::event_prepare(ctx);
			}
		}

		template<typename TraverseData, typename Node>
		auto event_match(TraverseContext& ctx, TraverseData& data, Node const* node) const {
			if constexpr (node_with_value_symbol<Node>) {
				if (!symbol) {
					if (OV_likely(ctx.parser())) {
						return false;
					}

					if (Key != node->value().view()) {
						return false;
					}
				} else if constexpr (node_with_value_symbol<Node>) {
					if (symbol != node->value()) {
						return false;
					}
				}

				if constexpr (choice_rule<Rule, Node>) {
					if (!Rule::event_match(ctx, data, node)) {
						return false;
					}
				}

				return true;
			} else {
				return visit_flat(ctx, data, node, [this](TraverseContext& ctx, TraverseData& data, ast::FlatValue const* fv) {
					return event_match(ctx, data, fv);
				});
			}
		}

		template<typename TraverseData, typename Node>
		requires rule_has_key<Rule, Node>
		std::string_view event_expected(TraverseContext& ctx, TraverseData& data, Node const* parent_node, bool seen) const {
			if (!Rule::event_has_key(ctx, data, parent_node, seen)) {
				return Key;
			}
			return std::string_view {};
		}

		template<typename TraverseData, typename CurNode>
		requires rule_has_duplicate<Rule, CurNode>
		bool event_duplicate(
		    TraverseContext& ctx, TraverseData& data, CurNode const* cur_node, ast::Node const* prev_node, DiagnosticLevel level
		) const {
			if (!Rule::event_duplicate(ctx, data, cur_node, prev_node, level)) {
				diagnostic::found_duplicate_key(ctx, cur_node, prev_node, level);
				return false;
			}

			return true;
		}

		template<rule RightRule>
		constexpr auto operator=(RightRule&& right_rule) const {
			return assignment(_key { *this }, OV_FWD(right_rule));
		}
	};

	struct _kexpect {
		template<typename TraverseData, typename Node>
		std::true_type event_process(TraverseContext&, TraverseData&, Node const*) const {
			return {};
		}

		template<typename TraverseData, typename Node>
		bool event_has_key(TraverseContext&, TraverseData&, Node const*, bool seen) const {
			return seen;
		}

		template<typename TraverseData, typename CurNode>
		auto event_duplicate(TraverseContext&, TraverseData&, CurNode const*, ast::Node const*, DiagnosticLevel) const {
			return std::false_type {};
		}
	};

	template<>
	struct rule_is_required<_kexpect> : std::true_type {};

	template<string_literal Key, typename T>
	struct rule_is_required<_key<Key, T>> : rule_is_required<T> {};

	struct _kopt {
		template<typename TraverseData, typename Node>
		std::true_type event_process(TraverseContext&, TraverseData&, Node const*) const {
			return {};
		}

		template<typename TraverseData, typename CurNode>
		auto event_duplicate(TraverseContext&, TraverseData&, CurNode const*, ast::Node const*, DiagnosticLevel) const {
			return std::false_type {};
		}
	};

	struct _kmany {
		template<typename TraverseData, typename Node>
		std::true_type event_process(TraverseContext&, TraverseData&, Node const*) const {
			return {};
		}

		template<typename TraverseData, typename CurNode>
		auto event_duplicate(TraverseContext&, TraverseData&, CurNode const*, ast::Node const*, DiagnosticLevel) const {
			return std::true_type {};
		}
	};

	template<string_literal Key>
	inline constexpr _key<Key, _kexpect> expect {};

	template<string_literal Key>
	inline constexpr _key<Key, _kopt> optional {};

	template<string_literal Key>
	inline constexpr _key<Key, _kmany> many {};
}
