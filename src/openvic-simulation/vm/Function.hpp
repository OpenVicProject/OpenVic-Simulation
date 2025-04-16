#pragma once

#include <cassert>
#include <string_view>

#include "openvic-simulation/vm/Handler.hpp"

#include <lauf/asm/module.h>

namespace OpenVic::Vm {
	struct Function : HandlerRef<lauf_asm_function> {
		using HandlerRef::HandlerRef;

		std::string_view name() const {
			return lauf_asm_function_name(*this);
		}

		lauf_asm_signature signature() const {
			return lauf_asm_function_signature(*this);
		}

		bool has_definition() const {
			return lauf_asm_function_has_definition(*this);
		}

		size_t get_instruction_by_index(lauf_asm_inst const* ip) const {
			return lauf_asm_get_instruction_index(*this, ip);
		}

		void set_export() {
			lauf_asm_export_function(*this);
		}
	};
}
