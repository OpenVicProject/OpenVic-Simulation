#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/core/template/Concepts.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Diagnostics.hpp"

namespace OpenVic::dataloader::dsl {
	namespace ast = ovdl::v2script::ast;

	struct TraverseContext;

	template<typename Node>
	concept node_with_value_symbol = requires(Node const* node) {
		{ node->value() } -> std::same_as<ovdl::symbol<>>;
	};

	template<typename Node>
	concept node_with_left = requires(Node const* node) {
		{ node->left() } -> std::same_as<ast::Value const*>;
	};

	template<typename Node>
	concept node_with_right = requires(Node const* node) {
		{ node->right() } -> std::same_as<ast::Value const*>;
	};

	using node_type = ast::Node;
	using statement_range_iterator = node_type::_children_range<const node_type>::iterator;
	using statement_range_type = dryad::node_range<statement_range_iterator, ast::Statement>;

	template<typename Node>
	concept node_with_statements = requires(Node const* node) {
		{ node->statements() } -> std::same_as<statement_range_type>;
	};

	template<typename F, typename... Args>
	concept invocable = requires(F f, Args... args) { f(args...); };

	template<typename F, typename Return, typename... Args>
	concept invocable_r = requires(F f, Args... args) {
		{ f(args...) } -> std::same_as<Return>;
	};

	template<typename F, typename Return, typename... Args>
	concept invocable_r_convertible = requires(F f, Args... args) {
		{ f(args...) } -> std::convertible_to<Return>;
	};

	struct fake_data {
		template<typename T>
		struct FakeValueParser {
			template<typename TraverseData, typename Node>
			auto parse(dsl::TraverseContext&, TraverseData&, Node const*, auto) const {
				return std::true_type {};
			}
		};

		template<typename T>
		constexpr FakeValueParser<T> parser_for() {
			return {};
		}

		template<typename T>
		constexpr FakeValueParser<T> parser_for() const {
			return {};
		}
	};

	template<typename Sink, typename TraverseData, typename... Args>
	concept invocable_sink_with =
	    invocable_r_convertible<Sink, bool, TraverseContext&, TraverseData&, ast::Node const*, Args...>;

	template<typename Rule, typename Node = ast::Node>
	concept choice_rule = requires(Rule rule, fake_data& data, TraverseContext& c, Node const* node) {
		{ rule.event_match(c, data, node) } -> std::convertible_to<bool>;
	};

	template<typename Rule, typename Node = ast::Node>
	concept sink_rule = requires(Rule rule, fake_data& data, TraverseContext& c, Node const* node) {
		{
			rule.event_sink(c, data, node, [](TraverseContext&, fake_data&, ast::Node const*, auto&&...) {
				return true;
			})
		} -> std::convertible_to<bool>;
	};

	template<typename Rule, typename Node = ast::Node>
	concept process_rule = requires(Rule rule, fake_data& data, TraverseContext& ctx, Node const* node) {
		{ rule.event_process(ctx, data, node) } -> std::convertible_to<bool>;
	};

	template<typename Rule, typename ParentNode = ast::Node>
	concept rule_has_key = requires(
	    Rule rule, fake_data& data, TraverseContext& ctx, ParentNode const* parent_node, bool seen
	) {
		{ rule.event_has_key(ctx, data, parent_node, seen) } -> std::convertible_to<bool>;
	};

	template<typename Rule, typename ParentNode = ast::Node>
	concept rule_has_expected = requires(
	    Rule rule, fake_data& data, TraverseContext& ctx, ParentNode const* parent_node, bool seen
	) {
		{ rule.event_expected(ctx, data, parent_node, seen) } -> std::convertible_to<std::string_view>;
	};

	template<typename Rule, typename CurNode = ast::Node>
	concept rule_has_duplicate = requires(
	    Rule rule,
	    fake_data& data,
	    TraverseContext& ctx,
	    CurNode const* cur_node,
	    ast::Node const* prev_node,
	    DiagnosticLevel level
	) {
		{ rule.event_duplicate(ctx, data, cur_node, prev_node, level) } -> std::convertible_to<bool>;
	};

	template<typename Rule>
	concept rule_has_prepare = requires(Rule rule, TraverseContext& ctx) { rule.event_prepare(ctx); };

	template<typename Rule>
	concept rule_has_generate_parsers = requires(Rule rule, TraverseContext& ctx) {
		{ rule.event_generate_parsers(ctx) } -> not_same_as<void>;
	};

	template<typename Rule, typename Node = ast::Node>
	concept rule = choice_rule<Rule> || sink_rule<Rule, Node> || process_rule<Rule, Node>;

	template<typename T>
	struct rule_is_required : std::false_type {};
	template<typename T>
	struct rule_is_required<T&> : rule_is_required<T> {};
	template<typename T>
	struct rule_is_required<T const&> : rule_is_required<T> {};
	template<typename T>
	struct rule_is_required<T&&> : rule_is_required<T> {};

	template<typename T>
	concept required_rule = rule_is_required<T>::value;

	template<typename T>
	struct rule_disables_stop : std::false_type {};
	template<typename T>
	struct rule_disables_stop<T&> : rule_disables_stop<T> {};
	template<typename T>
	struct rule_disables_stop<T const&> : rule_disables_stop<T> {};
	template<typename T>
	struct rule_disables_stop<T&&> : rule_disables_stop<T> {};

	template<typename T>
	concept no_stop_rule = rule_disables_stop<T>::value;

	struct empty {};

	template<typename T, typename Key, typename... Args>
	concept type_with_failable_try_emplace = requires(T reference, Key&& key, Args&&... args) {
		typename T::iterator;
		{ reference.try_emplace(OV_FWD(key), OV_FWD(args)...) } -> std::same_as<std::pair<typename T::iterator, bool>>;
	};

	OV_ALWAYS_INLINE std::false_type default_visit_fallback() {
		return {};
	}

	template<typename TraverseData, typename Node, typename F, typename Fallback>
	OV_ALWAYS_INLINE auto visit_list_or_tree(
	    TraverseContext& ctx, TraverseData& data, Node const* node, F&& list_or_tree_func, Fallback&& fallback
	) {
		return dryad::visit_node_all(
		    node,
		    [&, list_or_tree_func = OV_FWD(list_or_tree_func)](ast::FileTree const* ft) mutable {
			    return OV_FWD(list_or_tree_func)(ctx, data, ft);
		    },
		    [&, list_or_tree_func = OV_FWD(list_or_tree_func)](ast::ListValue const* lv) mutable {
			    return OV_FWD(list_or_tree_func)(ctx, data, lv);
		    },
		    [&, fallback = OV_FWD(fallback)](ast::Node const* n) mutable {
			    diagnostic::expected_file_tree_or_list_value(ctx, n);
			    return OV_FWD(fallback)();
		    }
		);
	}

	template<typename TraverseData, typename Node, typename F>
	OV_ALWAYS_INLINE auto visit_list_or_tree(
	    TraverseContext& ctx, TraverseData& data, Node const* node, F&& list_or_tree_func
	) {
		return visit_list_or_tree(ctx, data, node, OV_FWD(list_or_tree_func), default_visit_fallback);
	}

	template<typename TraverseData, typename Node, typename F, typename Fallback>
	OV_ALWAYS_INLINE auto visit_assign(
	    TraverseContext& ctx, TraverseData& data, Node const* node, F&& assign_func, Fallback&& fallback
	) {
		return dryad::visit_node_all(
		    node,
		    [&, assign_func = OV_FWD(assign_func)](ast::AssignStatement const* as) mutable {
			    return OV_FWD(assign_func)(ctx, data, as);
		    },
		    [&, fallback = OV_FWD(fallback)](ast::Node const* n) mutable {
			    diagnostic::expected_assign_statement(ctx, n);
			    return OV_FWD(fallback)();
		    }
		);
	}

	template<typename TraverseData, typename Node, typename F>
	OV_ALWAYS_INLINE auto visit_assign(TraverseContext& ctx, TraverseData& data, Node const* node, F&& assign_func) {
		return visit_assign(ctx, data, node, OV_FWD(assign_func), default_visit_fallback);
	}

	template<typename TraverseData, typename Node, typename F, typename Fallback>
	OV_ALWAYS_INLINE auto visit_flat(
	    TraverseContext& ctx, TraverseData& data, Node const* node, F&& flat_func, Fallback&& fallback
	) {
		return dryad::visit_node_all(
		    node,
		    [&, flat_func = OV_FWD(flat_func)](ast::FlatValue const* v) mutable {
			    return OV_FWD(flat_func)(ctx, data, v);
		    },
		    [&, fallback = OV_FWD(fallback)](ast::Node const* n) mutable {
			    diagnostic::expected_flat_value(ctx, n);
			    return OV_FWD(fallback)();
		    }
		);
	}

	template<typename TraverseData, typename Node, typename F>
	OV_ALWAYS_INLINE auto visit_flat(TraverseContext& ctx, TraverseData& data, Node const* node, F&& flat_func) {
		return visit_flat(ctx, data, node, OV_FWD(flat_func), default_visit_fallback);
	}
}
