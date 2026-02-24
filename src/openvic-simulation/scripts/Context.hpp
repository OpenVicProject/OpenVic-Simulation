#pragma once

#include <optional>
#include <variant>

#include "Condition.hpp"
#include "openvic-simulation/utility/Containers.hpp"

namespace OpenVic {
	struct CountryInstance;
	struct ProvinceInstance;
	struct State;
	struct Pop;
	struct DefinitionManager;
	struct InstanceManager;

	using ScopePointer = std::variant<
		std::monostate, // Represents a null/empty scope
		CountryInstance const*,
		ProvinceInstance const*,
		State const*,
		Pop const*>;

	struct Context {
		ScopePointer ptr;

	private:
		scope_type_t PROPERTY(scope_type);
	public:

		DefinitionManager const& definition_manager;
		InstanceManager const& instance_manager;

		ScopePointer this_scope;
		ScopePointer from_scope;

		Context(ScopePointer new_ptr, DefinitionManager const& new_definition_manager, InstanceManager const& new_instance_manager)
			: Context(new_ptr, new_definition_manager, new_instance_manager, new_ptr, std::monostate{}) {}

		Context(
			ScopePointer p,
			DefinitionManager const& new_definition_manager,
			InstanceManager const& new_instance_manager,
			ScopePointer new_this_scope,
			ScopePointer new_from_scope
		) : ptr(p),
			scope_type(determine_scope_type(p)),
			definition_manager(new_definition_manager),
			instance_manager(new_instance_manager),
			this_scope(new_this_scope),
			from_scope(new_from_scope) {}

		std::string_view get_identifier() const;

		bool evaluate_leaf(ConditionNode const& node) const;

		memory::vector<Context> get_sub_contexts(std::string_view condition_id, scope_type_t target) const;

		std::optional<Context> get_redirect_context(std::string_view condition_id, scope_type_t target) const;

		Context make_child(ScopePointer p) const {
			return Context(
				p,
				definition_manager,
				instance_manager,
				this->this_scope,
				this->from_scope
			);
		}
	private:

		static scope_type_t determine_scope_type(ScopePointer const& p) {
			return std::visit([](auto const& v) -> scope_type_t {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, std::monostate>) {
					return scope_type_t::NO_SCOPE;
				} else if (!v) {
					return scope_type_t::NO_SCOPE;
				} else if constexpr (std::is_same_v<T, CountryInstance const*>) {
					return scope_type_t::COUNTRY;
				} else if constexpr (std::is_same_v<T, ProvinceInstance const*>) {
					return scope_type_t::PROVINCE;
				} else if constexpr (std::is_same_v<T, State const*>) {
					return scope_type_t::STATE;
				} else if constexpr (std::is_same_v<T, Pop const*>) {
					return scope_type_t::POP;
				}
			}, p);
		}
	};
}