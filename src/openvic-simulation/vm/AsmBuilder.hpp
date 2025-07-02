#pragma once

#include "openvic-simulation/vm/Chunk.hpp"
#include "openvic-simulation/vm/CodeBuilder.hpp"
#include "openvic-simulation/vm/Function.hpp"
#include "openvic-simulation/vm/Handler.hpp"
#include "openvic-simulation/vm/Module.hpp"

#include <lauf/asm/builder.h>
#include <lauf/asm/module.h>

namespace OpenVic::Vm {
	struct AsmBuilderRef : HandlerRef<lauf_asm_builder> {
		using HandlerRef::HandlerRef;

		CodeBuilder& build(ModuleRef module, Function function) {
			_current_builder = { *this, module, function };
			return _current_builder;
		}

		CodeBuilder& build(ModuleRef module, Chunk chunk, lauf_asm_signature signature) {
			_current_builder = { *this, module, chunk, signature };
			return _current_builder;
		}

		CodeBuilder& current_builder() {
			return _current_builder;
		}

	private:
		CodeBuilder _current_builder { *this };
	};

	struct AsmBuilder : UniqueHandlerDerived<AsmBuilder, AsmBuilderRef> {
		using UniqueHandlerDerived::UniqueHandlerDerived;
		using UniqueHandlerDerived::operator=;

		AsmBuilder(lauf_asm_build_options options = lauf_asm_default_build_options)
			: UniqueHandlerDerived(lauf_asm_create_builder(options)) {}

		~AsmBuilder() {
			if (_handle == nullptr) {
				return;
			}
			lauf_asm_destroy_builder(*this);
			_handle = nullptr;
		}
	};
}
