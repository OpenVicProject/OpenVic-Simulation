#pragma once

#include <cstdint>

namespace OpenVic {
	enum struct IdentifierSyntax : uint8_t {
		Either,
		Identifier, // ENG
		String // "ENG"
	};
	struct IdentifierValidation {
		bool allow_empty = false;
		bool warn_on_invalid = false;
	};
}