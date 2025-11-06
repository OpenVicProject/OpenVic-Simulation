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
		utility::forwardable_span<const BuildingType> building_types;
		CountryDefines const& country_defines;
		CountryRelationManager& country_relations_manager;
		utility::forwardable_span<const Crime> crimes;
		Date fallback_date_for_never_completing_research;
		DiplomacyDefines const& diplomacy_defines;
		EconomyDefines const& economy_defines;
		utility::forwardable_span<const Invention> inventions;
		utility::forwardable_span<const Ideology> ideologies;
		GameRulesManager const& game_rules_manager;
		utility::forwardable_span<const GoodInstance> good_instances;
		GoodInstanceManager& good_instance_manager;
		utility::forwardable_span<const GovernmentType> government_types;
		MarketInstance& market_instance;
		MilitaryDefines const& military_defines;
		ModifierEffectCache const& modifier_effect_cache;
		utility::forwardable_span<const PopType> pop_types;
		utility::forwardable_span<const ReformGroup> reforms;
		utility::forwardable_span<const RegimentType> regiment_types;
		utility::forwardable_span<const ShipType> ship_types;
		utility::forwardable_span<const Strata> stratas;
		utility::forwardable_span<const Technology> technologies;
		UnitTypeManager const& unit_type_manager;
	};
}