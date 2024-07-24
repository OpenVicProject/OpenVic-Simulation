#pragma once

#include "vm/RuntimeFiber.hpp"
#include "vm/Stacktrace.hpp"
#include <lauf/runtime/memory.h>
#include <lauf/runtime/process.h>
#include <lauf/runtime/stacktrace.h>

namespace OpenVic::Vm {
	struct VirtualMachine;

	struct RuntimeProcess {
		RuntimeProcess(RuntimeProcess&&) = default;
		RuntimeProcess& operator=(RuntimeProcess&&) = default;

		RuntimeProcess(RuntimeProcess const&) = delete;
		RuntimeProcess& operator=(RuntimeProcess const&) = delete;

		~RuntimeProcess() {
			lauf_runtime_destroy_process(_handle);
		}

		lauf_runtime_process* handle() {
			return _handle;
		}

		const lauf_runtime_process* handle() const {
			return _handle;
		}

		operator lauf_runtime_process*() {
			return _handle;
		}

		operator const lauf_runtime_process*() const {
			return _handle;
		}

		RuntimeFiber create_fiber(const lauf_asm_function* fn) {
			return RuntimeFiber(this, lauf_runtime_create_fiber(*this, fn));
		}

		RuntimeFiberRef get_current_fiber() {
			return lauf_runtime_get_current_fiber(_handle);
		}

		RuntimeFiberRef get_fiber_ptr(lauf_runtime_address addr) {
			return lauf_runtime_get_fiber_ptr(_handle, addr);
		}

	private:
		friend struct VirtualMachine;
		RuntimeProcess(lauf_runtime_process* process) : _handle(process) {}

		lauf_runtime_process* _handle;
	};
}
