#include "vm/Builtin.hpp"

#include "GameManager.hpp"
#include <lauf/asm/type.h>
#include <lauf/runtime/builtin.h>
#include <lauf/runtime/memory.h>
#include <lauf/runtime/process.h>
#include <lauf/runtime/value.h>

OpenVic::Asm::scope_store_variant* create_scope( //
	OpenVic::GameManager* instance_manager, const char* scope_name, OpenVic::Asm::scope_store_variant* relative_scope
) {
	// TODO: create scope based on scope_name and relative_scope
	return nullptr;
}

bool execute_effect(
	OpenVic::GameManager* instance_manager, const char* effect_id, OpenVic::Asm::scope_store_variant& scope,
	OpenVic::Asm::argument* arguments, std::size_t arg_count
) {
	// TODO: execute effect based on id, scope, and arguments
	return true;
}

bool execute_trigger(
	OpenVic::GameManager* instance_manager, const char* trigger_id, OpenVic::Asm::scope_store_variant& scope,
	OpenVic::Asm::argument* arguments, std::size_t arg_count, bool* result
) {
	// TODO: execute trigger based on id, scope, and arguments
	return true;
}

LAUF_RUNTIME_BUILTIN(call_effect, 3, 0, LAUF_RUNTIME_BUILTIN_DEFAULT, "call_effect", nullptr) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}
	auto vm_data = static_cast<OpenVic::Vm::VmUserData*>(user_data);

	auto effect_name_addr = vstack_ptr[0].as_address;
	auto scope_def_ptr = static_cast<OpenVic::Asm::scope_store_variant*>(vstack_ptr[1].as_native_ptr);
	auto argument_array_addr = vstack_ptr[2].as_address;

	auto effect_name = lauf_runtime_get_cstr(process, effect_name_addr);
	if (effect_name == nullptr) {
		return lauf_runtime_panic(process, "invalid effect name address");
	}

	lauf_runtime_allocation array_allocation;
	if (!lauf_runtime_get_allocation(process, argument_array_addr, &array_allocation)) {
		return lauf_runtime_panic(process, "invalid arguments address");
	}

	std::size_t count = array_allocation.size / lauf_asm_type_value.layout.size;

	auto argument_array =
		static_cast<OpenVic::Asm::argument*>(lauf_runtime_get_mut_ptr(process, argument_array_addr, { 1, 1 }));

	if (!execute_effect(vm_data->game_manager, effect_name, *scope_def_ptr, argument_array, count)) {
		return lauf_runtime_panic(process, "effect could not be found");
	}

	vstack_ptr += 3;
	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(call_trigger, 3, 1, LAUF_RUNTIME_BUILTIN_DEFAULT, "call_trigger", &call_effect) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}
	auto vm_data = static_cast<OpenVic::Vm::VmUserData*>(user_data);

	auto trigger_name_addr = vstack_ptr[0].as_address;
	auto scope_ptr = vstack_ptr[1].as_native_ptr;
	auto argument_array_addr = vstack_ptr[2].as_address;

	auto effect_name = lauf_runtime_get_cstr(process, trigger_name_addr);
	if (effect_name == nullptr) {
		return lauf_runtime_panic(process, "invalid trigger name address");
	}

	lauf_runtime_allocation array_allocation;
	if (!lauf_runtime_get_allocation(process, argument_array_addr, &array_allocation)) {
		return lauf_runtime_panic(process, "invalid arguments address");
	}

	std::size_t count = array_allocation.size / lauf_asm_type_value.layout.size;

	auto argument_array =
		static_cast<OpenVic::Asm::argument*>(lauf_runtime_get_mut_ptr(process, argument_array_addr, { 1, 1 }));

	bool trigger_result;

	if (!execute_trigger(
			vm_data->game_manager, effect_name, *static_cast<OpenVic::Asm::scope_store_variant*>(scope_ptr), argument_array,
			count, &trigger_result
		)) {
		return lauf_runtime_panic(process, "trigger could not be found");
	}

	vstack_ptr += 2;

	vstack_ptr[0].as_sint = trigger_result;

	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(
	translate_address_to_pointer, 1, 1, LAUF_RUNTIME_BUILTIN_DEFAULT, "translate_address_to_pointer", &call_trigger
) {
	auto address = vstack_ptr[0].as_address;

	auto ptr = lauf_runtime_get_const_ptr(process, address, { 0, 1 });
	if (ptr == nullptr) {
		return lauf_runtime_panic(process, "invalid address");
	}
	vstack_ptr[0].as_native_ptr = (void*)ptr;

	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(
	translate_address_to_string, 1, 1, LAUF_RUNTIME_BUILTIN_DEFAULT, "translate_address_to_string",
	&translate_address_to_pointer
) {
	auto address = vstack_ptr[0].as_address;

	auto ptr = lauf_runtime_get_cstr(process, address);
	if (ptr == nullptr) {
		return lauf_runtime_panic(process, "invalid address");
	}
	vstack_ptr[0].as_native_ptr = (void*)ptr;

	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(
	load_static_scope_ptr, 1, 1, LAUF_RUNTIME_BUILTIN_DEFAULT, "load_static_scope_ptr", &translate_address_to_string
) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}
	auto vm_data = static_cast<OpenVic::Vm::VmUserData*>(user_data);

	auto scope_ref_position = vstack_ptr[0].as_uint;

	if (scope_ref_position >= vm_data->scope_refs.size()) {
		return lauf_runtime_panic(process, "invalid scope reference value");
	}

	vstack_ptr[0].as_native_ptr = &vm_data->scope_refs[scope_ref_position];

	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(
	load_relative_scope_ptr, 2, 1, LAUF_RUNTIME_BUILTIN_DEFAULT, "load_relative_scope_ptr", &load_static_scope_ptr
) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}
	auto vm_data = static_cast<OpenVic::Vm::VmUserData*>(user_data);

	auto scope_name_addr = vstack_ptr[0].as_address;
	auto vstack_1 = vstack_ptr[1];
	auto relative_scope = static_cast<OpenVic::Asm::scope_store_variant*>(vstack_ptr[1].as_native_ptr);

	auto scope_name = lauf_runtime_get_cstr(process, scope_name_addr);
	if (scope_name == nullptr) {
		return lauf_runtime_panic(process, "invalid scope name address");
	}

	vstack_ptr++;

	auto scope = create_scope(vm_data->game_manager, scope_name, relative_scope);

	if (scope == nullptr) {
		return lauf_runtime_panic(process, "could not create scope");
	}

	vm_data->scope_refs.emplace_back(std::move(*scope));
	delete scope;

	vstack_ptr[0].as_native_ptr = &vm_data->scope_refs.back();

	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(push_current_scope, 1, 0, LAUF_RUNTIME_BUILTIN_DEFAULT, "push_current_scope", &load_relative_scope_ptr) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}
	auto vm_data = static_cast<OpenVic::Vm::VmUserData*>(user_data);

	auto current_scope = static_cast<OpenVic::Asm::scope_store_variant*>(vstack_ptr[0].as_native_ptr);

	vm_data->scope_stack.push(current_scope);

	vstack_ptr++;
	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(pop_current_scope, 0, 0, LAUF_RUNTIME_BUILTIN_DEFAULT, "pop_current_scope", &push_current_scope) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}
	auto vm_data = static_cast<OpenVic::Vm::VmUserData*>(user_data);

	if (vm_data->scope_stack.empty()) {
		return lauf_runtime_panic(process, "scope stack empty");
	}

	if (vm_data->scope_stack.size() == 1) {
		return lauf_runtime_panic(process, "attempt to pop last scope in stack");
	}

	vm_data->scope_stack.pop();

	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

LAUF_RUNTIME_BUILTIN(load_current_scope, 0, 1, LAUF_RUNTIME_BUILTIN_DEFAULT, "load_current_scope", &pop_current_scope) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}
	auto vm_data = static_cast<OpenVic::Vm::VmUserData*>(user_data);

	if (vm_data->scope_stack.empty()) {
		return lauf_runtime_panic(process, "scope stack empty");
	}

	vstack_ptr--;
	vstack_ptr[0].as_native_ptr = vm_data->scope_stack.top();

	LAUF_RUNTIME_BUILTIN_DISPATCH;
}
