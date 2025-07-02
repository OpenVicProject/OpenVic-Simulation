#include "RuntimeProcess.hpp"

#include "openvic-simulation/vm/VirtualMachine.hpp"

#include <lauf/runtime/process.h>

using namespace OpenVic::Vm;

VirtualMachineRef RuntimeProcess::get_vm() {
	return lauf_runtime_get_vm(*this);
}
