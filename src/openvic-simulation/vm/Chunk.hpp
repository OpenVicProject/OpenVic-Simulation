#pragma once

#include "openvic-simulation/vm/Handler.hpp"

#include <lauf/asm/module.h>

namespace OpenVic::Vm {
	struct Chunk : HandlerRef<lauf_asm_chunk> {
		using HandlerRef::HandlerRef;

		lauf_asm_signature signature() const {
			return lauf_asm_chunk_signature(*this);
		}

		bool is_empty() const {
			return lauf_asm_chunk_is_empty(*this);
		}
	};
}
