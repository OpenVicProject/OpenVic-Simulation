#pragma once

#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
	struct GameRulesManager;
	struct GoodInstanceManager;
	struct MapInstance;
	struct MilitaryDefines;
	struct ModifierEffectCache;
	struct PopsDefines;
	struct ProductionTypeManager;
	struct StaticModifierCache;
	
	struct ThreadDeps {
		GameRulesManager const& game_rules_manager;
		GoodInstanceManager const& good_instance_manager;
		MapInstance const& map_instance;
		MilitaryDefines const& military_defines;
		ModifierEffectCache const& modifier_effect_cache;
		PopsDefines const& pop_defines;
		ProductionTypeManager const& production_type_manager;
		StaticModifierCache const& static_modifier_cache;
		country_index_t country_count;
		good_index_t good_count;
		strata_index_t strata_count;
	};
}