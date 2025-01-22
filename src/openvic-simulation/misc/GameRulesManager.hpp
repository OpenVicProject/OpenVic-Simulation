#pragma once

#include "openvic-simulation/utility/Getters.hpp"
namespace OpenVic {
	struct GameRulesManager {
	private:
		bool PROPERTY_RW(use_simple_farm_mine_logic, false);
		bool PROPERTY_RW(display_rgo_eff_tech_throughput_effect, false);
	};
}