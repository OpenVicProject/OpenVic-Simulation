#pragma once

#include <cstdint>
#include <variant>

#include "InstanceManager.hpp"
#include "types/fixed_point/FixedPoint.hpp"

extern "C" {
	typedef struct lauf_runtime_builtin lauf_runtime_builtin;

	// This builtin takes three arguments:
	// * vstack_ptr[0] is an address of the name of the effect
	// * vstack_ptr[1] is an address of the scope reference to apply the effect by
	// * vstack_ptr[2] is an address to an array of pointers to the arguments for the effect
	extern const lauf_runtime_builtin call_effect;

	// This builtin takes three arguments:
	// * vstack_ptr[0] is an address of the name of the trigger
	// * vstack_ptr[1] is an address of the scope to apply the trigger by
	// * vstack_ptr[2] is an address to an array of pointers to the arguments for the trigger
	// Returns whether the trigger evaluated to true
	extern const lauf_runtime_builtin call_trigger;

	// Translates a lauf address into the native pointer representation.
	// It takes one argument, which is the address, and returns one argument, which is the pointer.
	extern const lauf_runtime_builtin translate_address_to_pointer;
	// As above, but translates to a C string.
	extern const lauf_runtime_builtin translate_address_to_string;

	// This builtin takes three arguments:
	// * vstack_ptr[0] is a uint for the position of the pointer variant in scope_references
	// Returns scope native pointer
	extern const lauf_runtime_builtin load_scope_ptr;
}

namespace OpenVic::Asm {
	using scope_variant = std::variant<std::monostate>;

	struct argument {
		const char* key;

		union {
			const char* as_cstr;
			const scope_variant* as_scope;
			std::uint64_t as_uint;
			std::int64_t as_int;
			const void* as_ptr;
		} value;

		enum class type_t : std::uint8_t { //
			cstring,
			scope,
			uint,
			int_,
			ptr,
			fixed_point
		} type;

		const char* val_cstr() const {
			if (type != type_t::cstring) {
				return nullptr;
			}
			return value.as_cstr;
		}

		const scope_variant* val_scope() const {
			if (type != type_t::scope) {
				return nullptr;
			}
			return value.as_scope;
		}

		std::uint64_t val_uint() const {
			if (type != type_t::uint) {
				return 0;
			}
			return value.as_uint;
		}

		std::int64_t val_int() const {
			if (type != type_t::int_) {
				return 0;
			}
			return value.as_int;
		}

		const void* val_ptr() const {
			if (type != type_t::ptr) {
				return nullptr;
			}
			return value.as_ptr;
		}

		OpenVic::fixed_point_t val_fixed() const {
			if (type != type_t::fixed_point) {
				return 0;
			}
			return OpenVic::fixed_point_t(value.as_int);
		}
	};
}

namespace OpenVic::Vm {
	struct VmUserData {
		std::vector<Asm::scope_variant> scope_references;
		InstanceManager* instance_manager;
	};
}
