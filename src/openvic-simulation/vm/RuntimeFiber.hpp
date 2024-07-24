#pragma once

#include <cassert>
#include <optional>

#include "vm/Stacktrace.hpp"
#include <lauf/runtime/process.h>
#include <lauf/runtime/value.h>

namespace OpenVic::Vm {
	struct RuntimeProcess;

	struct RuntimeFiber;

	struct RuntimeFiberRef {
		RuntimeFiberRef(lauf_runtime_fiber* fiber) : _handle(fiber) {}

		lauf_runtime_fiber* handle() {
			return _handle;
		}

		const lauf_runtime_fiber* handle() const {
			return _handle;
		}

		operator lauf_runtime_fiber*() {
			return _handle;
		}

		operator const lauf_runtime_fiber*() const {
			return _handle;
		}

		lauf_runtime_address lauf_handle() const {
			assert(is_valid());
			return lauf_runtime_get_fiber_handle(_handle);
		}

		lauf_runtime_fiber_status status() const {
			assert(is_valid());
			return lauf_runtime_get_fiber_status(_handle);
		}

		const lauf_runtime_value* get_vstack_base() const;

		RuntimeFiberRef iterate_next() {
			assert(is_valid());
			return lauf_runtime_iterate_fibers_next(_handle);
		}

		bool is_valid() const {
			return _handle != nullptr;
		}

	protected:
		lauf_runtime_fiber* _handle;
	};

	struct RuntimeFiber : RuntimeFiberRef {
		RuntimeFiber(RuntimeFiber&&) = default;
		RuntimeFiber& operator=(RuntimeFiber&&) = default;

		RuntimeFiber(RuntimeFiber const&) = delete;
		RuntimeFiber& operator=(RuntimeFiber const&) = delete;

		~RuntimeFiber();

		RuntimeFiberRef& as_ref() {
			return *this;
		}

		RuntimeFiberRef const& as_ref() const {
			return *this;
		}

		std::optional<Stacktrace> get_stacktrace() const;

		std::optional<RuntimeFiberRef> get_parent();

		const lauf_runtime_value* get_vstack_ptr() const;

		bool resume(const lauf_runtime_value* input, size_t input_count, lauf_runtime_value* output, size_t output_count);
		bool resume_until_completion(
			const lauf_runtime_value* input, size_t input_count, lauf_runtime_value* output, size_t output_count
		);

		void destroy();

	private:
		friend RuntimeFiberRef;
		friend struct RuntimeProcess;
		RuntimeFiber(RuntimeProcess* process, lauf_runtime_fiber* fiber) : RuntimeFiberRef(fiber), _process(process) {}

		RuntimeProcess* _process;
	};
}
