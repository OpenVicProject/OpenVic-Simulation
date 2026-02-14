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

		Context(CountryInstance const* p) : ptr(p) {}
		Context(ProvinceInstance const* p) : ptr(p) {}
		Context(State const* p) : ptr(p) {}
		Context(Pop const* p) : ptr(p) {}

		scope_type_t get_scope_type() const;

		bool evaluate_leaf(ConditionNode const& node) const;

		std::vector<Context> get_sub_contexts(scope_type_t target) const;

		std::optional<Context> get_redirect_context(scope_type_t target) const;
	};
}