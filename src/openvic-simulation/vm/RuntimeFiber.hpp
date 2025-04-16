#pragma once

#include <cassert>
#include <optional>
#include <span>

#include "openvic-simulation/vm/Handler.hpp"
#include "openvic-simulation/vm/Stacktrace.hpp"

#include <lauf/runtime/process.h>
#include <lauf/runtime/value.h>

namespace OpenVic::Vm {
	struct RuntimeProcess;

	struct RuntimeFiber;

	struct RuntimeFiberRef : HandlerRef<lauf_runtime_fiber> {
		using HandlerRef::HandlerRef;

		lauf_runtime_address lauf_handle() const {
			return lauf_runtime_get_fiber_handle(_handle);
		}

		lauf_runtime_fiber_status status() const {
			return lauf_runtime_get_fiber_status(_handle);
		}

		const lauf_runtime_value* get_vstack_base() const;

		RuntimeFiberRef iterate_next() {
			return lauf_runtime_iterate_fibers_next(_handle);
		}
	};

	struct RuntimeFiber : UniqueHandlerDerived<RuntimeFiber, RuntimeFiberRef> {
		using UniqueHandlerDerived::UniqueHandlerDerived;
		using UniqueHandlerDerived::operator=;

		~RuntimeFiber();

		std::optional<Stacktrace> get_stacktrace() const;

		std::optional<RuntimeFiberRef> get_parent();

		const lauf_runtime_value* get_vstack_ptr() const;

		bool resume(lauf_runtime_value input, lauf_runtime_value& output);
		bool resume(lauf_runtime_value input, std::span<lauf_runtime_value> output);
		bool resume(const std::span<lauf_runtime_value> input, lauf_runtime_value& output);
		bool resume(const std::span<lauf_runtime_value> input, std::span<lauf_runtime_value> output);

		bool resume_until_completion(lauf_runtime_value input, lauf_runtime_value& output);
		bool resume_until_completion(lauf_runtime_value input, std::span<lauf_runtime_value> output);
		bool resume_until_completion(const std::span<lauf_runtime_value> input, lauf_runtime_value& output);
		bool resume_until_completion(const std::span<lauf_runtime_value> input, std::span<lauf_runtime_value> output);

	private:
		friend struct RuntimeProcess;
		RuntimeFiber(RuntimeProcess* process, lauf_runtime_fiber* fiber) : UniqueHandlerDerived(fiber), _process(process) {}

		RuntimeProcess* _process;
	};
}
