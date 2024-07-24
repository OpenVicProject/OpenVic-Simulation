#pragma once

#include <memory>
#include <span>
#include <type_traits>

#include <range/v3/view/enumerate.hpp>

#include "vm/Utility.hpp"
#include <lauf/asm/module.h>
#include <lauf/asm/program.h>
#include <lauf/asm/type.h>

namespace OpenVic::Vm {
	template<typename T>
	concept Arithmetic = std::is_arithmetic_v<T>;

	struct ModuleRef : utility::HandleBase<lauf_asm_module> {
		using HandleBase::HandleBase;

		const char* name() const {
			return lauf_asm_module_name(_handle);
		}

		const char* debug_path() const {
			return lauf_asm_module_debug_path(_handle);
		}

		void debug_path(const char* path) {
			lauf_asm_set_module_debug_path(_handle, path);
		}

		lauf_asm_function* add_function(const char* name, lauf_asm_signature sig) {
			return lauf_asm_add_function(_handle, name, sig);
		}

		const lauf_asm_function* find_function_by_name(const char* name) const {
			return lauf_asm_find_function_by_name(_handle, name);
		}

		lauf_asm_global* add_global(lauf_asm_global_permissions perms) {
			return lauf_asm_add_global(_handle, perms);
		}

		lauf_asm_program create_program(const lauf_asm_function* entry) const {
			return lauf_asm_create_program(_handle, entry);
		}

		void define_data(lauf_asm_global* global, lauf_asm_layout layout, const void* data) {
			lauf_asm_define_data_global(_handle, global, layout, data);
		}

		template<Arithmetic T>
		void define_data_type(lauf_asm_global* global, const T* data = nullptr) {
			define_data(global, { sizeof(T), alignof(T) }, static_cast<const void*>(data));
		}

		static std::unique_ptr<const lauf_asm_module* const[]> make_list(std::span<ModuleRef> mods) {
			auto result = new const lauf_asm_module*[mods.size()];
			auto span = std::span<const lauf_asm_module*> { result, mods.size() };
			for (auto [count, element] : span | ranges::views::enumerate) {
				element = mods[count].handle();
			}
			return std::unique_ptr<const lauf_asm_module* const[]>(result);
		}
	};

	struct Module : utility::MoveOnlyHandleDerived<Module, ModuleRef> {
		using MoveOnlyHandleDerived::MoveOnlyHandleDerived;
		using MoveOnlyHandleDerived::operator=;

		Module(const char* module_name) : MoveOnlyHandleDerived(lauf_asm_create_module(module_name)) {}

		~Module() {
			if (_handle == nullptr) {
				return;
			}
			lauf_asm_destroy_module(_handle);
			_handle = nullptr;
		}
	};
}
