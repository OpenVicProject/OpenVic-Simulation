#include "Context.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/population/Pop.hpp"

using namespace OpenVic;
scope_type_t Context::get_scope_type() const {
	return std::visit(
		[](auto* p) -> scope_type_t {
			using T = std::decay_t<decltype(*p)>;

			if constexpr (std::is_same_v<T, CountryInstance>) {
				return scope_type_t::COUNTRY;
			} else if constexpr (std::is_same_v<T, ProvinceInstance>) {
				return scope_type_t::PROVINCE;
			} else if constexpr (std::is_same_v<T, State>) {
				return scope_type_t::STATE;
			} else if constexpr (std::is_same_v<T, Pop>) {
				return scope_type_t::POP;
			}

			return scope_type_t::NO_SCOPE;
		},
		ptr
	);
}

bool Context::evaluate_leaf(ConditionNode const& node) const {
	return std::visit(
		[&](auto* p) -> bool {
			return p->evaluate_leaf(node);
		},
		ptr
	);
}

std::vector<Context> Context::get_sub_contexts(std::string_view condition_id, scope_type_t target) const {
	std::vector<Context> result;

	if (std::holds_alternative<CountryInstance const*>(ptr)) {
		CountryInstance const* country = std::get<CountryInstance const*>(ptr);

		if (target == scope_type_t::PROVINCE) {
			if (condition_id == "any_owned_province") {
				for (ProvinceInstance const* prov : country->get_owned_provinces()) {
					result.emplace_back(make_child(prov));
				}
			}
			else if (condition_id == "any_controlled_province") {
				for (ProvinceInstance const* prov : country->get_controlled_provinces()) {
					result.emplace_back(make_child(prov));
				}
			}
			else if (condition_id == "any_core" || condition_id == "all_core") {
				for (ProvinceInstance const* prov : country->get_core_provinces()) {
					result.emplace_back(make_child(prov));
				}
			}
		}
		else if (target == scope_type_t::STATE) {
			if (condition_id == "any_state") {
				for (State const* state : country->get_states()) {
					result.emplace_back(make_child(state));
				}
			}
		}
	}
	else if (std::holds_alternative<ProvinceInstance const*>(ptr)) {
		ProvinceInstance const* province = std::get<ProvinceInstance const*>(ptr);

		// TODO
	}

	return result;
}

std::optional<Context> Context::get_redirect_context(std::string_view condition_id, scope_type_t target) const {
	if (target == scope_type_t::THIS) {
		if (this_scope != nullptr) {
			return *this_scope;
		}
		return std::nullopt;
	}

	if (target == scope_type_t::FROM) {
		if (from_scope != nullptr) {
			return *from_scope;
		}
		return std::nullopt;
	}

	if (std::holds_alternative<ProvinceInstance const*>(ptr)) {
		ProvinceInstance const* province = std::get<ProvinceInstance const*>(ptr);

		if (target == scope_type_t::COUNTRY) {
			if (condition_id == "owner") {
				return make_child(province->get_owner());
			}
			if (condition_id == "controller") {
				return make_child(province->get_controller());
			}
		}
	}

	return std::nullopt;
}
