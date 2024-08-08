#pragma once

#include <cstdint>
#include <stack>
#include <variant>

#include "GameManager.hpp"
#include "country/CountryDefinition.hpp"
#include "economy/BuildingType.hpp"
#include "map/Crime.hpp"
#include "map/ProvinceDefinition.hpp"
#include "map/State.hpp"
#include "military/Leader.hpp"
#include "military/LeaderTrait.hpp"
#include "military/UnitType.hpp"
#include "misc/Event.hpp"
#include "misc/Modifier.hpp"
#include "politics/Government.hpp"
#include "politics/Ideology.hpp"
#include "politics/Issue.hpp"
#include "politics/NationalValue.hpp"
#include "pop/Culture.hpp"
#include "pop/Pop.hpp"
#include "pop/Religion.hpp"
#include "research/Technology.hpp"
#include "types/fixed_point/FixedPoint.hpp"
#include <lauf/runtime/memory.h>
#include <lauf/runtime/value.h>

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

	// This builtin takes one argument:
	// * vstack_ptr[0] is a uint for the position of the pointer variant in scope_references
	// Returns scope native pointer
	extern const lauf_runtime_builtin load_static_scope_ptr;

	// This builtin takes two arguments:
	// * vstack_ptr[0] is an address for the name of the scope
	// * vstack_ptr[1] is the native pointer for the relative scope
	// Returns scope native pointer
	extern const lauf_runtime_builtin load_relative_scope_ptr;

	// This builtin takes one argument:
	// * vstack_ptr[0] is the native pointer to push to the scope stack
	extern const lauf_runtime_builtin push_current_scope;

	// This builtin takes no arguments and pops the current off the stack
	extern const lauf_runtime_builtin pop_current_scope;

	// This builtin takes no arguments
	// Returns scope native pointer
	extern const lauf_runtime_builtin load_current_scope;
}

namespace OpenVic::Asm {
	using scope_store_variant = std::variant<
		std::monostate, //
		// Actual scopes
		const OpenVic::CountryDefinition*, const OpenVic::Region*, const OpenVic::ProvinceDefinition*, OpenVic::Pop*, //
		// Variant data for scope-like behavior
		const OpenVic::Continent*, const OpenVic::PopType*, const OpenVic::UnitType*, const OpenVic::BuildingType*, //
		const OpenVic::Crime*, const OpenVic::Culture*, const OpenVic::Religion*, const OpenVic::ReformGroup*,
		const OpenVic::IssueGroup*, const OpenVic::Issue*, const OpenVic::Ideology*, const OpenVic::CountryParty*,
		OpenVic::LeaderBase*, const OpenVic::LeaderTrait*, const OpenVic::Event* /*, trade good, resource*/, //
		const OpenVic::Technology*, const OpenVic::GovernmentType* /*, casus belli */,
		const OpenVic::NationalValue*
		/*, variable, global flag, country flag */,
		const OpenVic::Modifier* /*, province flag */
		>;

	struct argument {
		const char* key;

		union {
			const char* as_cstr;
			const scope_store_variant* as_scope;
			std::uint64_t as_uint;
			std::int64_t as_int;
			const void* as_ptr;
			lauf_runtime_function_address as_function_address;
		} value;

		enum class type_t : std::uint8_t { //
			cstring,
			scope,
			uint,
			int_,
			ptr,
			function_address,
			fixed_point
		} type;

		const char* val_cstr() const {
			if (type != type_t::cstring) {
				return nullptr;
			}
			return value.as_cstr;
		}

		const scope_store_variant* val_scope() const {
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

		lauf_runtime_function_address val_func_addr() const {
			if (type != type_t::function_address) {
				return lauf_runtime_function_address_null;
			}
			return value.as_function_address;
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
		std::vector<Asm::scope_store_variant> scope_refs;
		std::stack<Asm::scope_store_variant*> scope_stack;
		GameManager* game_manager;
	};
}
