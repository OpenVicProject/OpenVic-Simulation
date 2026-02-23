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

std::string_view Context::get_identifier() const {
	return std::visit([](auto const* p) -> std::string_view {
		if (!p) return "";

		using T = std::decay_t<decltype(p)>;

		if constexpr (std::is_same_v<T, CountryInstance const*>) {
			return p->get_identifier();
		}
		else if constexpr (std::is_same_v<T, ProvinceInstance const*>) {
			return p->get_identifier();
		}
		else if constexpr (std::is_same_v<T, State const*>) {
			return p->get_identifier();
		}
		else if constexpr (std::is_same_v<T, Pop const*>) {
			return "";
		}

		return "";
	}, ptr);
}

bool Context::evaluate_leaf(ConditionNode const& node) const {
	std::string_view const id = node.get_condition()->get_identifier();

	return std::visit(
		[&](auto const* p) -> bool {
			if (!p) {
				return false;
			}

			using T = std::decay_t<decltype(*p)>;

			if constexpr (std::is_same_v<T, CountryInstance>) {
				// TODO: https://vic2.paradoxwikis.com/List_of_conditions#Country_Scope

				if (id == "ai") {
					bool expected = std::get<bool>(node.get_value());
					return p->is_ai() == expected;
				}
				if (id == "average_consciousness") {
					fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
					return p->get_average_consciousness() >= expected;
				}
				if (id == "average_militancy") {
					fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
					return p->get_average_militancy() >= expected;
				}
				if (id == "badboy") {
					fixed_point_t expected_ratio = std::get<fixed_point_t>(node.get_value());
					return p->get_infamy_untracked() >= (expected_ratio * fixed_point_t(25));
				}
				if (id == "civilized") {
					bool expected = std::get<bool>(node.get_value());
					return p->is_civilised() == expected;
				}
				if (id == "colonial_nation") {
					bool expected = std::get<bool>(node.get_value());
					bool has_colonies = false;
					for (State const* state : p->get_states()) {
						if (state && state->is_colonial_state()) {
							has_colonies = true;
							break;
						}
					}
					return has_colonies == expected;
				}
				if (id == "exists") {
					bool expected = std::get<bool>(node.get_value());
					return p->exists() == expected;
				}
				if (id == "industrial_score") {
					fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
					return p->get_industrial_power_untracked() >= expected;
				}
				if (id == "is_disarmed") {
					bool expected = std::get<bool>(node.get_value());
					return p->is_disarmed() == expected;
				}
				if (id == "is_greater_power") {
					bool expected = std::get<bool>(node.get_value());
					return p->is_great_power() == expected;
				}
				if (id == "is_mobilised") {
					bool expected = std::get<bool>(node.get_value());
					return p->is_mobilised() == expected;
				}
				if (id == "is_secondary_power") {
					bool expected = std::get<bool>(node.get_value());
					return p->is_secondary_power() == expected;
				}
				if (id == "num_of_cities") {
					uint64_t expected = std::get<uint64_t>(node.get_value());
					return p->get_owned_provinces().size() >= expected;
				}
				if (id == "num_of_ports") {
					uint64_t expected = std::get<uint64_t>(node.get_value());
					return p->get_port_count() >= expected;
				}
				if (id == "number_of_states") {
					uint64_t expected = std::get<uint64_t>(node.get_value());
					return p->get_states().size() >= expected;
				}
				if (id == "prestige") {
					fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
					return p->get_prestige_untracked() >= expected;
				}
				if (id == "plurality") {
					fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
					return p->get_plurality_untracked() >= expected;
				}
				if (id == "total_amount_of_ships") {
					uint64_t expected = std::get<uint64_t>(node.get_value());
					return p->get_ship_count() >= expected;
				}
				if (id == "rank") {
					uint64_t expected = std::get<uint64_t>(node.get_value());
					return p->get_total_rank() >= expected;
				}
				if (id == "tag") {
					memory::string const& expected = std::get<memory::string>(node.get_value());
					return p->get_identifier() == expected;
				}
				if (id == "war") {
					bool expected = std::get<bool>(node.get_value());
					return p->is_at_war() == expected;
				}
				if (id == "war_exhaustion") {
					fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
					return p->get_war_exhaustion() >= expected;
				}

				spdlog::warn_s("Condition {} not implemented for Country scope", id);
				return false;
			}
			else if constexpr (std::is_same_v<T, State>) {
				// No state conditions according to wiki?
				spdlog::warn_s("Condition {} not implemented for State scope", id);
				return false;
			}
			else if constexpr (std::is_same_v<T, ProvinceInstance>) {
				// TODO: https://vic2.paradoxwikis.com/List_of_conditions#Province_Scope
				spdlog::warn_s("Condition {} not implemented for Province scope", id);
				return false;
			}
			else if constexpr (std::is_same_v<T, Pop>) {
				// TODO: https://vic2.paradoxwikis.com/List_of_conditions#Pop_Scope
				if (id == "consciousness") {
					fixed_point_t expected = std::get<fixed_point_t>(node.get_value());
					return p->get_consciousness() >= expected;
				}
				spdlog::warn_s("Condition {} not implemented for Pop scope", id);
				return false;
			}

			return false;
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
			else if (condition_id == "any_core_province" || condition_id == "all_core") {
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
				auto const* owner = province->get_owner();
				if (owner == nullptr) {
					return std::nullopt;
				}
				return make_child(owner);
			}
			if (condition_id == "controller") {
				auto const* controller = province->get_controller();
				if (controller == nullptr) {
					return std::nullopt;
				}
				return make_child(controller);
			}
		}
	}

	return std::nullopt;
}
