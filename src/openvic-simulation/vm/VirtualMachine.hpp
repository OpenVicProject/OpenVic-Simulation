#pragma once

#include <vector>

#include "InstanceManager.hpp"
#include "RuntimeProcess.hpp"
#include "Utility.hpp"
#include "vm/Builtin.hpp"
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

	struct OpenVicVirtualMachine : VirtualMachine {
		using VirtualMachine::VirtualMachine;
		using VirtualMachine::operator=;

		OpenVicVirtualMachine(
			InstanceManager* instance_manager, std::vector<Asm::scope_variant>&& scope_references,
			lauf_vm_options options = lauf_default_vm_options
		)
			: VirtualMachine([&] {
				  options.user_data = &user_data;
				  return options;
			  }()) {
			user_data.scope_references = std::move(scope_references);
			user_data.instance_manager = instance_manager;
		}

	private:
		VmUserData user_data;
	};
}
