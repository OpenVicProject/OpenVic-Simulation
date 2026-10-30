#pragma once

#include <string_view>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticBag.hpp"

namespace OpenVic::dataloader::dsl {
	struct TraverseContext {
		dataloader::DiagnosticBag diagnostic;

		ovdl::v2script::Parser const* parser() const {
			return diagnostic.parser();
		}

		ovdl::symbol<> find_intern(std::string_view view) const {
			if (parser()) {
				return parser()->find_intern(view);
			} else {
				return {};
			}
		}
	};
}
