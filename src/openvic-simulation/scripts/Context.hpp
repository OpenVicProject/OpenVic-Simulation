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

	struct Context {
		std::variant<
			CountryInstance const*,
			ProvinceInstance const*,
			State const*,
			Pop const*> ptr;

		Context const* this_scope = nullptr;
		Context const* from_scope = nullptr;

		Context(CountryInstance const* p) : ptr(p), this_scope(this) {}
		Context(ProvinceInstance const* p) : ptr(p), this_scope(this) {}
		Context(State const* p) : ptr(p), this_scope(this) {}
		Context(Pop const* p) : ptr(p), this_scope(this) {}

		Context(
			auto* p,
			Context const* this_ctx,
			Context const* from_ctx
		) : ptr(p), this_scope(this_ctx), from_scope(from_ctx) {}

		scope_type_t get_scope_type() const;

		std::string_view get_identifier() const;

		bool evaluate_leaf(ConditionNode const& node) const;

		std::vector<Context> get_sub_contexts(std::string_view condition_id, scope_type_t target) const;

		std::optional<Context> get_redirect_context(std::string_view condition_id, scope_type_t target) const;

		Context make_child(auto* p) const {
			return Context(p, this->this_scope, this);
		}
	};
}