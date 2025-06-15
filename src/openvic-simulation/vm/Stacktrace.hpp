#pragma once

#include "openvic-simulation/vm/Function.hpp"
#include "openvic-simulation/vm/Handler.hpp"

#include <lauf/asm/module.h>
#include <lauf/runtime/stacktrace.h>

namespace OpenVic::Vm {
	struct RuntimeFiber;

	struct StacktraceRef : HandlerRef<lauf_runtime_stacktrace> {
		using HandlerRef::HandlerRef;

		const Function get_function() {
			return const_cast<lauf_asm_function*>(lauf_runtime_stacktrace_function(*this));
		}

		const lauf_asm_inst* get_instruction() {
			return lauf_runtime_stacktrace_instruction(*this);
		}
	};

	struct Stacktrace : UniqueHandlerDerived<Stacktrace, StacktraceRef> {
		using UniqueHandlerDerived::UniqueHandlerDerived;
		using UniqueHandlerDerived::operator=;

		struct ParentIterable {
			struct iterator {
				StacktraceRef operator*() {
					return _current;
				}

				bool operator==(iterator it) const {
					return _current == it._current;
				}

				iterator operator++() {
					return { lauf_runtime_stacktrace_parent(_current) };
				}

			private:
				friend struct ParentIterable;
				iterator(lauf_runtime_stacktrace* current) : _current(current) {}

				lauf_runtime_stacktrace* _current;
			};

			iterator begin() const {
				return _begin;
			}

			iterator end() const {
				return { nullptr };
			}

		private:
			friend struct Stacktrace;
			ParentIterable(lauf_runtime_stacktrace* begin) : _begin(begin) {}

			iterator _begin;
		};

		ParentIterable parent_iterable() && {
			ParentIterable iterable { *this };
			_handle = nullptr;
			return iterable;
		}

		Stacktrace parent() && {
			Stacktrace parent = lauf_runtime_stacktrace_parent(*this);
			_handle = nullptr;
			return parent;
		}

		~Stacktrace() {
			if (_handle == nullptr) {
				return;
			}
			lauf_runtime_destroy_stacktrace(*this);
			_handle = nullptr;
		}
	};
}
