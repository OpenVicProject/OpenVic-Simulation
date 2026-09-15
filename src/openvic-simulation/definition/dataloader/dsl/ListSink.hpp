#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <tuple>
#include <type_traits>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include <type_safe/deferred_construction.hpp>
#include <type_safe/output_parameter.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticLevel.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/List.hpp"
#include "openvic-simulation/definition/dataloader/dsl/ListOptions.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader::dsl {
	template<ListOptions Options, typename Func, typename PreFunc, rule... Rules>
	struct _sink_list {
		OV_NO_UNIQUE_ADDRESS _list<Options, PreFunc, Rules...> list;
		OV_NO_UNIQUE_ADDRESS Func func;

		auto event_generate_parsers(TraverseContext& ctx) {
			list.event_generate_parsers(ctx);
		}

		void event_prepare(TraverseContext& ctx) {
			list.event_prepare(ctx);
		}

		template<std::size_t M = 0, typename TraverseData, typename Node, typename Acc>
		[[nodiscard]] auto call_func(
		    TraverseContext& ctx,
		    TraverseData& data,
		    Node const* node,
		    std::array<ast::Node const*, sizeof...(Rules)> const& nodes,
		    Acc&& acc
		) {
			return call_func<M>(ctx, data, node, nodes, OV_FWD(acc), OV_FWD(func));
		}

		template<std::size_t M = 0, typename TraverseData, typename Node, typename Acc, typename CallFunc>
		[[nodiscard]] auto call_func(
		    TraverseContext& ctx,
		    TraverseData& data,
		    Node const* node,
		    std::array<ast::Node const*, sizeof...(Rules)> const& nodes,
		    Acc&& acc,
		    CallFunc&& func_to_call
		) {
			static constexpr std::size_t N = sizeof...(Rules);
			if constexpr (M == N) {
				return std::apply(
				    [&, func_to_call = OV_FWD(func_to_call)](auto&&... args)
				    requires invocable_sink_with<CallFunc, TraverseData, decltype(args)...>
				    {
					    return OV_FWD(func_to_call)(ctx, data, node, OV_FWD(args)...);
				    },
				    OV_FWD(acc)
				);
			} else {
				using Rule = std::tuple_element_t<M, std::tuple<Rules...>>;
				Rule& rule = std::get<M>(this->list.rules);

				auto sink = [&, acc = OV_FWD(acc), func_to_call = OV_FWD(func_to_call)]<typename N>(
				                TraverseContext& ctx, TraverseData& data, N const*, auto&&... value
				            ) {
					return call_func<M + 1>(
					    ctx,
					    data,
					    node,
					    nodes,
					    std::tuple_cat(OV_FWD(acc), std::make_tuple(OV_FWD(value)...)),
					    OV_FWD(func_to_call)
					);
				};

				if constexpr (required_rule<Rule>) {
					// required keys are guaranteed present when we reach here
					return rule.event_sink(ctx, data, nodes[M], OV_MOV(sink));
				} else {
					// optional: include only when matched
					if (nodes[M]) {
						return rule.event_sink(ctx, data, nodes[M], OV_MOV(sink));
					} else {
						return static_cast<bool>(call_func<M + 1>(ctx, data, node, nodes, OV_FWD(acc), OV_FWD(func_to_call)));
					}
				}
			}
		}

		template<typename TraverseData, typename Node>
		auto event_process(TraverseContext& ctx, TraverseData& data, Node const* node) {
			return event_sink(ctx, data, node, OV_FWD(func));
		}

		template<typename TraverseData, typename Node, typename Sink>
		auto event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink)
		requires(sizeof...(Rules) == 0 || (sink_rule<Rules> || ...))
		{
			if constexpr (sizeof...(Rules) != 0) {
				std::array<ast::Node const*, sizeof...(Rules)> rule_associated_nodes {};

				auto result = list.iterate_statements(
				    ctx,
				    data,
				    node,
				    [&]<typename Rule, typename N>(
				        TraverseContext& ctx,
				        TraverseData& data,
				        N const* rule_node,
				        Rule& rule,
				        std::size_t rule_index,
				        bool& success,
				        std::bitset<sizeof...(Rules)>::reference seen
				    ) {
					    if constexpr (choice_rule<Rule, N> && sink_rule<Rule, N>) {
						    if (rule.event_match(ctx, data, rule_node)) {
							    seen = true;
							    if constexpr (
							        Options.duplicate_log_level != DiagnosticLevel::NONE && rule_has_duplicate<Rule, N>
							    ) {
								    if (rule_associated_nodes[rule_index] != nullptr) {
									    rule.event_duplicate(
									        ctx, data, rule_node, rule_associated_nodes[rule_index], Options.duplicate_log_level
									    );
								    }
							    }
							    if (rule_associated_nodes[rule_index] == nullptr) {
								    rule_associated_nodes[rule_index] = rule_node;
							    }
							    success = true;
						    } else {
							    return false;
						    }
						    return !no_stop_rule<Rule>;
					    } else if constexpr (sink_rule<Rule, N>) {
						    seen = true;
						    if constexpr (Options.duplicate_log_level != DiagnosticLevel::NONE && rule_has_duplicate<Rule, N>) {
							    if (rule_associated_nodes[rule_index] != nullptr) {
								    rule.event_duplicate(
								        ctx, data, rule_node, rule_associated_nodes[rule_index], Options.duplicate_log_level
								    );
							    }
						    }
						    if (rule_associated_nodes[rule_index] == nullptr) {
							    rule_associated_nodes[rule_index] = rule_node;
						    }
						    success = true;
						    return std::bool_constant<!no_stop_rule<Rule>> {};
					    } else {
						    return std::false_type {};
					    }
				    }
				);

				return result && call_func(ctx, data, node->parent(), rule_associated_nodes, std::tuple {}, OV_FWD(sink));
			} else {
				if constexpr (list.has_pre_iterate_func) {
					if constexpr (node_with_statements<Node>) {
						OV_FWD(list.has_pre_iterate_func)(ctx, data, node);
					} else {
						dsl::visit_list_or_tree(ctx, data, node, [this](TraverseContext& ctx, TraverseData& data, auto* node) {
							OV_FWD(list.has_pre_iterate_func)(ctx, data, node);
						});
					}
				}
				return OV_FWD(sink)(ctx, data, node);
			}
		}
	};
}
