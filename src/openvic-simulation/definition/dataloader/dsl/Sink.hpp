#pragma once

#include <concepts>

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader::dsl {
	template<typename Func>
	struct _sinkr {
		OV_NO_UNIQUE_ADDRESS Func func;

		template<typename TraverseData, typename Node, typename Sink>
		auto event_sink(TraverseContext& ctx, TraverseData& data, Node const* node, Sink&& sink) {
			return func(ctx, data, node, OV_FWD(sink));
		}
	};

	template<typename Func, typename Node = ast::Node>
	concept event_sink_function = requires(Func func, fake_data& data, TraverseContext& c, Node const* node) {
		{
			func(c, data, node, [](TraverseContext&, fake_data&, ast::Node const*, auto&&...) {
				return true;
			})
		} -> std::convertible_to<bool>;
	};

	template<event_sink_function Func>
	constexpr auto sink_event(Func&& func) {
		return _sinkr { OV_FWD(func) };
	}
}
