#include "AsmBuilder.hpp"

#include <lauf/asm/builder.h>

using namespace OpenVic::Vm;

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

void CodeBuilder::CodeBuilder::set_for(Function function) {
	set_for(_module, function);
}

void CodeBuilder::set_for(ModuleRef module, Function function) {
	_module = module;
	_body.function.is_function = true;
	_body.function.value = function;
	lauf_asm_build(*this, _module, _body.function.value);
}

void CodeBuilder::set_for(Chunk chunk) {
	set_for(_module, chunk);
}

void CodeBuilder::set_for(ModuleRef module, Chunk chunk) {
	set_for(module, chunk, chunk.signature());
}

void CodeBuilder::set_for(Chunk chunk, lauf_asm_signature signature) {
	set_for(_module, chunk, signature);
}

void CodeBuilder::set_for(ModuleRef module, Chunk chunk, lauf_asm_signature signature) {
	_module = module;
	_body.chunk.is_function = false;
	_body.chunk.value = chunk;
	lauf_asm_build_chunk(*this, _module, _body.chunk.value, signature);
}
