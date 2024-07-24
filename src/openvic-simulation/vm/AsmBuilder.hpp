#pragma once

#include "Utility.hpp"
#include <lauf/asm/builder.h>
#include <lauf/asm/module.h>

namespace OpenVic::Vm {
	struct AsmBuilder : utility::MoveOnlyHandleBase<AsmBuilder, lauf_asm_builder> {
		using MoveOnlyHandleBase::MoveOnlyHandleBase;
		using MoveOnlyHandleBase::operator=;

		AsmBuilder(lauf_asm_build_options options) : MoveOnlyHandleBase(lauf_asm_create_builder(options)) {}

		~AsmBuilder() {
			if (_handle == nullptr) {
				return;
			}
			lauf_asm_destroy_builder(*this);
			_handle = nullptr;
		}
	};
}
