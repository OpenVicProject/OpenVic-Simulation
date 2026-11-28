#pragma once

#include <type_traits>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

#include <type_safe/deferred_construction.hpp>
#include <type_safe/output_parameter.hpp>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader {
	template<typename T>
	struct ValueParser;
}

namespace OpenVic::dataloader::dsl {
	template<rule LeftRule, rule RightRule>
	struct _assign;

	template<typename T>
	struct _as;

	template<rule LeftRule, rule RightRule>
	constexpr auto assignment(LeftRule&& left, RightRule&& right);

	// Forcefully optimizes _as stack allocation away
	template<rule LeftRule, typename RightT>
	constexpr auto assignment(LeftRule&& left, _as<RightT>) {
		return _assign<LeftRule, _as<RightT>> { OV_FWD(left), {} };
	}

	template<typename T>
	struct _as {
		static constexpr std::bool_constant<requires { sizeof(ValueParser<type_safe::output_parameter<T>>) != 0; }>
		    stores_value_parser {};

		auto event_generate_parsers(TraverseContext& ctx)
		requires(stores_value_parser())
		{
			auto parser = ValueParser<type_safe::output_parameter<T>> {};
			if constexpr (requires { parser.make_preparation(ctx); }) {
				parser.make_preparation(ctx);
			}
			return parser;
		}

		template<typename TraverseData, typename Node, typename Sink>
		bool event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink) const
		requires invocable_sink_with<Sink, TraverseData, T>
		{
			if constexpr (node_with_value_symbol<Node>) {
				type_safe::deferred_construction<T> value;
				data.template parser_for<type_safe::output_parameter<T>>().parse(ctx, data, node, type_safe::out(value));
				if (!value.has_value()) {
					return false;
				}

				return sink(ctx, data, node, OV_MOV(value).value());
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
	};

	template<typename T>
	inline constexpr _as<T> as {};
}
