#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "openvic-simulation/ecs/ComponentTypeID.hpp"
#include "openvic-simulation/ecs_rgo/Types.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

// World singletons used by the parallel-reference RGO. Singletons are the right home for
// global simulation state that doesn't belong on a particular entity (see ECS.md / World.hpp's
// "set_singleton" doc): registries, market price tables, defines.

namespace OpenVic::ecs_rgo {

	// Mirrors the legacy ProductionType — flat POD, no inheritance, no manager. Owned by the
	// RgoProductionTypeRegistry singleton; indexed by production_type_idx_t.
	struct ProductionTypeDef {
		TemplateType template_type = TemplateType::Rgo;
		std::optional<Job> owner;
		std::vector<Job> jobs;
		good_idx_t output_good_idx = INVALID_IDX;
		pop_size_t base_workforce_size = 0;
		fixed_point_t base_output_quantity {};
		bool is_farm = false;
		bool is_mine = false;
	};

	// Lookup table for ProductionTypeDef by production_type_idx. Stored in a singleton so any
	// system can dereference an index without going through a manager — see ECS.md's
	// "no thin wrappers around World" rule.
	struct RgoProductionTypeRegistry {
		std::vector<ProductionTypeDef> production_types;
	};

	// Stub of the legacy MarketInstance::place_market_sell_order callback path. Every good_idx
	// has a fixed unit price for the fixture; Stage-4's resolver uses
	// `money_gained = unit_price[good_idx] * quantity_sold`.
	struct RgoMarketPriceTable {
		std::vector<fixed_point_t> unit_price; // indexed by good_idx_t
	};

	// Parsed-but-unused vanilla defines. Included so the singleton surface mirrors the legacy
	// parser surface — data is reachable from a System but no current code path reads it.
	struct RgoEconomyDefines {
		fixed_point_t supply_demand_factor_hire_hi {};
		fixed_point_t supply_demand_factor_hire_lo {};
		fixed_point_t supply_demand_factor_fire {};
	};

	// GameRulesManager equivalent — toggles the farm-vs-mine classification quirk (see
	// resolve_farm_mine_classification in RgoMath.hpp).
	struct RgoGameRules {
		bool use_simple_farm_mine_logic = false;
	};
}

ECS_COMPONENT(OpenVic::ecs_rgo::RgoProductionTypeRegistry, "OpenVic::ecs_rgo::RgoProductionTypeRegistry")
ECS_COMPONENT(OpenVic::ecs_rgo::RgoMarketPriceTable, "OpenVic::ecs_rgo::RgoMarketPriceTable")
ECS_COMPONENT(OpenVic::ecs_rgo::RgoEconomyDefines, "OpenVic::ecs_rgo::RgoEconomyDefines")
ECS_COMPONENT(OpenVic::ecs_rgo::RgoGameRules, "OpenVic::ecs_rgo::RgoGameRules")
