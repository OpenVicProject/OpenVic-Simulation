#include "Unit.hpp"

#define UNIT_ARGS icon, sprite, active, type, floating_flag, priority, max_strength, \
					default_organisation, maximum_speed, weighted_value, build_time, build_cost, supply_consumption, \
					supply_cost
#define LAND_ARGS reconnaissance, attack, defence, discipline, support, maneuver, siege
#define NAVY_ARGS naval_icon, sail, transport, capital, move_sound, select_sound, colonial_points, build_overseas, min_port_level, \
					limit_per_port, supply_consumption_score, hull, gun_power, fire_range, evasion, torpedo_attack

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Unit::Unit(std::string_view identifier, std::string_view category, UNIT_PARAMS) : HasIdentifier { identifier },
	icon { icon }, category { category }, sprite { sprite }, active { active }, type { type },
	floating_flag { floating_flag }, priority { priority }, max_strength { max_strength },
	default_organisation { default_organisation }, maximum_speed { maximum_speed }, weighted_value { weighted_value },
	build_time { build_time }, build_cost { build_cost }, supply_consumption { supply_consumption }, supply_cost { supply_cost } {}

Unit::icon_t Unit::get_icon() const {
	return icon;
}

Unit::sprite_t Unit::get_sprite() const {
	return sprite;
}

bool Unit::is_active() const {
	return active;
}

std::string_view Unit::get_type() const {
	return type;
}

bool Unit::has_floating_flag() const {
	return floating_flag;
}

uint32_t Unit::get_priority() const {
	return priority;
}

fixed_point_t Unit::get_max_strength() const {
	return max_strength;
}

fixed_point_t Unit::get_default_organisation() const {
	return default_organisation;
}

fixed_point_t Unit::get_maximum_speed() const {
	return maximum_speed;
}

Timespan Unit::get_build_time() const {
	return build_time;
}

fixed_point_t Unit::get_weighted_value() const {
	return weighted_value;
}

std::map<const Good*, fixed_point_t> const& Unit::get_build_cost() const {
	return build_cost;
}

fixed_point_t Unit::get_supply_consumption() const {
	return supply_consumption;
}

std::map<const Good*, fixed_point_t> const& Unit::get_supply_cost() const {
	return supply_cost;
}

LandUnit::LandUnit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS) : Unit { identifier, "land", UNIT_ARGS },
	reconnaissance { reconnaissance }, attack { attack }, defence { defence }, discipline { discipline }, support { support },
	maneuver { maneuver }, siege { siege } {}

fixed_point_t LandUnit::get_reconnaissance() const {
	return reconnaissance;
}

fixed_point_t LandUnit::get_attack() const {
	return attack;
}

fixed_point_t LandUnit::get_defence() const {
	return defence;
}

fixed_point_t LandUnit::get_discipline() const {
	return discipline;
}

fixed_point_t LandUnit::get_support() const {
	return support;
}

fixed_point_t LandUnit::get_maneuver() const {
	return maneuver;
}

fixed_point_t LandUnit::get_siege() const {
	return siege;
}

NavalUnit::NavalUnit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS) : Unit { identifier, "naval", UNIT_ARGS },
	naval_icon { naval_icon }, sail { sail }, transport { transport }, capital { capital }, move_sound { move_sound },
	select_sound { select_sound }, colonial_points { colonial_points }, build_overseas { build_overseas },
	min_port_level { min_port_level }, limit_per_port { limit_per_port }, supply_consumption_score { supply_consumption_score },
	hull { hull }, gun_power { gun_power }, fire_range { fire_range }, evasion { evasion }, torpedo_attack { torpedo_attack } {};

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

fixed_point_t NavalUnit::get_colonial_points() const {
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

fixed_point_t NavalUnit::get_supply_consumption_score() const {
	return supply_consumption_score;
}

fixed_point_t NavalUnit::get_hull() const {
	return hull;
}

fixed_point_t NavalUnit::get_gun_power() const {
	return gun_power;
}

fixed_point_t NavalUnit::get_fire_range() const {
	return fire_range;
}

fixed_point_t NavalUnit::get_evasion() const {
	return evasion;
}

fixed_point_t NavalUnit::get_torpedo_attack() const {
	return torpedo_attack;
}

UnitManager::UnitManager() : units { "units" } {}

bool UnitManager::_check_shared_parameters(std::string_view identifier, UNIT_PARAMS) {
	if (identifier.empty()) {
		Logger::error("Invalid religion identifier - empty!");
		return false;
	}

	if (sprite.empty()) {
		Logger::error("Invalid sprite identifier - empty!");
		return false;
	}

	if (type.empty()) {
		Logger::error("Invalid unit type - empty!");
		return false;
	}

	//TODO check that icon and sprite exist

	return true;
}

bool UnitManager::add_land_unit(std::string_view identifier, UNIT_PARAMS, LAND_PARAMS) {
	if (!_check_shared_parameters(identifier, UNIT_ARGS)) {
		return false;
	}

	return units.add_item(LandUnit { identifier, UNIT_ARGS, LAND_ARGS });
}

bool UnitManager::add_naval_unit(std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS) {
	if (!_check_shared_parameters(identifier, UNIT_ARGS)) {
		return false;
	}

	//TODO: check that icon and sounds exist

	return units.add_item(NavalUnit { identifier, UNIT_ARGS, NAVY_ARGS });
}

bool UnitManager::load_unit_file(GoodManager const& good_manager, ast::NodeCPtr root) {
	return NodeTools::expect_dictionary([this, &good_manager](std::string_view key, ast::NodeCPtr value) -> bool {
		Unit::icon_t icon;
		std::string_view category, type;
		Unit::sprite_t sprite;
		bool active = true, floating_flag;
		uint32_t priority, build_time_days;
		fixed_point_t maximum_speed, max_strength, default_organisation, weighted_value, supply_consumption;
		std::map<const Good*, fixed_point_t> build_cost, supply_cost;

		//shared
		bool ret = expect_dictionary_keys(ALLOW_OTHER_KEYS,
			"icon", ONE_EXACTLY, expect_uint(assign_variable_callback_uint("unit icon", icon)),
			"type", ONE_EXACTLY, expect_identifier(assign_variable_callback(category)),
			"sprite", ONE_EXACTLY, expect_identifier(assign_variable_callback(sprite)),
			"active", ZERO_OR_ONE, expect_bool(assign_variable_callback(active)),
			"unit_type", ONE_EXACTLY, expect_identifier(assign_variable_callback(type)),
			"floating_flag", ONE_EXACTLY, expect_bool(assign_variable_callback(floating_flag)),
			"priority", ONE_EXACTLY, expect_uint(assign_variable_callback_uint("unit priority", priority)),
			"max_strength", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_strength)),
			"default_organisation", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(default_organisation)),
			"maximum_speed", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(maximum_speed)),
			"weighted_value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(weighted_value)),
			"build_time", ONE_EXACTLY, expect_uint(assign_variable_callback_uint("unit build time", build_time_days)),
			"build_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(assign_variable_callback(build_cost)),
			"supply_consumption", ONE_EXACTLY, expect_fixed_point(move_variable_callback(supply_consumption)),
			"supply_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(move_variable_callback(supply_cost))
		)(value);

		Timespan build_time = { build_time_days };

		if (category == "land") {
			fixed_point_t reconnaissance, attack, defence, discipline, support, maneuver, siege;

			ret &= expect_dictionary_keys(ALLOW_OTHER_KEYS,
				"reconnaissance", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(reconnaissance)),
				"attack", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(attack)),
				"defence", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(defence)),
				"discipline", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(discipline)),
				"support", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(support)),
				"maneuver", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(maneuver)),
				"siege", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(siege))
			)(value);

			ret &= add_land_unit(key, UNIT_ARGS, LAND_ARGS);

			return ret;
		} else if (category == "naval") {
			Unit::icon_t naval_icon;
			bool sail = false, transport = false, capital = false, build_overseas = false;
			Unit::sound_t move_sound, select_sound; //TODO defaults for both
			uint32_t min_port_level;
			int32_t limit_per_port;
			fixed_point_t fire_range, evasion, supply_consumption_score, hull, gun_power, colonial_points = 0, torpedo_attack = 0;

			ret &= expect_dictionary_keys(ALLOW_OTHER_KEYS,
				"naval_icon", ONE_EXACTLY, expect_uint(assign_variable_callback_uint("unit naval icon", naval_icon)),
				"sail", ZERO_OR_ONE, expect_bool(assign_variable_callback(sail)),
				"transport", ZERO_OR_ONE, expect_bool(assign_variable_callback(transport)),
				"capital", ZERO_OR_ONE, expect_bool(assign_variable_callback(capital)),
				"move_sound", ZERO_OR_ONE, expect_identifier(assign_variable_callback(move_sound)),
				"select_sound", ZERO_OR_ONE, expect_identifier(assign_variable_callback(select_sound)),
				"colonial_points", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(colonial_points)),
				"can_build_overseas", ZERO_OR_ONE, expect_bool(assign_variable_callback(build_overseas)),
				"min_port_level", ONE_EXACTLY, expect_uint(assign_variable_callback_uint("unit min port level", min_port_level)),
				"limit_per_port", ONE_EXACTLY, expect_int(assign_variable_callback_int("unit limit per port", limit_per_port)),
				"supply_consumption_score", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(supply_consumption_score)),
				"hull", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(hull)),
				"gun_power", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(gun_power)),
				"fire_range", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(fire_range)),
				"evasion", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(evasion)),
				"torpedo_attack", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(torpedo_attack))
			)(value);

			ret &= add_naval_unit(key, UNIT_ARGS, NAVY_ARGS);

			return ret;
		} else return false;
	})(root);
}