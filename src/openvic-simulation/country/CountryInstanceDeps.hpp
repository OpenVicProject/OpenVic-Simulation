#pragma once

#include "openvic-simulation/core/stl/containers/TypedSpan.hpp"
#include "openvic-simulation/population/PopsAggregateDeps.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

#if defined(__APPLE__)
#include "openvic-simulation/military/UnitType.hpp"
#endif

namespace OpenVic {
	struct BuildingType;
	struct CountryDefines;
	struct CountryRelationManager;
	struct Crime;
	struct DiplomacyDefines;
	struct EconomyDefines;
	struct Ideology;
	struct Invention;
	struct GameRulesManager;
	struct GoodInstance;
	struct GoodInstanceManager;
	struct GovernmentType;
	struct MarketInstance;
	struct MilitaryDefines;
	struct ModifierEffectCache;
	struct PopType;
	struct ReformGroup;
	struct Strata;
	struct UnitTypeManager;

	struct CountryInstanceDeps {
		CountryDefines const& country_defines;
		CountryRelationManager& country_relations_manager;
		TypedSpan<crime_index_t, const Crime> crimes;
		Date fallback_date_for_never_completing_research;
		DiplomacyDefines const& diplomacy_defines;
		EconomyDefines const& economy_defines;
		TypedSpan<ideology_index_t, const Ideology> ideologies;
		TypedSpan<invention_index_t, const Invention> inventions;
		GameRulesManager const& game_rules_manager;
		TypedSpan<good_index_t, const GoodInstance> good_instances;
		GoodInstanceManager& good_instance_manager;
		TypedSpan<government_type_index_t, const GovernmentType> government_types;
		MarketInstance& market_instance;
		MilitaryDefines const& military_defines;
		ModifierEffectCache const& modifier_effect_cache;
		PopsAggregateDeps pops_aggregate_deps;
		TypedSpan<reform_group_index_t, const ReformGroup> reform_groups;
		TypedSpan<ship_type_index_t, const ShipType> ship_types;
		TypedSpan<strata_index_t, const Strata> stratas;
		UnitTypeManager const& unit_type_manager;
	};
}