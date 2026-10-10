#pragma once

#include "openvic-simulation/core/error/Error.hpp"
#include "openvic-simulation/definition/dataloader/diagnostic/DiagnosticBag.hpp"

namespace OpenVic {
	struct TraverseResult {
		dataloader::DiagnosticBag diagnostics;
		Error error = Error::OK;
	};

}
