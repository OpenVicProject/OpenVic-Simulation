#include "CodeBuilder.hpp"

#include "openvic-simulation/vm/AsmBuilder.hpp"

#include <lauf/asm/builder.h>

using namespace OpenVic::Vm;

CodeBuilder::CodeBuilder(AsmBuilderRef builder) : HandlerRef(builder), _module(nullptr), _body({ .chunk { false, nullptr } }) {}

CodeBuilder::CodeBuilder(AsmBuilderRef builder, ModuleRef module, Function function)
	: HandlerRef(builder), _module(module), _body({ .function = { true, function } }) {
	lauf_asm_build(builder, _module, _body.function.value);
}

CodeBuilder::CodeBuilder(AsmBuilderRef builder, ModuleRef module, Chunk chunk, lauf_asm_signature signature)
	: HandlerRef(builder), _module(module), _body({ .chunk = { false, chunk } }) {
	lauf_asm_build_chunk(builder, _module, _body.chunk.value, signature);
}

AsmBuilderRef CodeBuilder::asm_builder() {
	return _handle;
}
