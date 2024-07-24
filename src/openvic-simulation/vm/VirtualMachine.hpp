#pragma once

#include "RuntimeProcess.hpp"
#include "Utility.hpp"
#include <lauf/vm.h>

namespace OpenVic::Vm {
	struct VirtualMachine : utility::MoveOnlyHandleBase<VirtualMachine, lauf_vm> {
		using MoveOnlyHandleBase::MoveOnlyHandleBase;
		using MoveOnlyHandleBase::operator=;

		VirtualMachine(lauf_vm_options options) : MoveOnlyHandleBase(lauf_create_vm(options)) {}

		~VirtualMachine() {
			if (_handle == nullptr) {
				return;
			}
			lauf_destroy_vm(*this);
			_handle = nullptr;
		}

		RuntimeProcess start_process(const lauf_asm_program* program) {
			return RuntimeProcess(lauf_vm_start_process(*this, program));
		}
	};
}
