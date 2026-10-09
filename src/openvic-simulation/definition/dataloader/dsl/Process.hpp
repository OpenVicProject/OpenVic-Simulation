#pragma once

#include "openvic-simulation/core/Typedefs.hpp"
#include "openvic-simulation/definition/dataloader/dsl/Grammar.hpp"
#include "openvic-simulation/definition/dataloader/dsl/TraverseContext.hpp"

namespace OpenVic::dataloader::dsl {
	template<typename Func>
	struct _processr {
		OV_NO_UNIQUE_ADDRESS Func func;

		template<typename TraverseData, typename Node>
		auto event_process(TraverseContext& ctx, TraverseData& data, Node const* node) {
			return func(ctx, data, node);
		}
	};

	template<invocable_r_convertible<bool, TraverseContext&, fake_data&, ast::Node const*> Func>
	constexpr auto process_event(Func&& func) {
		return _processr { OV_FWD(func) };
	}
}
