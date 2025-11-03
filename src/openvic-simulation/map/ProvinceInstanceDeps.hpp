#pragma once

#include "openvic-simulation/utility/ForwardableSpan.hpp"

namespace OpenVic {
	struct BuildingTypeManager;
	struct GameRulesManager;
	struct Ideology;
	struct PopType;
	struct ResourceGatheringOperationDeps;
	struct Strata;

	struct ProvinceInstanceDeps {
		BuildingTypeManager const& building_type_manager;
		GameRulesManager const& game_rules_manager;
		utility::forwardable_span<const Ideology> ideology_keys;
		utility::forwardable_span<const PopType> pop_type_keys;
		ResourceGatheringOperationDeps const& rgo_deps;
		utility::forwardable_span<const Strata> strata_keys;
	};
}