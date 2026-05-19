#pragma once

#include "openvic-simulation/core/portable/ForwardableSpan.hpp"

namespace OpenVic {
	struct GameRulesManager;
	struct GoodDefinition;
	struct GoodInstanceManager;
	struct CountryInstance;
	struct MapInstance;
	struct MilitaryDefines;
	struct ModifierEffectCache;
	struct PopsDefines;
	struct ProductionTypeManager;
	struct StaticModifierCache;
	struct Strata;
	
	struct ThreadDeps {
		GameRulesManager const& game_rules_manager;
		GoodInstanceManager const& good_instance_manager;
		MapInstance const& map_instance;
		MilitaryDefines const& military_defines;
		ModifierEffectCache const& modifier_effect_cache;
		PopsDefines const& pop_defines;
		ProductionTypeManager const& production_type_manager;
		StaticModifierCache const& static_modifier_cache;
		forwardable_span<const CountryInstance> country_keys;
		forwardable_span<const GoodDefinition> good_keys;
		forwardable_span<const Strata> strata_keys;
	};
}