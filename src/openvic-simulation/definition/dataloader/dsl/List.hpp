#pragma once

#include <bitset>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Diagnostics.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/ListOptions.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"
#include "openvic-simulation/definition/dataloader/dsl/detail/TupleCatUnique.hpp"

namespace OpenVic::dataloader::dsl {
	template<ListOptions Options, typename Func, typename PreFunc, rule... Rules>
	struct _sink_list;

	template<ListOptions Options, typename PreIterateFunc, rule... Rules>
	struct _list {
		static constexpr std::bool_constant<!std::same_as<PreIterateFunc, void>> has_pre_iterate_func {};

		OV_NO_UNIQUE_ADDRESS std::tuple<Rules...> rules;
		OV_NO_UNIQUE_ADDRESS std::conditional_t<has_pre_iterate_func, PreIterateFunc, empty> pre_iterate_func;

		auto event_generate_parsers(TraverseContext& ctx) {
			if constexpr ((rule_has_generate_parsers<Rules> || ...)) {
				return detail::unique_event_generate_parsers(ctx, rules);
			}
		}

		void event_prepare(TraverseContext& ctx)
		requires(rule_has_prepare<Rules> || ...)
		{
			std::apply(
			    [&](auto&&... args) {
				    (
				        [&]<typename Rule>(TraverseContext& ctx, Rule&& arg) {
					        if constexpr (rule_has_prepare<Rule>) {
						        arg.event_prepare(ctx);
					        }
				        }(ctx, OV_FWD(args)),
				        ...);
			    },
			    rules
			);
		}

		template<typename TraverseData, typename Node>
		auto event_match(TraverseContext& ctx, TraverseData& data, Node const* node) {
			if constexpr (node_with_statements<Node>) {
				return std::true_type {};
			} else {
				return visit_list_or_tree(
				    ctx, data, node, [this]<typename N>(TraverseContext& ctx, TraverseData& data, N const* node) {
					    return event_match(ctx, data, node);
				    }
				);
			}
		}

		template<typename TraverseData, typename Node>
		auto event_process(TraverseContext& ctx, TraverseData& data, Node const* node) {
			return iterate_statements(
			    ctx,
			    data,
			    node,
			    []<typename Rule, typename N>(
			        TraverseContext& ctx,
			        TraverseData& data,
			        N const* rule_node,
			        Rule& rule,
			        std::size_t rule_index,
			        bool& success,
			        std::bitset<sizeof...(Rules)>::reference seen
			    ) {
				    if constexpr (choice_rule<Rule, N>) {
					    if (rule.event_match(ctx, data, rule_node)) {
						    seen = true;
						    if constexpr (process_rule<Rule, N>) {
							    success = rule.event_process(ctx, data, rule_node);
						    }
						    if constexpr (!no_stop_rule<Rule>) {
							    return true;
						    }
					    }
					    return false;
				    } else if constexpr (process_rule<Rule, N>) {
					    seen = true;
					    success = rule.event_process(ctx, data, rule_node);
					    return std::bool_constant<!no_stop_rule<Rule>> {};
				    } else {
					    return std::false_type {};
				    }
			    }
			);
		}

		template<typename TraverseData, typename Node, typename Func>
		auto iterate_statements(TraverseContext& ctx, TraverseData& data, Node const* node, Func&& func) {
			if constexpr (node_with_statements<Node>) {
				if constexpr (has_pre_iterate_func) {
					OV_FWD(pre_iterate_func)(ctx, data, node);
				}

				std::bitset<sizeof...(Rules)> seen;
				bool success = true;

				for (ast::Statement const* stmt : node->statements()) {
					std::size_t rule_index = 0;
					bool matched_any = false;

					auto try_one = [&, func = OV_FWD(func)]<typename Rule>(Rule& rule) -> bool {
						bool func_success = success;
						bool stop = func(ctx, data, stmt, rule, rule_index, func_success, seen[rule_index]);
						success = success && func_success;
						matched_any = matched_any || stop;
						++rule_index;
						return stop;
					};

					std::apply(
					    [&](auto&... rs) {
						    (try_one(rs) || ...);
					    },
					    rules
					);

					if constexpr (Options.unknown_log_level != dataloader::DiagnosticLevel::NONE) {
						if (!matched_any) {
							diagnostic::found_unknown_key(ctx, stmt, Options.unknown_log_level);
						}
					}
				}

				if constexpr ((rule_has_expected<Rules, Node> || ...)) {
					memory::vector<std::string_view> expected;
					expected.reserve(sizeof...(Rules));

					std::size_t index = 0;
					auto check_missing = [&]<typename Rule>(Rule& rule) {
						if constexpr (rule_has_expected<Rule, Node>) {
							if (std::string_view expect = rule.event_expected(ctx, data, node, seen[index]); !expect.empty()) {
								success = false;
								expected.push_back(OV_MOV(expect));
							}
						}
						++index;
					};
					std::apply(
					    [&](auto&... rs) {
						    (check_missing(rs), ...);
					    },
					    rules
					);

					if (!expected.empty()) {
						diagnostic::expected_keys(ctx, node, expected);
					}
				}

				return success;
			} else {
				return visit_list_or_tree(
				    ctx, data, node, [this, func = OV_FWD(func)](TraverseContext& ctx, TraverseData& data, auto* node) {
					    return iterate_statements(ctx, data, node, OV_FWD(func));
				    }
				);
			}
		}

		template<invocable<TraverseContext&, fake_data&, ast::Node const*> PreFunc>
		constexpr auto with_pre_iterate(PreFunc&& pre_func) {
			return _list<Options, PreFunc, Rules...> { .rules = OV_MOV(rules), .pre_iterate_func = OV_MOV(pre_func) };
		}

		template<typename Func, invocable<TraverseContext&, fake_data&, ast::Node const*> PreFunc>
		constexpr auto sink(Func&& func, PreFunc&& pre_func) {
			return with_pre_iterate(OV_FWD(pre_func)).sink(OV_FWD(func));
		}

		template<typename Func>
		constexpr auto sink(Func&& func) {
			static_assert(
			    requires {
				    _sink_list<Options, Func, PreIterateFunc, Rules...> {
					    .list = { .rules = OV_MOV(rules), .pre_iterate_func = OV_MOV(pre_iterate_func) },
					    .func = { OV_FWD(func) }
				    };
			    }, "Requires #include \"openvic-simulation/definition/dataloader/dsl/ListSink.hpp\""
			);

			return _sink_list<Options, Func, PreIterateFunc, Rules...> {
				.list = { .rules = OV_MOV(rules), .pre_iterate_func = OV_MOV(pre_iterate_func) }, .func = { OV_FWD(func) }
			};
		}

		template<typename T>
		constexpr auto try_reserve_for(T& reference) {
			if constexpr (requires(std::size_t s) { reference.reserve(s); }) {
				auto reserve_func = [&reference = reference](TraverseContext& ctx, auto& root, ast::Node const* node) mutable {
					statement_range_type range = visit_list_or_tree(
					    ctx,
					    root,
					    node->parent()->parent(),
					    []<typename N>(TraverseContext const& ctx, auto, N const* node) {
						    return node->statements();
					    },
					    [] {
						    return dryad::make_node_range<ast::Statement>(
						        statement_range_iterator::from_ptr(nullptr), statement_range_iterator::from_ptr(nullptr)
						    );
					    }
					);

					std::size_t index = 0;
					for (auto _ : range) {
						++index;
					}
					reference.reserve(index);
				};

				return with_pre_iterate(OV_MOV(reserve_func));
			} else {
				return *this;
			}
		}
	};

	template<ListOptions Options, rule... Rules>
	constexpr auto list_with(Rules&&... rules) {
		return _list<Options, void, Rules...> { .rules = { OV_FWD(rules)... } };
	}

	template<typename... Rules>
	requires(sizeof...(Rules) == 0 || ((rule<Rules> || list_option_tag<Rules>) || ...))
	constexpr auto list(Rules&&... rules) {
		return [... rules = OV_FWD(rules)]<std::size_t... I>(std::index_sequence<I...>) mutable {
			static constexpr auto opts = detail::get_options<std::decay_t<Rules>...>();
			auto all = std::forward_as_tuple(std::forward<Rules>(rules)...);
			return list_with<opts>(std::forward<std::tuple_element_t<I, decltype(all)>>(std::get<I>(all))...);
		}(detail::pure_index_sequence<std::decay_t<Rules>...>());
	}
}
