#pragma once

#include "RuntimeProcess.hpp"
#include <lauf/vm.h>

namespace OpenVic::Vm {
	struct VirtualMachine {
		VirtualMachine(lauf_vm_options options) : _handle(lauf_create_vm(options)) {}

		VirtualMachine(VirtualMachine&&) = default;
		VirtualMachine& operator=(VirtualMachine&&) = default;

		VirtualMachine(VirtualMachine const&) = delete;
		VirtualMachine& operator=(VirtualMachine const&) = delete;

		~VirtualMachine() {
			lauf_destroy_vm(_handle);
		}

		lauf_vm* handle() {
			return _handle;
		}

		const lauf_vm* handle() const {
			return _handle;
		}

		operator lauf_vm*() {
			return _handle;
		}

		operator const lauf_vm*() const {
			return _handle;
		}

		RuntimeProcess start_process(const lauf_asm_program* program) {
			return RuntimeProcess(lauf_vm_start_process(_handle, program));
		}

	private:
		lauf_vm* _handle;
	};
}
