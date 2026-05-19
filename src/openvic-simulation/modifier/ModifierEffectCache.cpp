#include "ModifierEffectCache.hpp"

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/politics/Rebel.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/research/Technology.hpp"

using namespace OpenVic;

ModifierEffectCache::building_type_effects_t const& ModifierEffectCache::get_building_type_effects(BuildingType const& key) const {
	return building_type_effects[key.index];
}
ModifierEffectCache::good_effects_t const& ModifierEffectCache::get_good_effects(GoodDefinition const& key) const {
	return good_effects[key.index];
}
ModifierEffectCache::regiment_type_effects_t const& ModifierEffectCache::get_regiment_type_effects(RegimentType const& key) const {
	return regiment_type_effects[key.index];
}
ModifierEffectCache::ship_type_effects_t const& ModifierEffectCache::get_ship_type_effects(ShipType const& key) const {
	return ship_type_effects[key.index];
}
ModifierEffectCache::unit_terrain_effects_t const& ModifierEffectCache::get_unit_terrain_effects(TerrainType const& key) const {
	return unit_terrain_effects[key.index];
}
ModifierEffect const* ModifierEffectCache::get_rebel_org_gain_effects(RebelType const& key) const {
	return rebel_org_gain_effects[key.index];
}
ModifierEffectCache::strata_effects_t const& ModifierEffectCache::get_strata_effects(Strata const& key) const {
	return strata_effects[key.index];
}
ModifierEffect const* ModifierEffectCache::get_research_bonus_effects(TechnologyFolder const& key) const {
	return research_bonus_effects[key.index];
}