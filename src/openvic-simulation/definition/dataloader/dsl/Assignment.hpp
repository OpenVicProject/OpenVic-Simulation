#pragma once

#include <string_view>
#include <tuple>
#include <type_traits>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader::dsl {
	template<rule LeftRule, rule RightRule>
	struct _assign;

	template<typename T>
	struct _as;

	template<rule LeftRule, rule RightRule>
	constexpr auto assignment(LeftRule&& left, RightRule&& right) {
		return _assign<LeftRule, RightRule> { OV_FWD(left), OV_FWD(right) };
	}

	template<rule LeftRule, rule RightRule>
	struct _assign {
		OV_NO_UNIQUE_ADDRESS LeftRule left;
		OV_NO_UNIQUE_ADDRESS RightRule right;

		auto event_generate_parsers(TraverseContext& ctx)
		requires rule_has_generate_parsers<LeftRule> || rule_has_generate_parsers<RightRule>
		{
			if constexpr (rule_has_generate_parsers<LeftRule> && rule_has_generate_parsers<RightRule>) {
				using left_return_type = decltype(left.event_generate_parsers(ctx));
				using right_return_type = decltype(right.event_generate_parsers(ctx));

				if constexpr (!std::is_void_v<left_return_type> && !std::is_void_v<right_return_type>) {
					return std::make_tuple(left.event_generate_parsers(ctx), right.event_generate_parsers(ctx));
				} else if constexpr (std::is_void_v<right_return_type>) {
					right.event_generate_parsers(ctx);
					return left.event_generate_parsers(ctx);
				} else {
					left.event_generate_parsers(ctx);
					return right.event_generate_parsers(ctx);
				}
			} else if constexpr (rule_has_generate_parsers<LeftRule>) {
				return left.event_generate_parsers(ctx);
			} else if constexpr (rule_has_generate_parsers<RightRule>) {
				return right.event_generate_parsers(ctx);
			}
		}

		void event_prepare(TraverseContext& ctx)
		requires rule_has_prepare<LeftRule> || rule_has_prepare<RightRule>
		{
			if constexpr (rule_has_prepare<LeftRule>) {
				left.event_prepare(ctx);
			}

			if constexpr (rule_has_prepare<RightRule>) {
				right.event_prepare(ctx);
			}
		}

		template<typename TraverseData, typename Node>
		auto event_match(TraverseContext& ctx, TraverseData& data, Node const* node) {
			if constexpr (node_with_left<Node> && choice_rule<LeftRule, Node>) {
				if (!left.event_match(ctx, data, node->left())) {
					return false;
				}
			}

			if constexpr (node_with_right<Node> && choice_rule<RightRule, Node>) {
				if (!right.event_match(ctx, data, node->right())) {
					return false;
				}
			}

			if constexpr (node_with_left<Node> || node_with_right<Node>) {
				return true;
			} else {
				return visit_assign(
				    ctx, data, node, [this](TraverseContext& ctx, TraverseData& data, ast::AssignStatement const* as) {
					    return event_match(ctx, data, as);
				    }
				);
			}
		}

		template<typename TraverseData, typename Node>
		auto event_process(TraverseContext& ctx, TraverseData& data, Node const* node) {
			if constexpr (node_with_left<Node> && process_rule<LeftRule, Node>) {
				if (!left.event_process(ctx, data, node->left())) {
					return false;
				}
			}

			if constexpr (node_with_right<Node> && process_rule<RightRule, Node>) {
				if (!right.event_process(ctx, data, node->right())) {
					return false;
				}
			}

			if constexpr (node_with_left<Node> || node_with_right<Node>) {
				return true;
			} else {
				return visit_assign(
				    ctx, data, node, [this](TraverseContext& ctx, TraverseData& data, ast::AssignStatement const* as) {
					    return event_process(ctx, data, as);
				    }
				);
			}
		}

		template<typename TraverseData, typename Node>
		std::string_view event_expected(TraverseContext& ctx, TraverseData& data, Node const* parent, bool seen)
		requires rule_has_expected<LeftRule, Node> || rule_has_expected<RightRule, Node>
		{
			if constexpr (rule_has_expected<LeftRule, Node>) {
				if (std::string_view lstr = left.event_expected(ctx, data, parent, seen); !lstr.empty()) {
					return lstr;
				}
			}

			if constexpr (rule_has_expected<RightRule, Node>) {
				if (std::string_view rstr = right.event_expected(ctx, data, parent, seen); !rstr.empty()) {
					return rstr;
				}
			}

			return {};
		}

		template<typename TraverseData, typename CurNode>
		bool event_duplicate(
		    TraverseContext& ctx, TraverseData& data, CurNode const* cur_node, ast::Node const* prev_node, DiagnosticLevel level
		)
		requires rule_has_duplicate<LeftRule, CurNode> || rule_has_duplicate<RightRule, CurNode>
		{
			if constexpr (rule_has_duplicate<LeftRule, CurNode>) {
				if (!left.event_duplicate(ctx, data, cur_node, prev_node, level)) {
					return false;
				}
			}

			if constexpr (rule_has_duplicate<RightRule, CurNode>) {
				if (!right.event_duplicate(ctx, data, cur_node, prev_node, level)) {
					return false;
				}
			}

			return true;
		}

		template<typename Node, typename Sink>
		struct LeftSink {
			Node const* node;
			Sink sink;
			RightRule& right;

			template<typename TraverseData, typename N>
			OV_ALWAYS_INLINE auto operator()(TraverseContext& ctx, TraverseData& data, N const*, auto&&... lhs) {
				return right.event_sink(
				    ctx,
				    data,
				    node->right(),
				    [... lhs = OV_FWD(lhs),
				     sink = OV_FWD(sink)]<typename N2>(TraverseContext& ctx, TraverseData& data, N2 const* node, auto&&... rhs)
				    requires invocable_sink_with<decltype(sink), TraverseData, decltype(lhs)..., decltype(rhs)...>
				    {
					    return OV_FWD(sink)(ctx, data, node, OV_FWD(lhs)..., OV_FWD(rhs)...);
				    }
				    );
			}
		};

		template<typename TraverseData, typename Node, typename Sink>
		auto event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink)
		requires(sink_rule<LeftRule, ast::Value> || sink_rule<RightRule, ast::Value>)
		{
			/*if constexpr (
			    node_with_left<Node> && node_with_right<Node> && sink_rule<LeftRule, Node> && sink_rule<RightRule, Node>
			) {
			    return left.event_sink(ctx, node->left(), LeftSink { node, OV_FWD(sink), right });
			} else*/
			if constexpr (node_with_left<Node> && sink_rule<LeftRule, ast::Value>) {
				return left.event_sink(ctx, data, node->left(), OV_FWD(sink));
			} else if constexpr (node_with_right<Node> && sink_rule<RightRule, ast::Value>) {
				return right.event_sink(ctx, data, node->right(), OV_FWD(sink));
			} else {
				return visit_assign(
				    ctx,
				    data,
				    node,
				    [this, sink = OV_FWD(sink)](TraverseContext& ctx, TraverseData& data, ast::AssignStatement const* as) {
					    return event_sink(ctx, data, as, OV_FWD(sink));
				    }
				);
			}
		}
	};

	template<rule LeftRule, rule RightRule>
	struct rule_is_required<_assign<LeftRule, RightRule>> : rule_is_required<LeftRule> {};
}
