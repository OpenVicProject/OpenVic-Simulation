#include "Module.hpp"

#include "openvic-simulation/vm/Program.hpp"

#include <lauf/asm/program.h>

using namespace OpenVic::Vm;

Program ModuleRef::create_program(const Function entry) const {
	return lauf_asm_create_program(*this, entry);
}

Program ModuleRef::create_program(const Chunk chunk) const {
	return lauf_asm_create_program_from_chunk(*this, chunk);
}
