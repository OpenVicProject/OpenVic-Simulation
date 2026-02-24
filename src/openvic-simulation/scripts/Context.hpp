#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "Condition.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct ProvinceInstance;
	struct State;
	struct Pop;
	struct DefinitionManager;
	struct InstanceManager;

	struct Context {
		std::variant<
			CountryInstance const*,
			ProvinceInstance const*,
			State const*,
			Pop const*> ptr;

		DefinitionManager const& definition_manager;
		InstanceManager const& instance_manager;

		Context const* this_scope = nullptr;
		Context const* from_scope = nullptr;

		Context(CountryInstance const* new_ptr, DefinitionManager const& new_definition_manager, InstanceManager const& new_instance_manager)
			: Context(new_ptr, new_definition_manager, new_instance_manager, this, nullptr) {}
		Context(ProvinceInstance const* new_ptr, DefinitionManager const& new_definition_manager, InstanceManager const& new_instance_manager)
			: Context(new_ptr, new_definition_manager, new_instance_manager, this, nullptr) {}
		Context(State const* new_ptr, DefinitionManager const& new_definition_manager, InstanceManager const& new_instance_manager)
			: Context(new_ptr, new_definition_manager, new_instance_manager, this, nullptr) {}
		Context(Pop const* new_ptr, DefinitionManager const& new_definition_manager, InstanceManager const& new_instance_manager)
			: Context(new_ptr, new_definition_manager, new_instance_manager, this, nullptr) {}

		Context(
			auto* p,
			DefinitionManager const& new_definition_manager,
			InstanceManager const& new_instance_manager,
			Context const* new_this_scope,
			Context const* new_from_scope
		) : ptr(p),
			definition_manager(new_definition_manager),
			instance_manager(new_instance_manager),
			this_scope(new_this_scope),
			from_scope(new_from_scope) {}

		scope_type_t get_scope_type() const;

		std::string_view get_identifier() const;

		bool evaluate_leaf(ConditionNode const& node) const;

		std::vector<Context> get_sub_contexts(std::string_view condition_id, scope_type_t target) const;

		std::optional<Context> get_redirect_context(std::string_view condition_id, scope_type_t target) const;

		Context make_child(auto* p) const {
			return Context(p, definition_manager, instance_manager, this->this_scope, this);
		}
	};
}