#pragma once

#include <string_view>

#include "openvic-simulation/vm/Handler.hpp"

#include <lauf/asm/module.h>
#include <lauf/asm/type.h>

namespace OpenVic::Vm {
	struct Global : HandlerRef<lauf_asm_global> {
		using HandlerRef::HandlerRef;

		std::string_view debug_name() const {
			const char* name = lauf_asm_global_debug_name(*this);
			return name == nullptr ? std::string_view {} : name;
		}

		lauf_asm_layout layout() const {
			return lauf_asm_global_layout(*this);
		}

		bool has_definition() const {
			return lauf_asm_global_has_definition(*this);
		}
	};
}
