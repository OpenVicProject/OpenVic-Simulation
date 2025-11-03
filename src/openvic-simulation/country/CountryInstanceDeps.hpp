#pragma once

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"
#include "openvic-simulation/utility/ForwardableSpan.hpp"

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
	struct Technology;
	struct UnitTypeManager;

	struct CountryInstanceDeps {
		utility::forwardable_span<const BuildingType> building_type_keys;
		CountryDefines const& country_defines;
		CountryRelationManager& country_relations_manager;
		utility::forwardable_span<const Crime> crime_keys;
		const Date fallback_date_for_never_completing_research;
		DiplomacyDefines const& diplomacy_defines;
		EconomyDefines const& economy_defines;
		utility::forwardable_span<const Invention> invention_keys;
		utility::forwardable_span<const Ideology> ideology_keys;
		GameRulesManager const& game_rules_manager;
		utility::forwardable_span<const GoodInstance> good_instances_keys;
		GoodInstanceManager& good_instance_manager;
		utility::forwardable_span<const GovernmentType> government_type_keys;
		MarketInstance& market_instance;
		MilitaryDefines const& military_defines;
		ModifierEffectCache const& modifier_effect_cache;
		utility::forwardable_span<const PopType> pop_type_keys;
		utility::forwardable_span<const ReformGroup> reform_keys;
		utility::forwardable_span<const RegimentType> regiment_type_unlock_levels_keys;
		utility::forwardable_span<const ShipType> ship_type_unlock_levels_keys;
		utility::forwardable_span<const Strata> strata_keys;
		utility::forwardable_span<const Technology> technology_keys;
		UnitTypeManager const& unit_type_manager;
	};
}