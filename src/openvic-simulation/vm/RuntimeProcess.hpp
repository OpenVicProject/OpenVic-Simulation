#pragma once

#include <optional>
#include <string_view>

#include "openvic-simulation/vm/Function.hpp"
#include "openvic-simulation/vm/Global.hpp"
#include "openvic-simulation/vm/Handler.hpp"
#include "openvic-simulation/vm/Program.hpp"
#include "openvic-simulation/vm/RuntimeFiber.hpp"

#include <lauf/asm/module.h>
#include <lauf/asm/type.h>
#include <lauf/runtime/memory.h>
#include <lauf/runtime/process.h>
#include <lauf/runtime/value.h>

namespace OpenVic::Vm {
	struct VirtualMachineRef;

	struct RuntimeProcess : UniqueHandler<RuntimeProcess, lauf_runtime_process> {
		using UniqueHandler::UniqueHandler;
		using UniqueHandler::operator=;

		~RuntimeProcess() {
			if (_handle == nullptr) {
				return;
			}
			lauf_runtime_destroy_process(*this);
			_handle = nullptr;
		}

		RuntimeFiber create_fiber(const lauf_asm_function* fn) {
			return RuntimeFiber(this, lauf_runtime_create_fiber(*this, fn));
		}

		RuntimeFiberRef get_current_fiber() {
			return lauf_runtime_get_current_fiber(*this);
		}

		struct FiberIterable {
			struct iterator {
				lauf_runtime_fiber* _current;

				RuntimeFiberRef operator*() {
					return _current;
				}

				bool operator==(iterator it) const {
					return _current == it._current;
				}

				iterator operator++() {
					return { lauf_runtime_iterate_fibers_next(_current) };
				}
			};

			iterator _begin;

			iterator begin() {
				return _begin;
			}

			iterator end() {
				return { nullptr };
			}
		};

		FiberIterable get_fiber_iterable() {
			return { lauf_runtime_iterate_fibers(*this) };
		}

		bool is_single_fibered() {
			return lauf_runtime_is_single_fibered(*this);
		}

		std::optional<RuntimeFiberRef> get_fiber_ptr(lauf_runtime_address addr) {
			lauf_runtime_fiber* result = lauf_runtime_get_fiber_ptr(*this, addr);
			return result == nullptr ? std::nullopt : std::optional<RuntimeFiberRef> { result };
		}

		void const* get_const_ptr(lauf_runtime_address addr, lauf_asm_layout layout) {
			return lauf_runtime_get_const_ptr(*this, addr, layout);
		}

		void* get_ptr(lauf_runtime_address addr, lauf_asm_layout layout) {
			return lauf_runtime_get_mut_ptr(*this, addr, layout);
		}

		std::string_view get_string(lauf_runtime_address addr) {
			const char* result = lauf_runtime_get_cstr(*this, addr);
			return result == nullptr ? std::string_view {} : result;
		}

		bool get_address(const void* ptr, lauf_runtime_address_store* r_addr) {
			return lauf_runtime_get_address(*this, r_addr, ptr);
		}

		lauf_runtime_address get_global_address(const Global global) {
			return lauf_runtime_get_global_address(*this, global);
		}

		std::optional<const Function> get_function(lauf_runtime_function_address addr) {
			const lauf_asm_function* result = lauf_runtime_get_function_ptr_any(*this, addr);
			return result == nullptr ? std::nullopt : std::optional<const Function> { const_cast<lauf_asm_function*>(result) };
		}

		std::optional<const Function> get_function(lauf_runtime_function_address addr, lauf_asm_signature signature) {
			const lauf_asm_function* result = lauf_runtime_get_function_ptr(*this, addr, signature);
			return result == nullptr ? std::nullopt : std::optional<const Function> { const_cast<lauf_asm_function*>(result) };
		}

		std::optional<RuntimeFiberRef> get_fiber(lauf_runtime_address addr) {
			lauf_runtime_fiber* result = lauf_runtime_get_fiber_ptr(*this, addr);
			return result == nullptr ? std::nullopt : std::optional<RuntimeFiberRef> { result };
		}

		VirtualMachineRef get_vm();

		void* get_user_data() {
			return lauf_runtime_get_vm_user_data(*this);
		}

		ProgramRef get_program() {
			return *lauf_runtime_get_program(*this);
		}

		std::optional<RuntimeFiberRef> get_parent_for(RuntimeFiberRef fiber) {
			lauf_runtime_fiber* result = lauf_runtime_get_fiber_parent(*this, fiber);
			return result == nullptr ? std::nullopt : std::optional<RuntimeFiberRef> { result };
		}

		std::optional<Stacktrace> get_stacktrace_for(RuntimeFiberRef fiber) {
			lauf_runtime_stacktrace* result = lauf_runtime_get_stacktrace(*this, fiber);
			return result == nullptr ? std::nullopt : std::optional<Stacktrace> { result };
		}

		const lauf_runtime_value* get_vstack_ptr_for(RuntimeFiberRef fiber) {
			return lauf_runtime_get_vstack_ptr(*this, fiber);
		}

	private:
		friend struct VirtualMachineRef;
		RuntimeProcess(lauf_runtime_process* process) : UniqueHandler(process) {}
	};
}
