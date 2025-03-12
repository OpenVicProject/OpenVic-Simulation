#include "ModifierEffectCache.hpp"

#include "openvic-simulation/economy/BuildingType.hpp" // IWYU pragma: keep
#include "openvic-simulation/economy/GoodDefinition.hpp" // IWYU pragma: keep
#include "openvic-simulation/map/TerrainType.hpp" // IWYU pragma: keep
#include "openvic-simulation/politics/Rebel.hpp" // IWYU pragma: keep
#include "openvic-simulation/research/Technology.hpp" // IWYU pragma: keep

using namespace OpenVic;

ModifierEffectCache::building_type_effects_t::building_type_effects_t() {}
ModifierEffectCache::good_effects_t::good_effects_t() {}
ModifierEffectCache::unit_type_effects_t::unit_type_effects_t() {}
ModifierEffectCache::regiment_type_effects_t::regiment_type_effects_t() {}
ModifierEffectCache::ship_type_effects_t::ship_type_effects_t() {}
ModifierEffectCache::unit_terrain_effects_t::unit_terrain_effects_t() {}
ModifierEffectCache::strata_effects_t::strata_effects_t() {}

ModifierEffectCache::ModifierEffectCache()
	: /* BuildingType Effects */
	  building_type_effects { nullptr },

	  /* GoodDefinition Effects */
	  good_effects { nullptr },

	  regiment_type_effects { nullptr },

	  ship_type_effects { nullptr },
	  unit_terrain_effects { nullptr },

	  /* Rebel Effects */
	  rebel_org_gain_effects { nullptr },

	  /* Pop Effects */
	  strata_effects { nullptr },

	  /* Technology Effects */
	  research_bonus_effects { nullptr } {}
