#pragma once

#include "vm/RuntimeFiber.hpp"
#include "vm/Stacktrace.hpp"
#include "vm/Utility.hpp"
#include <lauf/runtime/memory.h>
#include <lauf/runtime/process.h>
#include <lauf/runtime/stacktrace.h>

namespace OpenVic::Vm {
	struct VirtualMachine;

	struct RuntimeProcess : utility::MoveOnlyHandleBase<RuntimeProcess, lauf_runtime_process> {
		using MoveOnlyHandleBase::MoveOnlyHandleBase;
		using MoveOnlyHandleBase::operator=;

		~RuntimeProcess() {
			lauf_runtime_destroy_process(*this);
			_handle = nullptr;
		}

		RuntimeFiber create_fiber(const lauf_asm_function* fn) {
			return RuntimeFiber(this, lauf_runtime_create_fiber(*this, fn));
		}

		RuntimeFiberRef get_current_fiber() {
			return lauf_runtime_get_current_fiber(*this);
		}

		RuntimeFiberRef get_fiber_ptr(lauf_runtime_address addr) {
			return lauf_runtime_get_fiber_ptr(*this, addr);
		}

	private:
		friend struct VirtualMachine;
		RuntimeProcess(lauf_runtime_process* process) : MoveOnlyHandleBase(process) {}
	};
}
