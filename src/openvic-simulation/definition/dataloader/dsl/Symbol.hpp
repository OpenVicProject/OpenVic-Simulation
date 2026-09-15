#pragma once

#include <concepts>
#include <type_traits>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include <type_safe/deferred_construction.hpp>
#include <type_safe/output_parameter.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Assignment.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/ListOptions.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"
#include "openvic-simulation/definition/dataloader/dsl/ValueParser.hpp"

namespace OpenVic::dataloader::dsl {
	template<rule LeftRule, rule RightRule>
	struct _assign;

	template<typename T>
	struct _symbol;

	template<rule LeftRule, rule RightRule>
	constexpr auto assignment(LeftRule&& left, RightRule&& right);

	// Forcefully optimizes _symbol stack allocation away
	template<rule LeftRule, typename RightT>
	constexpr auto assignment(LeftRule&& left, _symbol<RightT>) {
		return _assign<LeftRule, _symbol<RightT>> { OV_FWD(left), {} };
	}

	template<typename T = ovdl::symbol<>>
	struct _symbol {
		static_assert(requires { sizeof(ValueParser<T>); }, "ValueParser<T> must be defined.");
		static constexpr std::bool_constant<requires { sizeof(ValueParser<T>) != 0; }> stores_value_parser {};

		auto event_generate_parsers(TraverseContext& ctx)
		requires(stores_value_parser())
		{
			auto parser = ValueParser<T> {};
			if constexpr (requires { parser.make_preparation(ctx); }) {
				parser.make_preparation(ctx);
			}
			return parser;
		}

		template<typename TraverseData, typename Node>
		bool event_match(TraverseContext& ctx, TraverseData& data, Node const* node) const {
			if constexpr (node_with_value_symbol<Node>) {
				return std::true_type {};
			} else {
				return visit_flat(ctx, data, node, [this](TraverseContext& ctx, TraverseData& data, ast::FlatValue const* fv) {
					return event_match(ctx, data, fv);
				});
			}
		}

		template<typename TraverseData, typename Node>
		bool event_process(TraverseContext& ctx, TraverseData& data, Node const* node) const {
			if constexpr (node_with_value_symbol<Node>) {
				return std::true_type {};
			} else {
				return visit_flat(ctx, data, node, [this](TraverseContext& ctx, TraverseData& data, ast::FlatValue const* fv) {
					return event_process(ctx, data, fv);
				});
			}
		}

		template<typename TraverseData, typename Node, typename Sink>
		auto event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink) const
		requires invocable_sink_with<Sink, TraverseData, T>
		{
			if constexpr (node_with_value_symbol<Node>) {
				if constexpr (std::same_as<T, ovdl::symbol<>>) {
					return sink(ctx, node, node->value());
				} else {
					type_safe::deferred_construction<T> value;
					if (!data.template parser_for<type_safe::output_parameter<T>>().parse(
					        ctx, data, node, type_safe::out(value)
					    ) ||
					    !value.has_value()) {
						return false;
					}
					return static_cast<bool>(sink(ctx, data, node, OV_MOV(value).value()));
				}
			} else {
				return visit_flat(
				    ctx,
				    data,
				    node,
				    [this, sink = OV_FWD(sink)](TraverseContext& ctx, TraverseData& data, ast::FlatValue const* fv) {
					    return event_sink(ctx, data, fv, OV_FWD(sink));
				    }
				);
			}
		}

		template<rule RightRule>
		constexpr auto operator=(RightRule&& right_rule) const {
			return assignment(_symbol {}, OV_FWD(right_rule));
		}
	};

	inline constexpr _symbol<> symbol {};

	template<typename T>
	inline constexpr _symbol<T> symbol_as {};

	template<ListOptions Options, typename Func, typename PreFunc, rule... Rules>
	struct _sink_list;

	template<typename T, ListOptions Options, typename Func, typename PreFunc, rule... Rules>
	struct _sym_assign_sink_list : _sink_list<Options, Func, PreFunc, Rules...> {
		using base_type = _sink_list<Options, Func, PreFunc, Rules...>;

		template<typename TraverseData, typename Node>
		auto event_match(TraverseContext& ctx, TraverseData& data, Node const* node) {
			if constexpr (node_with_left<Node>) {
				if (!_symbol<T> {}.event_match(ctx, data, node->left())) {
					return false;
				}
			}

			if constexpr (node_with_right<Node> && choice_rule<base_type, Node>) {
				if (!base_type::event_match(ctx, data, node->right())) {
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
			if constexpr (node_with_right<Node>) {
				if (!event_sink(ctx, data, node, OV_MOV(this->func))) {
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

		template<typename TraverseData, typename Node, typename Sink>
		auto event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink) {
			if constexpr (node_with_left<Node>) {
				bool parse_succeed = false;
				auto parse_value = [&] {
					type_safe::deferred_construction<T> value;
					parse_succeed = data.template parser_for<type_safe::output_parameter<T>>().parse(
					    ctx, data, node->left(), type_safe::out(value)
					);
					return value;
				};

				if constexpr (node_with_right<Node> && sink_rule<base_type, Node>) {
					return static_cast<bool>(base_type::event_sink(
					    ctx,
					    data,
					    node->right(),
					    [value = parse_value(), parse_succeed, sink = OV_FWD(sink)]<typename N>(
					        TraverseContext& ctx, TraverseData& data, N const* node, auto&&... input
					    ) mutable
					    requires invocable_sink_with<Sink, TraverseData, T&&, decltype(input)...>
					    {
						    if (!parse_succeed || !value.has_value()) {
							    return false;
						    }
						    return OV_FWD(sink)(ctx, data, node, OV_MOV(value).value(), OV_FWD(input)...);
					    }
					    ));
				} else {
					return false;
				}
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

	template<typename T, ListOptions Options, typename Func, typename PreFunc, rule... Rules>
	constexpr auto assignment(_symbol<T>, _sink_list<Options, Func, PreFunc, Rules...>&& right) {
		static_assert(
		    requires { _sym_assign_sink_list<T, Options, Func, PreFunc, Rules...> { OV_MOV(right) }; },
		    "Requires #include \"openvic-simulation/definition/dataloader/dsl/ListSink.hpp\""
		);

		return _sym_assign_sink_list<T, Options, Func, PreFunc, Rules...> { OV_MOV(right) };
	}
}
