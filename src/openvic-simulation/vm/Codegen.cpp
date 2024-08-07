#include "Codegen.hpp"

#include <bit> // IWYU pragma: keep
#include <cctype>
#include <charconv>
#include <cstring>
#include <string_view>
#include <unordered_map>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/Utility.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <tsl/ordered_map.h>

#include <dryad/_detail/assert.hpp>
#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/reverse.hpp>

#include "types/fixed_point/FixedPoint.hpp"
#include "vm/Builtin.hpp"
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

Codegen::Codegen(
	ovdl::v2script::Parser const& parser, OpenVic::InstanceManager* instance_manager, const char* module_name,
	lauf_asm_build_options options
)
	: _parser(parser), _instance_manager(instance_manager), _module(module_name), _builder(options) {}

Codegen::Codegen(
	ovdl::v2script::Parser const& parser, OpenVic::InstanceManager* instance_manager, Module&& module, AsmBuilder&& builder
)
	: _parser(parser), _instance_manager(instance_manager), _module(std::move(module)), _builder(std::move(builder)) {}

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

	if (!name) {
		return None;
	}

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
	Asm::argument::type_t ov_asm_type;

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

				arguments = lauf_asm_build_local(
					*this, lauf_asm_array_layout(LAUF_ASM_NATIVE_LAYOUT_OF(Asm::argument), named_arguments.size())
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
				arguments = lauf_asm_build_local(*this, lauf_asm_array_layout(LAUF_ASM_NATIVE_LAYOUT_OF(Asm::argument), 0));
			} else if (_parser.find_intern("no"sv) != right->value()) {
				arguments = lauf_asm_build_local(*this, lauf_asm_array_layout(LAUF_ASM_NATIVE_LAYOUT_OF(Asm::argument), 1));
				inst_store_ov_asm_key_null(arguments, 0);

				visitor(right);
				inst_store_ov_asm_value_from_vstack(arguments, 0, ovdl::detail::to_underlying(ov_asm_type));

				inst_store_ov_asm_type(arguments, 0, ovdl::detail::to_underlying(ov_asm_type));
			}

			// Load arguments address (vstack[2])
			lauf_asm_inst_local_addr(*this, arguments);
			// Load scope address in lauf (vstack[1])
			push_instruction_for_keyword_scope(scope_execution_type::Effect, current_scope.type, current_scope.symbol);
			// Create effect name literal (vstack[0])
			auto effect_name = lauf_asm_build_string_literal(*this, left->value().c_str());
			lauf_asm_inst_global_addr(*this, effect_name);
			// Consumes vstack[0], vstack[1], and vstack[2]
			lauf_asm_inst_call_builtin(*this, call_effect);

			if (is_iterative) {
				// TODO: loop footer
			}
		},
		[&](const FlatValue* value) {
			using enum Asm::argument::type_t;
			if (!is_arguments) {
				return;
			}

			if (push_instruction_for_keyword_scope(scope_execution_type::Effect, current_scope.type, value->value())) {
				ov_asm_type = scope;
				return;
			}

			auto view = value->value().view();
			if (view[0] == '-') {
				if (std::int64_t value; std::from_chars(view.begin(), view.end(), value).ptr == view.end()) {
					lauf_asm_inst_sint(*this, value);
					ov_asm_type = int_;
					return;
				}
			}

			if (std::uint64_t value; std::from_chars(view.begin(), view.end(), value).ptr == view.end()) {
				lauf_asm_inst_uint(*this, value);
				ov_asm_type = uint;
				return;
			}

			{
				bool success;
				auto value = fixed_point_t::parse(view, &success);
				if (success) {
					lauf_asm_inst_sint(*this, value.get_raw_value());
					ov_asm_type = fixed_point;
					return;
				}
			}

			// Create argument string literal
			auto argument_str = lauf_asm_build_string_literal(*this, value->value().c_str());
			lauf_asm_inst_global_addr(*this, argument_str);
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

bool Codegen::push_instruction_for_keyword_scope(
	scope_execution_type execution_type, scope_type type, ovdl::symbol<char> scope_symbol
) {
	if (type >> get_scope_type_for(execution_type, scope_symbol)) {
		lauf_asm_inst_uint(*this, _scope_references.size());
		_scope_references.push_back(get_scope_for(execution_type, type, scope_symbol));
		// TODO: push scope into _scope_references
		lauf_asm_inst_call_builtin(*this, load_scope_ptr);
	}

	return false;
}

OpenVic::Asm::scope_variant Codegen::get_scope_for( //
	scope_execution_type execution_type, scope_type type, ovdl::symbol<char> scope_symbol
) const {
	return {};
}

static constexpr lauf_asm_layout ov_asm_argument_layout[] = { LAUF_ASM_NATIVE_LAYOUT_OF(OpenVic::Asm::argument::key),
															  LAUF_ASM_NATIVE_LAYOUT_OF(OpenVic::Asm::argument::type),
															  LAUF_ASM_NATIVE_LAYOUT_OF(OpenVic::Asm::argument::value) };

bool Codegen::inst_store_ov_asm_key_null(lauf_asm_local* local, std::size_t index) {
	lauf_asm_inst_null(*this);
	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(*this, local);
	// vstack[0]
	lauf_asm_inst_uint(*this, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(*this, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(*this, 0, ov_asm_argument_layout, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(*this, lauf_asm_type_value, 0);
	return true;
}

bool Codegen::inst_store_ov_asm_key(lauf_asm_local* local, std::size_t index, ovdl::symbol<char> key) {
	// Create key literal
	auto argument_key = lauf_asm_build_string_literal(*this, key.c_str());
	lauf_asm_inst_global_addr(*this, argument_key);
	// Translate key literal to cstring
	lauf_asm_inst_call_builtin(*this, translate_address_to_string);
	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(*this, local);
	// vstack[0]
	lauf_asm_inst_uint(*this, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(*this, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(*this, 0, ov_asm_argument_layout, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(*this, lauf_asm_type_value, 0);
	return true;
}

bool Codegen::inst_store_ov_asm_value_from_vstack(lauf_asm_local* local, std::size_t index, std::uint8_t type) {
	switch (ovdl::detail::from_underlying<Asm::argument::type_t>(type)) {
		using enum Asm::argument::type_t;
	case cstring:
		// Translate key literal to cstring
		lauf_asm_inst_call_builtin(*this, translate_address_to_string);
	case ptr:
		// Translate address to pointer
		lauf_asm_inst_call_builtin(*this, translate_address_to_pointer);
	case scope:
		// Scope pointer is already native
	case uint:
	case int_:
	case fixed_point:
		// needs no translation
		break;
	}

	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(*this, local);
	// vstack[0]
	lauf_asm_inst_uint(*this, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(*this, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(*this, 2, ov_asm_argument_layout, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(*this, lauf_asm_type_value, 0);
	return true;
}

bool Codegen::inst_store_ov_asm_type(lauf_asm_local* local, std::size_t index, std::uint8_t type) {
	lauf_asm_inst_uint(*this, static_cast<lauf_uint>(type));
	// Start Load arguments //
	// vstack[1]
	lauf_asm_inst_local_addr(*this, local);
	// vstack[0]
	lauf_asm_inst_uint(*this, index);
	// Consumes vstack[0] = arguments_index, vstack[1] = argument_key_address, produces array address as
	// vstack[0]
	lauf_asm_inst_array_element(*this, lauf_asm_type_value.layout);
	// End Load arguments //
	lauf_asm_inst_aggregate_member(*this, 1, ov_asm_argument_layout, 3);
	// Consumes vstack[0] and vstack[1]
	lauf_asm_inst_store_field(*this, lauf_asm_type_value, 0);
	return true;
}
