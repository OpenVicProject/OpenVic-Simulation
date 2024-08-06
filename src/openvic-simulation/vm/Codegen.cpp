#include "Codegen.hpp"

#include <bit> // IWYU pragma: keep
#include <cctype>
#include <charconv>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/Utility.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <tsl/ordered_map.h>

#include <dryad/_detail/assert.hpp>
#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/reverse.hpp>

#include "InstanceManager.hpp"
#include "types/fixed_point/FixedPoint.hpp"
#include <lauf/asm/builder.h>
#include <lauf/asm/module.h>
#include <lauf/runtime/builtin.h>
#include <lauf/runtime/memory.h>
#include <lauf/runtime/process.h>
#include <lauf/runtime/value.h>

using namespace OpenVic::Vm;
using namespace ovdl::v2script::ast;
using namespace ovdl::v2script;

using namespace std::string_view_literals;

bool ichar_equals(char a, char b) {
	return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

using scope_variant = std::variant<std::monostate>;
using argument_variant = std::variant<std::monostate>;
using argument_map = std::unordered_map<std::string_view, argument_variant>;

static constexpr auto argument_size = 32;

struct ov_asm_argument {
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

bool execute_effect(
	OpenVic::InstanceManager* instance_manager, const char* effect_id, scope_variant& scope, ov_asm_argument*,
	std::size_t arg_count
) {
	return true;
}

ov_asm_argument arguments[argument_size];

// This builtin takes three arguments:
// * vstack_ptr[0] is an address of the name of the effect
// * vstack_ptr[1] is an address of the scope reference to apply the effect by
// * vstack_ptr[2] is an address to an array of pointers to the arguments for the effect
LAUF_RUNTIME_BUILTIN(call_effect, 3, 0, LAUF_RUNTIME_BUILTIN_DEFAULT, "call_effect", nullptr) {
	auto user_data = lauf_runtime_get_vm_user_data(process);
	if (user_data == nullptr) {
		return lauf_runtime_panic(process, "invalid user data");
	}

	auto effect_name_addr = vstack_ptr[0].as_address;
	auto scope_addr = vstack_ptr[1].as_address;
	auto argument_array_addr = vstack_ptr[2].as_address;

	auto effect_name = lauf_runtime_get_cstr(process, effect_name_addr);
	if (effect_name == nullptr) {
		return lauf_runtime_panic(process, "invalid effect name address");
	}

	auto scope_ptr = lauf_runtime_get_mut_ptr(process, scope_addr, { 0, 1 });
	if (scope_ptr == nullptr) {
		return lauf_runtime_panic(process, "invalid scope address");
	}

	lauf_runtime_allocation array_allocation;
	if (!lauf_runtime_get_allocation(process, argument_array_addr, &array_allocation)) {
		return lauf_runtime_panic(process, "invalid arguments address");
	}

	std::size_t count = array_allocation.size / lauf_asm_type_value.layout.size;
	if (count > argument_size) {
		return lauf_runtime_panic(process, "too many arguments");
	}

	auto argument_array = static_cast<ov_asm_argument*>(lauf_runtime_get_mut_ptr(process, argument_array_addr, { 1, 1 }));

	if (!execute_effect(
			static_cast<OpenVic::InstanceManager*>(user_data), effect_name, *static_cast<scope_variant*>(scope_ptr),
			argument_array, count
		)) {
		return lauf_runtime_panic(process, "effect could not be found");
	}

	vstack_ptr += 3;
	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

// This builtin takes three arguments:
// * vstack_ptr[0] is an address of the name of the trigger
// * vstack_ptr[1] is an address of the scope to apply the trigger by
// * vstack_ptr[2] is an address to an array of pointers to the arguments for the trigger
// Returns whether the trigger evaluated to true
LAUF_RUNTIME_BUILTIN(call_trigger, 3, 1, LAUF_RUNTIME_BUILTIN_DEFAULT, "call_trigger", &call_effect) {
	auto effect_name_addr = vstack_ptr[0].as_address;
	auto scope_addr = vstack_ptr[1].as_address;
	auto argument_array_addr = vstack_ptr[2].as_address;

	auto effect_name = lauf_runtime_get_cstr(process, effect_name_addr);
	if (effect_name == nullptr) {
		return lauf_runtime_panic(process, "invalid address");
	}


	auto argument_addresses =
		static_cast<lauf_runtime_value*>(lauf_runtime_get_mut_ptr(process, argument_array_addr, { 1, 1 }));

	// TODO: call trigger by name

	vstack_ptr[0].as_sint = 1;

	vstack_ptr += 2;
	LAUF_RUNTIME_BUILTIN_DISPATCH;
}

// Translates a lauf address into the native pointer representation.
// It takes one argument, which is the address, and returns one argument, which is the pointer.
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
// As above, but translates to a C string.
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

Codegen::Codegen(ovdl::v2script::Parser const& parser, const char* module_name, lauf_asm_build_options options)
	: _parser(parser), _module(module_name), _builder(options) {}

Codegen::Codegen(ovdl::v2script::Parser const& parser, Module&& module, AsmBuilder&& builder)
	: _parser(parser), _module(std::move(module)), _builder(std::move(builder)) {}

static constexpr auto any_ = "any_"sv;
static constexpr auto all_ = "all_"sv;
static constexpr auto war_countries = "war_countries"sv;

bool Codegen::is_iterative_scope(scope_execution_type execution_type, scope_type active_scope_type, ovdl::symbol<char> name)
	const {
	return std::strncmp(name.c_str(), any_.data(), any_.size()) == 0 ||
		std::strncmp(name.c_str(), all_.data(), all_.size()) == 0 ||
		std::strncmp(name.c_str(), war_countries.data(), war_countries.size()) == 0;
}

Codegen::scope_type Codegen::get_scope_type_for(scope_execution_type execution_type, ovdl::symbol<char> name) const {
	using enum scope_type;

#define FIND_SCOPE(SCOPE_TYPE, NAME) \
	if (_parser.find_intern(#NAME##sv) == name) \
	return SCOPE_TYPE

	FIND_SCOPE(Generic, THIS);
	FIND_SCOPE(Generic, FROM);
	FIND_SCOPE(Generic, from);

	// Country and Pop Scope
	FIND_SCOPE(Country | Pop, cultural_union);

	// Country Scope
	FIND_SCOPE(Country, capital_scope);
	FIND_SCOPE(Country, overlord);
	FIND_SCOPE(Country, sphere_owner);
	// Country Scope Random
	FIND_SCOPE(Country, random_country);
	FIND_SCOPE(Country, random_owned);
	FIND_SCOPE(Country, random_pop);
	// Country Scope Iterative
	FIND_SCOPE(Country, all_core);
	FIND_SCOPE(Country, any_country);
	FIND_SCOPE(Country, any_core);
	FIND_SCOPE(Country, any_greater_power);
	FIND_SCOPE(Country, any_neighbor_country);
	FIND_SCOPE(Country, any_owned_province);
	FIND_SCOPE(Country, any_pop);
	FIND_SCOPE(Country, any_sphere_member);
	FIND_SCOPE(Country, any_state);
	FIND_SCOPE(Country, any_substate);
	FIND_SCOPE(Country, war_countries);

	// Province Scope
	FIND_SCOPE(Province, controller);
	FIND_SCOPE(Province, owner);
	FIND_SCOPE(Province, state_scope);
	// Province Scope Random
	FIND_SCOPE(Province, random_neighbor_province);
	FIND_SCOPE(Province, random_empty_neighbor_province);
	// Province Scope Iterative
	FIND_SCOPE(Province, any_core);
	FIND_SCOPE(Province, any_neighbor_province);
	FIND_SCOPE(Province, any_pop);

	// Pop Scope
	FIND_SCOPE(Pop, country);
	FIND_SCOPE(Pop, location);

#undef FIND_SCOPE
	return None;
}

void Codegen::generate_effect_from(scope_type type, Node* node) {
	struct current_scope_t {
		scope_type type;
		ovdl::symbol<char> symbol;
	} current_scope { .type = type, .symbol = {} };

	bool is_arguments = false;
	std::unordered_map<ovdl::symbol<char>, FlatValue const*, symbol_hash> named_arguments;
	ov_asm_argument::type_t ov_asm_type;

	dryad::visit_tree(
		node, //
		[&](dryad::child_visitor<NodeKind> visitor, const AssignStatement* statement) {
			auto const* left = dryad::node_try_cast<FlatValue>(statement->left());
			if (!left) {
				return;
			}

			auto const* right = dryad::node_try_cast<FlatValue>(statement->right());
			if (is_arguments) {
				if (right) {
					named_arguments.insert_or_assign(left->value(), right);
					return;
				}

				// Boils war's attacker_goal = { casus_belli = [casus belli] } down to attacker_goal = [casus belli]
				if (auto right_list = dryad::node_try_cast<ListValue>(statement->right())) {
					if (right_list->statements().empty()) {
						return;
					}
					if (auto right_statement = dryad::node_try_cast<AssignStatement>(right_list->statements().front())) {
						if (auto right_flat_value = dryad::node_try_cast<FlatValue>(right_statement->right())) {
							named_arguments.insert_or_assign(left->value(), right_flat_value);
						}
					}
				}
				return;
			}

			bool is_iterative = false;
			lauf_asm_local* arguments = nullptr;

			if (!right) {
				using enum Codegen::scope_execution_type;
				is_arguments = current_scope.type >> get_scope_type_for(Effect, left->value());
				if (!is_arguments) {
					current_scope_t previous_scope = current_scope;
					current_scope.symbol = left->value();
					visitor(right);
					current_scope = previous_scope;
					return;
				}

				is_iterative = is_iterative_scope(Effect, current_scope.type, current_scope.symbol);
				if (is_iterative) {
					// TODO: loop header
				}

				// TODO: build arguments from named_arguments

				arguments = lauf_asm_build_local(
					_builder, lauf_asm_array_layout(LAUF_ASM_NATIVE_LAYOUT_OF(ov_asm_argument), named_arguments.size())
				);

				std::size_t index = 0;
				for (auto&& [key, value] : named_arguments) {
					inst_store_ov_asm_key(arguments, index, key);

					visitor(value);
					inst_store_ov_asm_value_from_vstack(arguments, index, ovdl::detail::to_underlying(ov_asm_type));

					inst_store_ov_asm_type(arguments, index, ovdl::detail::to_underlying(ov_asm_type));
					index++;
				}

				named_arguments.clear();
				is_arguments = false;
			} else if (auto yes_symbol = _parser.find_intern("yes"sv); yes_symbol && right->value() == yes_symbol) {
				// Build empty arguments
				arguments = lauf_asm_build_local(_builder, lauf_asm_array_layout(lauf_asm_type_value.layout, 0));
			} else if (_parser.find_intern("no"sv) != right->value()) {
				arguments = lauf_asm_build_local(_builder, lauf_asm_array_layout(lauf_asm_type_value.layout, 1));
				inst_store_ov_asm_key_null(arguments, 0);

				visitor(right);
				inst_store_ov_asm_value_from_vstack(arguments, 0, ovdl::detail::to_underlying(ov_asm_type));

				inst_store_ov_asm_type(arguments, 0, ovdl::detail::to_underlying(ov_asm_type));
			}

			// Load arguments address (vstack[2])
			lauf_asm_inst_local_addr(_builder, arguments);
			// Load scope address in lauf (vstack[1])
			push_instruction_for_scope(current_scope.type, current_scope.symbol);
			// Create effect name literal (vstack[0])
			auto effect_name = lauf_asm_build_string_literal(_builder, left->value().c_str());
			lauf_asm_inst_global_addr(_builder, effect_name);
			// Consumes vstack[0], vstack[1], and vstack[2]
			lauf_asm_inst_call_builtin(_builder, call_effect);

			if (is_iterative) {
				// TODO: loop footer
			}
		},
		[&](const FlatValue* value) {
			using enum ov_asm_argument::type_t;
			if (!is_arguments) {
				return;
			}

			if (push_instruction_for_scope(current_scope.type, value->value())) {
				ov_asm_type = scope;
				return;
			}

			auto view = value->value().view();
			if (view[0] == '-') {
				if (std::int64_t value; std::from_chars(view.begin(), view.end(), value).ptr == view.end()) {
					lauf_asm_inst_sint(_builder, value);
					ov_asm_type = int_;
					return;
				}
			}

			if (std::uint64_t value; std::from_chars(view.begin(), view.end(), value).ptr == view.end()) {
				lauf_asm_inst_uint(_builder, value);
				ov_asm_type = uint;
				return;
			}

			{
				bool success;
				auto value = fixed_point_t::parse(view, &success);
				if (success) {
					lauf_asm_inst_sint(_builder, value.get_raw_value());
					ov_asm_type = fixed_point;
					return;
				}
			}

			// Create argument string literal
			auto argument_str = lauf_asm_build_string_literal(_builder, value->value().c_str());
			lauf_asm_inst_global_addr(_builder, argument_str);
			ov_asm_type = cstring;

			// TODO: find/create and insert value here
		}
	);
}

void Codegen::generate_condition_from(scope_type type, Node* node) {
	struct current_scope_t {
		scope_type type;
		ovdl::symbol<char> symbol;
	} current_scope { .type = type, .symbol = {} };

	bool is_prepping_args = false;
	tsl::ordered_map<std::string_view, std::string_view> prepped_arguments;

	dryad::visit_tree(
		node, //
		[&](dryad::child_visitor<NodeKind> visitor, AssignStatement* statement) {
			auto const* left = dryad::node_try_cast<FlatValue>(statement->left());
			if (!left) {
				return;
			}

			auto const* right = dryad::node_try_cast<FlatValue>(statement->right());
			if (!right) {
				using enum Codegen::scope_execution_type;
				bool is_iterative = is_iterative_scope(Trigger, current_scope.type, current_scope.symbol);
				if (is_iterative) {}
				is_prepping_args = current_scope.type >> get_scope_type_for(Trigger, current_scope.symbol);
				visitor(right);
				if (is_iterative) {}
			} else if (auto yes_symbol = _parser.find_intern("yes"sv); yes_symbol && right->value() == yes_symbol) {
				// TODO: insert vic2 bytecode scope object address here
				// TODO: calls a builtin for Victoria 2 triggers?
			} else if (_parser.find_intern("no"sv) != right->value()) {
				// TODO: single argument execution
			}
		},
		[&](FlatValue* value) {
			// TODO: handle right side
		}
	);
}

bool Codegen::push_instruction_for_scope(scope_type type, ovdl::symbol<char> scope_symbol) {

	auto is_scope = [&](std::string_view name) -> bool {
		auto intern = _parser.find_intern(name);
		return intern && intern == scope_symbol;
	};

	if (is_scope("this"sv)) {
		inst_push_scope_this();
		return true;
	} else if (is_scope("from"sv)) {
		inst_push_scope_from();
		return true;
	} else if (is_scope("cultural_union"sv)) {
		DRYAD_PRECONDITION(std::has_single_bit(ovdl::detail::to_underlying(type)));
		switch (type) {
		case scope_type::Country: //
			inst_push_get_country_cultural_union();
			return true;
		case scope_type::State: //
			inst_push_get_state_cultural_union();
			return true;
		case scope_type::Province: //
			DRYAD_ASSERT(false, "province scope does not support cultural_union scope");
			break;
		case scope_type::Pop: //
			inst_push_get_pop_cultural_union();
			return true;
		default: return false;
		}
	} else {
		DRYAD_PRECONDITION(std::has_single_bit(ovdl::detail::to_underlying(type)));
		switch (type) {
		case scope_type::Country:
			if (is_scope("capital_scope"sv)) {
				inst_push_get_country_capital();
				return true;
			} else if (is_scope("overlord"sv)) {
				inst_push_get_country_overlord();
				return true;
			} else if (is_scope("sphere_owner"sv)) {
				inst_push_get_country_sphere_owner();
				return true;
			} else if (is_scope("random_country"sv)) {
				inst_push_get_random_country();
				return true;
			} else if (is_scope("random_owned"sv)) {
				inst_push_get_random_owned();
				return true;
			} else if (is_scope("random_pop"sv)) {
			}
			break;
		case scope_type::State: break;
		case scope_type::Province:
			if (is_scope("controller"sv)) {
				inst_push_get_province_controller();
				return true;
			} else if (is_scope("owner"sv)) {
				inst_push_get_province_owner();
				return true;
			} else if (is_scope("state_scope"sv)) {
				inst_push_get_province_state();
				return true;
			} else if (is_scope("random_neighbor_province"sv)) {
				inst_push_get_random_neighbor_province();
				return true;
			} else if (is_scope("random_empty_neighbor_province"sv)) {
				inst_push_get_random_empty_neighbor_province();
				return true;
			}
			break;
		case scope_type::Pop:
			if (is_scope("country"sv)) {
				inst_push_get_pop_country();
				return true;
			} else if (is_scope("location"sv)) {
				inst_push_get_pop_location();
				return true;
			}
			break;
		default: return false;
		}
	}
	return false;
}

static constexpr lauf_asm_layout aggregate_layouts[] = { LAUF_ASM_NATIVE_LAYOUT_OF(ov_asm_argument::key),
														 LAUF_ASM_NATIVE_LAYOUT_OF(ov_asm_argument::type),
														 LAUF_ASM_NATIVE_LAYOUT_OF(ov_asm_argument::value) };

bool Codegen::inst_store_ov_asm_key_null(lauf_asm_local* local, std::size_t index) {
	lauf_asm_inst_null(_builder);
	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(_builder, local);
	// vstack[0]
	lauf_asm_inst_uint(_builder, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(_builder, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(_builder, 0, aggregate_layouts, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(_builder, lauf_asm_type_value, 0);
	return true;
}

bool Codegen::inst_store_ov_asm_key(lauf_asm_local* local, std::size_t index, ovdl::symbol<char> key) {
	// Create key literal
	auto argument_key = lauf_asm_build_string_literal(_builder, key.c_str());
	lauf_asm_inst_global_addr(_builder, argument_key);
	// Translate key literal to cstring
	lauf_asm_inst_call_builtin(_builder, translate_address_to_string);
	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(_builder, local);
	// vstack[0]
	lauf_asm_inst_uint(_builder, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(_builder, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(_builder, 0, aggregate_layouts, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(_builder, lauf_asm_type_value, 0);
	return true;
}

bool Codegen::inst_store_ov_asm_value_from_vstack(lauf_asm_local* local, std::size_t index, std::uint8_t type) {
	switch (ovdl::detail::from_underlying<ov_asm_argument::type_t>(type)) {
		using enum ov_asm_argument::type_t;
	case cstring:
		// Translate key literal to cstring
		lauf_asm_inst_call_builtin(_builder, translate_address_to_string);
	case scope:
	case ptr:
		// Translate address to pointer
		lauf_asm_inst_call_builtin(_builder, translate_address_to_pointer);
	case uint:
	case int_:
	case fixed_point:
		// needs no translation
		break;
	}

	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(_builder, local);
	// vstack[0]
	lauf_asm_inst_uint(_builder, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(_builder, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(_builder, 2, aggregate_layouts, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(_builder, lauf_asm_type_value, 0);
	return true;
}

bool Codegen::inst_store_ov_asm_type(lauf_asm_local* local, std::size_t index, std::uint8_t type) {
	lauf_asm_inst_uint(_builder, static_cast<lauf_uint>(type));
	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(_builder, local);
	// vstack[0]
	lauf_asm_inst_uint(_builder, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(_builder, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(_builder, 1, aggregate_layouts, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(_builder, lauf_asm_type_value, 0);
	return true;
}
