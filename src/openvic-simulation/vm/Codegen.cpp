#include "Codegen.hpp"

#include <cctype>
#include <cstring>
#include <string_view>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <tsl/ordered_map.h>

#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include <range/v3/algorithm/equal.hpp>

#include <lauf/asm/builder.h>

using namespace OpenVic::Vm;
using namespace ovdl::v2script::ast;
using namespace ovdl::v2script;

using namespace std::string_view_literals;

bool ichar_equals(char a, char b) {
	return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

Codegen::Codegen(ovdl::v2script::Parser const& parser, const char* module_name, lauf_asm_build_options options)
	: _parser(parser), _module(module_name), _builder(options) {
	intern_scopes();
}

Codegen::Codegen(ovdl::v2script::Parser const& parser, Module&& module, AsmBuilder&& builder)
	: _parser(parser), _module(std::move(module)), _builder(std::move(builder)) {
	intern_scopes();
}

void Codegen::intern_scopes() {
	_has_top_level_province_scopes = static_cast<bool>(_parser.find_intern("province_event"sv));
	_has_top_level_country_scopes = static_cast<bool>(_parser.find_intern("country_event"sv)) ||
		static_cast<bool>(_parser.find_intern("political_decisions"sv));

#define PUSH_INTERN(CONTAINTER, NAME) \
	[&](auto&& scope_name) { \
		if (scope_name) \
			CONTAINTER.insert(scope_name); \
	}(_parser.find_intern(#NAME##sv))

	PUSH_INTERN(_all_scopes, THIS);
	PUSH_INTERN(_all_scopes, FROM);
	PUSH_INTERN(_all_scopes, from);
	PUSH_INTERN(_all_scopes, cultural_union);

	// Country Scope
	PUSH_INTERN(_country_scopes, capital_scope);
	PUSH_INTERN(_country_scopes, overlord);
	PUSH_INTERN(_country_scopes, sphere_owner);
	// Country Scope Random
	PUSH_INTERN(_country_scopes, random_country);
	PUSH_INTERN(_country_scopes, random_owned);
	PUSH_INTERN(_country_scopes, random_pop);
	// Country Scope Iterative
	PUSH_INTERN(_country_scopes, all_core);
	PUSH_INTERN(_country_scopes, any_country);
	PUSH_INTERN(_country_scopes, any_core);
	PUSH_INTERN(_country_scopes, any_greater_power);
	PUSH_INTERN(_country_scopes, any_neighbor_country);
	PUSH_INTERN(_country_scopes, any_owned_province);
	PUSH_INTERN(_country_scopes, any_pop);
	PUSH_INTERN(_country_scopes, any_sphere_member);
	PUSH_INTERN(_country_scopes, any_state);
	PUSH_INTERN(_country_scopes, any_substate);
	PUSH_INTERN(_country_scopes, war_countries);

	// Province Scope
	PUSH_INTERN(_province_scopes, controller);
	PUSH_INTERN(_province_scopes, owner);
	PUSH_INTERN(_province_scopes, state_scope);
	// Province Scope Random
	PUSH_INTERN(_province_scopes, random_neighbor_province);
	PUSH_INTERN(_province_scopes, random_empty_neighbor_province);
	// Province Scope Iterative
	PUSH_INTERN(_province_scopes, any_core);
	PUSH_INTERN(_province_scopes, any_neighbor_province);
	PUSH_INTERN(_province_scopes, any_pop);

	// Pop Scope
	PUSH_INTERN(_pop_scopes, country);
	PUSH_INTERN(_pop_scopes, location);


#undef PUSH_INTERN
}

constexpr auto any_ = "any_"sv;
constexpr auto all_ = "all_"sv;
constexpr auto war_countries = "war_countries"sv;

bool Codegen::is_iterative_scope(scope_execution_type execution_type, scope_type active_scope, ovdl::symbol<char> name) const {
	return std::strncmp(name.c_str(), any_.data(), any_.size()) == 0 ||
		std::strncmp(name.c_str(), all_.data(), all_.size()) == 0 ||
		std::strncmp(name.c_str(), war_countries.data(), war_countries.size()) == 0;
}

bool Codegen::is_scope_for(scope_execution_type execution_type, scope_type active_scope, ovdl::symbol<char> name) const {
	if (_all_scopes.contains(name)) {
		return true;
	}

	switch (active_scope) {
	case scope_type::Country:
		if (_country_scopes.contains(name)) {
			return true;
		}
		break;
	case scope_type::State: break;
	case scope_type::Province:
		if (_province_scopes.contains(name)) {
			return true;
		}
		break;
	case scope_type::Pop:
		if (_pop_scopes.contains(name)) {
			return true;
		}
		break;
	}

	return false;
}

void Codegen::generate_effect_from(Codegen::scope_type type, Node* node) {
	bool is_prepping_args = false;
	Codegen::scope_type current_scope = type;
	tsl::ordered_map<std::string_view, std::string_view> prepped_arguments;

	dryad::visit_tree(
		node, //
		[&](dryad::child_visitor<NodeKind> visitor, AssignStatement* statement) {
			auto const* left = dryad::node_try_cast<FlatValue>(statement->left());
			if (!left) {
				return;
			}

			if (is_prepping_args) {
				visitor(statement->right());
				return;
			}

			auto const* right = dryad::node_try_cast<FlatValue>(statement->right());
			if (!right) {
				using enum Codegen::scope_execution_type;
				bool is_iterative = is_iterative_scope(Effect, current_scope, left->value());
				if (is_iterative) {}
				is_prepping_args = is_scope_for(Effect, current_scope, left->value());
				visitor(right);
				if (is_iterative) {}
			} else if (ranges::equal(right->value().view(), "yes"sv, ichar_equals)) {
				// TODO: insert vic2 bytecode scope object address here
				// TODO: calls a builtin for Victoria 2 effects?
			} else if (!ranges::equal(right->value().view(), "no"sv, ichar_equals)) {
				// TODO: single argument execution
			}
		},
		[&](FlatValue* value) {
			// TODO: handle right side
		},
		[&](dryad::traverse_event ev, ListValue* value) {
			if (is_prepping_args && ev == dryad::traverse_event::exit) {
				// TODO: arguments have been prepared, call effect function
				is_prepping_args = false;
			} else if (!is_prepping_args && ev == dryad::traverse_event::enter) {
				// TODO: this is a scope
			}
		}
	);
}

void Codegen::generate_condition_from(Codegen::scope_type type, Node* node) {
	bool is_prepping_args = false;
	Codegen::scope_type current_scope = type;
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
				bool is_iterative = is_iterative_scope(Trigger, current_scope, left->value());
				if (is_iterative) {}
				is_prepping_args = is_scope_for(Trigger, current_scope, left->value());
				visitor(right);
				if (is_iterative) {}
			} else if (ranges::equal(right->value().view(), "yes"sv, ichar_equals)) {
				// TODO: insert vic2 bytecode scope object address here
				// TODO: calls a builtin for Victoria 2 triggers?
			} else if (!ranges::equal(right->value().view(), "no"sv, ichar_equals)) {
				// TODO: single argument execution
			}
		},
		[&](FlatValue* value) {
			// TODO: handle right side
		}
	);
}
