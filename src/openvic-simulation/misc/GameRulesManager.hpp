#pragma once

#include "openvic-simulation/utility/Getters.hpp"
namespace OpenVic {
	struct GameRulesManager {
	private:
		bool PROPERTY_RW(use_simple_farm_mine_logic, false);
		// if changed during a session, call on_use_exponential_price_changes_changed for each GoodInstance.
		bool PROPERTY_RW(use_exponential_price_changes, false);

	public:
		constexpr bool get_use_optimal_pricing() const {
			return use_exponential_price_changes;
		}
	};
}