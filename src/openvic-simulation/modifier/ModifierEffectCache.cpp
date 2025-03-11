#include "ModifierEffectCache.hpp"

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/pop/PopType.hpp"
#include "openvic-simulation/research/Technology.hpp"

using namespace OpenVic;

// This is in a CPP file rather than all in the header to
// minimise the number of includes needed in the header.

ModifierEffectCache::ModifierEffectCache()
  : /* BuildingType Effects */
	building_type_effects { nullptr },

	/* GoodDefinition Effects */
	good_effects { nullptr },

	/* UnitType Effects */
	regiment_type_effects { nullptr },
	ship_type_effects { nullptr },
	unit_terrain_effects { nullptr },

	/* Rebel Effects */
	rebel_org_gain_effects { nullptr },

	/* Pop Effects */
	strata_effects { nullptr },

	/* Technology Effects */
	research_bonus_effects { nullptr } {}
