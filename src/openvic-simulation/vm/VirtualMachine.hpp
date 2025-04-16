#pragma once

#include "openvic-simulation/vm/Handler.hpp"
#include "openvic-simulation/vm/Program.hpp"
#include "openvic-simulation/vm/RuntimeProcess.hpp"

#include <lauf/vm.h>

namespace OpenVic::Vm {
	struct VirtualMachineRef : HandlerRef<lauf_vm> {
		using HandlerRef::HandlerRef;

		RuntimeProcess start_process(const ProgramRef program) {
			return RuntimeProcess(lauf_vm_start_process(*this, program));
		}

		bool execute( //
			const ProgramRef program, const std::span<lauf_runtime_value> input, lauf_runtime_value* output
		) {
			assert(program.entry().signature().input_count == input.size());
			return lauf_vm_execute(*this, program, input.data(), output);
		}

		bool execute(const ProgramRef program, lauf_runtime_value input, lauf_runtime_value* output) {
			assert(program.entry().signature().input_count == 1);
			return lauf_vm_execute(*this, program, &input, output);
		}

		bool execute_oneshot( //
			const ProgramRef program, const std::span<lauf_runtime_value> input, lauf_runtime_value* output
		) && {
			assert(program.entry().signature().input_count == input.size());
			bool result = lauf_vm_execute_oneshot(*this, program, input.data(), output);
			_handle = nullptr;
			return result;
		}

		bool execute_oneshot(const ProgramRef program, lauf_runtime_value input, lauf_runtime_value* output) && {
			assert(program.entry().signature().input_count == 1);
			bool result = lauf_vm_execute_oneshot(*this, program, &input, output);
			_handle = nullptr;
			return result;
		}

		lauf_vm_allocator allocator() {
			return lauf_vm_get_allocator(*this);
		}

		lauf_vm_allocator allocator(lauf_vm_allocator allocator) {
			return lauf_vm_set_allocator(*this, allocator);
		}

		void* user_data() {
			return lauf_vm_get_user_data(*this);
		}

		void* user_data(void* data) {
			return lauf_vm_set_user_data(*this, data);
		}

		lauf_vm_panic_handler set_panic_handler(lauf_vm_panic_handler handler) {
			return lauf_vm_set_panic_handler(*this, handler);
		}
	};

	struct VirtualMachine : UniqueHandlerDerived<VirtualMachine, VirtualMachineRef> {
		using UniqueHandlerDerived::operator=;

		VirtualMachine(lauf_vm_options options = lauf_default_vm_options) : UniqueHandlerDerived(lauf_create_vm(options)) {}

		~VirtualMachine() {
			if (_handle == nullptr) {
				return;
			}
			lauf_destroy_vm(*this);
			_handle = nullptr;
		}

	protected:
		using UniqueHandlerDerived::UniqueHandlerDerived;
	};
}
