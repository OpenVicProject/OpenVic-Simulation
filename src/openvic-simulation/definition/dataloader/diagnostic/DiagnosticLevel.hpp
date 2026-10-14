#pragma once

#include <cstdint>

namespace OpenVic::dataloader {
	enum class DiagnosticLevel : std::uint8_t {
		NONE, // Filters out all diagnostic reporting
		FATAL, // Critical failure that ends processing
		ERROR,
		WARN,
		INFO, // Helpful context, probably attached to a ERROR or WARN item
		HINT, // Non-style hint
		SUGGEST, // Style suggestion
	};
}
