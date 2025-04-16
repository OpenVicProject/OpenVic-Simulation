#pragma once

#include "openvic-simulation/vm/Handler.hpp"

#include <lauf/asm/builder.h>
#include <lauf/asm/type.h>

namespace OpenVic::Vm {
	struct Local : HandlerRef<lauf_asm_local> {
		using HandlerRef::HandlerRef;

		lauf_asm_layout layout() {
			return lauf_asm_local_layout(nullptr, *this);
		}
	};
}
