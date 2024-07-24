#pragma once

#include <optional>

#include <lauf/runtime/stacktrace.h>

namespace OpenVic::Vm {
	struct RuntimeFiber;

	struct StacktraceRef {
		lauf_runtime_stacktrace* handle() {
			return _handle;
		}

		const lauf_runtime_stacktrace* handle() const {
			return _handle;
		}

		operator lauf_runtime_stacktrace*() {
			return _handle;
		}

		operator const lauf_runtime_stacktrace*() const {
			return _handle;
		}

		std::optional<StacktraceRef> get_parent() {
			auto result = lauf_runtime_stacktrace_parent(_handle);
			if (result == nullptr) {
				return std::nullopt;
			}
			return StacktraceRef(result);
		}

	protected:
		friend struct RuntimeFiber;
		StacktraceRef(lauf_runtime_stacktrace* stacktrace) : _handle(stacktrace) {}

		lauf_runtime_stacktrace* _handle;
	};

	struct Stacktrace : StacktraceRef {
		Stacktrace(Stacktrace&&) = default;
		Stacktrace& operator=(Stacktrace&&) = default;

		StacktraceRef& as_ref() {
			return *this;
		}

		StacktraceRef const& as_ref() const {
			return *this;
		}

		Stacktrace(Stacktrace const&) = delete;
		Stacktrace& operator=(Stacktrace const&) = delete;

		~Stacktrace() {
			lauf_runtime_destroy_stacktrace(_handle);
		}

	private:
		friend struct RuntimeFiber;
		Stacktrace(lauf_runtime_stacktrace* stacktrace) : StacktraceRef(stacktrace) {}
	};
}
