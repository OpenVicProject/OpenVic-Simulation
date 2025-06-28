#pragma once

#include <span>
#include <string_view>

#include <range/v3/view/enumerate.hpp>

#include "openvic-simulation/vm/Chunk.hpp"
#include "openvic-simulation/vm/Function.hpp"
#include "openvic-simulation/vm/Global.hpp"
#include "openvic-simulation/vm/Handler.hpp"

#include <lauf/asm/module.h>
#include <lauf/asm/program.h>
#include <lauf/asm/type.h>

namespace OpenVic::Vm {
	struct Program;

	struct ModuleRef : HandlerRef<lauf_asm_module> {
		using HandlerRef::HandlerRef;

		std::string_view name() const {
			return lauf_asm_module_name(*this);
		}

		std::string_view debug_path() const {
			return lauf_asm_module_debug_path(*this);
		}

		void debug_path(const char* path) {
			lauf_asm_set_module_debug_path(*this, path);
		}

		Function add_function(const char* name, lauf_asm_signature sig) {
			return lauf_asm_add_function(*this, name, sig);
		}

		const Function find_function_by_name(const char* name) const {
			return const_cast<lauf_asm_function*>(lauf_asm_find_function_by_name(*this, name));
		}

		Global add_global(lauf_asm_global_permissions perms) {
			return lauf_asm_add_global(*this, perms);
		}

		Program create_program(const Function entry) const;
		Program create_program(const Chunk chunk) const;

		Chunk create_chunk() {
			return lauf_asm_create_chunk(*this);
		}

		void define_data(Global global, lauf_asm_layout layout, const void* data = nullptr) {
			lauf_asm_define_data_global(*this, global, layout, data);
		}

		template<typename T>
		void define_data_type(Global global, T& data) {
			define_data(global, { sizeof(T), alignof(T) }, static_cast<const void*>(&data));
		}

		template<typename T>
		void define_data_type(Global global) {
			define_data(global, { sizeof(T), alignof(T) }, nullptr);
		}

		template<typename T>
		void define_data_type(Global global, std::span<T> data) {
			define_data(global, { data.size_bytes(), alignof(T) }, static_cast<const void*>(data.data()));
		}

		template<typename CharT, typename CharTraits>
		void define_data_type(Global global, std::basic_string_view<CharT, CharTraits> data) {
			define_data(global, { sizeof(CharT) * data.size(), alignof(CharT) }, static_cast<const void*>(data.data()));
		}

		Global add_and_define_global(lauf_asm_global_permissions perms, lauf_asm_layout layout, const void* data = nullptr) {
			Global result = add_global(perms);
			define_data(result, layout, data);
			return result;
		}

		template<typename T>
		Global add_and_define_global(lauf_asm_global_permissions perms, T& data) {
			Global result = add_global(perms);
			define_data_type<T>(result, data);
			return result;
		}

		template<typename T>
		Global add_and_define_global(lauf_asm_global_permissions perms, std::span<T> data) {
			Global result = add_global(perms);
			define_data_type<T>(result, data);
			return result;
		}

		template<typename CharT, typename CharTraits>
		Global add_and_define_global(lauf_asm_global_permissions perms, std::basic_string_view<CharT, CharTraits> data) {
			Global result = add_global(perms);
			define_data_type<CharT, CharTraits>(result, data);
			return result;
		}

		LAUF_FORCE_INLINE static std::span<const lauf_asm_module*> make_span(std::span<ModuleRef> mods) {
			// While unsafe, ModuleRef is only a wrapper for lauf_asm_module* so a ModuleRef* should always reinterpret
			// into lauf_asm_module**
			return std::span<const lauf_asm_module*> { reinterpret_cast<const lauf_asm_module**>(mods.data()), mods.size() };
		}
	};

	struct Module : UniqueHandlerDerived<Module, ModuleRef> {
		using UniqueHandlerDerived::UniqueHandlerDerived;
		using UniqueHandlerDerived::operator=;

		Module(const char* module_name) : UniqueHandlerDerived(lauf_asm_create_module(module_name)) {}

		~Module() {
			if (_handle == nullptr) {
				return;
			}
			lauf_asm_destroy_module(_handle);
			_handle = nullptr;
		}
	};
}
