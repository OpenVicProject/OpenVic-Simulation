#pragma once

#include <span>

#include "openvic-simulation/vm/Function.hpp"
#include "openvic-simulation/vm/Module.hpp"

#include <lauf/asm/module.h>
#include <lauf/asm/program.h>

namespace OpenVic::Vm {
	struct ProgramRef {
		ProgramRef(lauf_asm_program program) : _program(program) {}

		operator lauf_asm_program*() {
			return &this->_program;
		}

		operator const lauf_asm_program*() const {
			return &this->_program;
		}

		operator lauf_asm_program&() {
			return this->_program;
		}

		operator lauf_asm_program const&() const {
			return this->_program;
		}

		std::string_view debug_path(const Function func) const {
			return lauf_asm_program_debug_path(*this, func);
		}

		lauf_asm_debug_location find_debug_location_of(const lauf_asm_inst* ip) const {
			return lauf_asm_program_find_debug_location_of_instruction(*this, ip);
		}

		const Function entry() const {
			return const_cast<lauf_asm_function*>(lauf_asm_program_entry_function(*this));
		}

		void link_modules(std::span<ModuleRef> mods) {
			std::span<const lauf_asm_module*> ptrs = Module::make_span(mods);
			lauf_asm_link_modules(*this, ptrs.data(), ptrs.size());
		}
		void link_module(ModuleRef mod) {
			lauf_asm_link_module(*this, mod);
		}

		template<typename T>
		void define_native_global(const Global global, T* ptr) {
			lauf_asm_define_native_global(*this, global, ptr, sizeof(T));
		}

		void define_native_function(const Function func, lauf_asm_native_function native_func, void* user_data = nullptr) {
			lauf_asm_define_native_function(*this, func, native_func, user_data);
		}

	protected:
		lauf_asm_program _program;
	};

	struct Program : ProgramRef {
		using ProgramRef::ProgramRef;

		Program(Program&& other) : ProgramRef(other._program) {
			other._program = {};
		}

		~Program() {
			if (_program._mod == nullptr) {
				return;
			}
			lauf_asm_destroy_program(_program);
			_program = {};
		}

		Program& operator=(Program&& other) {
			this->_program = other._program;
			other._program = {};
			return *this;
		}

		Program(Program const&) = delete;
		Program& operator=(Program const&) = delete;

		ProgramRef& as_ref() {
			return *this;
		}

		ProgramRef const& as_ref() const {
			return *this;
		}
	};
}
