#pragma once

#include "openvic-simulation/core/portable/ForwardableSpan.hpp"

namespace OpenVic {
	struct BuildingTypeManager;
	struct GameRulesManager;
	struct PopsAggregateDeps;
	struct PopType;
	struct ResourceGatheringOperationDeps;
	struct Strata;

	struct ProvinceInstanceDeps {
		BuildingTypeManager const& building_type_manager;
		GameRulesManager const& game_rules_manager;
		PopsAggregateDeps const& pops_aggregate_deps;
		forwardable_span<const PopType> pop_types;
		ResourceGatheringOperationDeps const& rgo_deps;
		forwardable_span<const Strata> stratas;
	};
}