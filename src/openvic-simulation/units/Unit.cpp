#include "Unit.hpp"
#include <cstdint>
#include <map>
#include "types/IdentifierRegistry.hpp"
#include "types/fixed_point/FixedPoint.hpp"
#include <sys/types.h>

#define SUPER_ARGS icon, sprite, active, type, floating_flag, priority, max_strength, \
					default_organisation, maximum_speed, weighted_value, build_time, build_cost, supply_consumption, \
					supply_cost, supply_consumption_score
#define FOR_SUPER(cat) identifier, cat, SUPER_ARGS


using namespace OpenVic;

Unit::Unit(std::string_view identifier, UnitCategory category, UNIT_PARAMS) : HasIdentifier { identifier },
	category { category }, icon { icon }, sprite { sprite }, active { active }, type { type },
	floating_flag { floating_flag }, priority { priority }, max_strength { max_strength },
	default_organisation { default_organisation }, maximum_speed { maximum_speed }, weighted_value { weighted_value },
	build_time { build_time }, build_cost { build_cost }, supply_consumption { supply_consumption }, supply_cost { supply_cost },
	supply_consumption_score { supply_consumption_score } {}

Unit::icon_t Unit::get_icon() const {
	return icon;
}

Unit::sprite_t Unit::get_sprite() const {
	return sprite;
}

bool Unit::is_active() const {
	return active;
}

UnitType Unit::get_type() const {
	return type;
}

bool Unit::has_floating_flag() const {
	return floating_flag;
}

uint32_t Unit::get_priority() const {
	return priority;
}

uint32_t Unit::get_max_strength() const {
	return max_strength;
}

uint32_t Unit::get_default_organisation() const {
	return default_organisation;
}

uint32_t Unit::get_maximum_speed() const {
	return maximum_speed;
}

uint32_t Unit::get_build_time() const {
	return build_time;
}

fixed_point_t Unit::get_weighted_value() const {
	return weighted_value;
}

std::map<const Good*, fixed_point_t> Unit::get_build_cost() const {
	return build_cost;
}

fixed_point_t Unit::get_supply_consumption() const {
	return supply_consumption;
}

std::map<const Good*, fixed_point_t> Unit::get_supply_cost() const {
	return supply_cost;
}

uint32_t Unit::get_supply_consumption_score() const {
	return supply_consumption_score;
}

LandUnit::LandUnit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS) : Unit { FOR_SUPER(UnitCategory::LAND) },
	reconnaisance { reconnaisance }, attack { attack }, defence { defence }, discipline { discipline }, support { support },
	maneuver { maneuver }, siege { siege } {}

uint32_t LandUnit::get_reconnaisance() const {
	return reconnaisance;
}

uint32_t LandUnit::get_attack() const {
	return attack;
}

uint32_t LandUnit::get_defence() const {
	return defence;
}

uint32_t LandUnit::get_discipline() const {
	return discipline;
}

uint32_t LandUnit::get_support() const {
	return support;
}

uint32_t LandUnit::get_maneuver() const {
	return maneuver;
}

uint32_t LandUnit::get_siege() const {
	return siege;
}

NavalUnit::NavalUnit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS) : Unit { FOR_SUPER(UnitCategory::NAVAL) },
	naval_icon { naval_icon }, sail { sail }, transport { transport }, move_sound { move_sound }, select_sound { select_sound },
	colonial_points { colonial_points }, build_overseas { build_overseas }, min_port_level { min_port_level },
	limit_per_port { limit_per_port }, hull { hull }, gun_power { gun_power }, fire_range { fire_range }, evasion { evasion },
	torpedo_attack { torpedo_attack } {};

NavalUnit::icon_t NavalUnit::get_naval_icon() const {
	return naval_icon;
}

bool NavalUnit::can_sail() const {
	return sail;
}

bool NavalUnit::is_transport() const {
	return transport;
}

NavalUnit::sound_t NavalUnit::get_move_sound() const {
	return move_sound;
}

NavalUnit::sound_t NavalUnit::get_select_sound() const {
	return select_sound;
}

uint32_t NavalUnit::get_colonial_points() const {
	return colonial_points;
}

bool NavalUnit::can_build_overseas() const {
	return build_overseas;
}

uint32_t NavalUnit::get_min_port_level() const {
	return min_port_level;
}

int32_t NavalUnit::get_limit_per_port() const {
	return limit_per_port;
}

uint32_t NavalUnit::get_hull() const {
	return hull;
}

uint32_t NavalUnit::get_gun_power() const {
	return gun_power;
}

fixed_point_t NavalUnit::get_fire_range() const {
	return fire_range;
}

uint32_t NavalUnit::get_evasion() const {
	return evasion;
}

uint32_t NavalUnit::get_torpedo_attack() const {
	return torpedo_attack;
}

UnitManager::UnitManager() : units { "units " } {};

bool UnitManager::_check_superclass_parameters(const std::string_view identifier, UnitCategory cat, UNIT_PARAMS) {
	if (identifier.empty()) {
		Logger::error("Invalid religion identifier - empty!");
		return false;
	}

	//TODO check that icon and sprite exist

	return true;
}

bool UnitManager::add_land_unit(const std::string_view identifier, UnitCategory cat, UNIT_PARAMS, LAND_PARAMS) {
	if (!_check_superclass_parameters(FOR_SUPER(cat))) {
		return false;
	}

	return units.add_item(
		LandUnit { identifier, SUPER_ARGS, reconnaisance, attack, defence, discipline, support, maneuver, siege }
	);
}

bool UnitManager::add_naval_unit(const std::string_view identifier, UnitCategory cat, UNIT_PARAMS, NAVY_PARAMS) {
	if (!_check_superclass_parameters(FOR_SUPER(cat))) {
		return false;
	}

	//TODO: check that icon and sounds exist

	return units.add_item(
		NavalUnit { identifier, SUPER_ARGS, naval_icon, sail, transport, move_sound, select_sound, colonial_points, build_overseas, min_port_level, limit_per_port, hull, gun_power, fire_range, evasion, torpedo_attack }
	);
}