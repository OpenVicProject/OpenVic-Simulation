#pragma once

#include <optional>

#include "Utility.hpp"
#include <lauf/runtime/stacktrace.h>

namespace OpenVic::Vm {
	struct RuntimeFiber;

	struct StacktraceRef : utility::HandleBase<lauf_runtime_stacktrace> {
		using HandleBase<lauf_runtime_stacktrace>::HandleBase;

		std::optional<StacktraceRef> get_parent() {
			auto result = lauf_runtime_stacktrace_parent(_handle);
			if (result == nullptr) {
				_handle = nullptr;
				return std::nullopt;
			}
			return StacktraceRef(result);
		}
	};

	struct Stacktrace : utility::MoveOnlyHandleDerived<Stacktrace, StacktraceRef> {
		using MoveOnlyHandleDerived::MoveOnlyHandleDerived;
		using MoveOnlyHandleDerived::operator=;

		~Stacktrace() {
			if (_handle == nullptr) {
				return;
			}
			lauf_runtime_destroy_stacktrace(_handle);
			_handle = nullptr;
		}
	};
}
