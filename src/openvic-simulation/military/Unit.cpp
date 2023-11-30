#include "Unit.hpp"

#define UNIT_ARGS \
	icon, sprite, active, unit_type, floating_flag, priority, max_strength, default_organisation, maximum_speed, \
	weighted_value, move_sound, select_sound, build_time, std::move(build_cost), supply_consumption, \
	std::move(supply_cost)

#define LAND_ARGS \
	primary_culture, sprite_override, sprite_mount, sprite_mount_attach_node, reconnaissance, attack, defence, discipline, \
	support, maneuver, siege

#define NAVY_ARGS \
	naval_icon, sail, transport, capital, colonial_points, build_overseas, min_port_level, limit_per_port, \
	supply_consumption_score, hull, gun_power, fire_range, evasion, torpedo_attack

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Unit::Unit(
	std::string_view identifier, type_t type, UNIT_PARAMS
) : HasIdentifier { identifier }, icon { icon }, type { type }, sprite { sprite }, active { active }, unit_type { unit_type },
	floating_flag { floating_flag }, priority { priority }, max_strength { max_strength },
	default_organisation { default_organisation }, maximum_speed { maximum_speed }, weighted_value { weighted_value },
	move_sound { move_sound }, select_sound { select_sound }, build_time { build_time }, build_cost { std::move(build_cost) },
	supply_consumption { supply_consumption }, supply_cost { std::move(supply_cost) } {}

LandUnit::LandUnit(
	std::string_view identifier, UNIT_PARAMS, LAND_PARAMS
) : Unit { identifier, type_t::LAND, UNIT_ARGS }, primary_culture { primary_culture }, sprite_override { sprite_override },
	sprite_mount { sprite_mount }, sprite_mount_attach_node { sprite_mount_attach_node }, reconnaissance { reconnaissance },
	attack { attack }, defence { defence }, discipline { discipline }, support { support }, maneuver { maneuver },
	siege { siege } {}

NavalUnit::NavalUnit(
	std::string_view identifier, UNIT_PARAMS, NAVY_PARAMS
) : Unit { identifier, type_t::NAVAL, UNIT_ARGS }, naval_icon { naval_icon }, sail { sail }, transport { transport },
	capital { capital }, colonial_points { colonial_points }, build_overseas { build_overseas },
	min_port_level { min_port_level }, limit_per_port { limit_per_port },
	supply_consumption_score { supply_consumption_score }, hull { hull }, gun_power { gun_power }, fire_range { fire_range },
	evasion { evasion }, torpedo_attack { torpedo_attack } {};

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

	if (unit_type.empty()) {
		Logger::error("Invalid unit type - empty!");
		return false;
	}

	// TODO check that icon and sprite exist

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

	// TODO: check that icon and sounds exist

	return units.add_item(NavalUnit { identifier, UNIT_ARGS, NAVY_ARGS });
}

callback_t<std::string_view> UnitManager::expect_type_str(Callback<Unit::type_t> auto callback) {
	using enum Unit::type_t;
	static const string_map_t<Unit::type_t> type_map = { { "land", LAND }, { "naval", NAVAL }, { "sea", NAVAL } };
	return expect_mapped_string(type_map, callback);
}

bool UnitManager::load_unit_file(GoodManager const& good_manager, ast::NodeCPtr root) {
	return expect_dictionary([this, &good_manager](std::string_view key, ast::NodeCPtr value) -> bool {
		Unit::type_t type;
		Unit::icon_t icon = 0;
		std::string_view unit_type, sprite, move_sound, select_sound; // TODO defaults for move_sound and select_sound
		bool active = true, floating_flag = false;
		uint32_t priority = 0;
		Timespan build_time;
		fixed_point_t maximum_speed = 0, max_strength = 0, default_organisation = 0;
		fixed_point_t weighted_value = 0, supply_consumption = 0;
		Good::good_map_t build_cost, supply_cost;

		bool ret = expect_key("type", expect_identifier(expect_type_str(assign_variable_callback(type))))(value);

		if (!ret) {
			Logger::error("Failed to read type for unit: ", key);
			return false;
		}

		key_map_t key_map;
		// shared
		ret &= add_key_map_entries(key_map,
			"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(icon)),
			"type", ONE_EXACTLY, success_callback,
			"sprite", ONE_EXACTLY, expect_identifier(assign_variable_callback(sprite)),
			"active", ZERO_OR_ONE, expect_bool(assign_variable_callback(active)),
			"unit_type", ONE_EXACTLY, expect_identifier(assign_variable_callback(unit_type)),
			"floating_flag", ONE_EXACTLY, expect_bool(assign_variable_callback(floating_flag)),
			"priority", ONE_EXACTLY, expect_uint(assign_variable_callback(priority)),
			"max_strength", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(max_strength)),
			"default_organisation", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(default_organisation)),
			"maximum_speed", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(maximum_speed)),
			"weighted_value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(weighted_value)),
			"move_sound", ZERO_OR_ONE, expect_identifier(assign_variable_callback(move_sound)),
			"select_sound", ZERO_OR_ONE, expect_identifier(assign_variable_callback(select_sound)),
			"build_time", ONE_EXACTLY, expect_days(assign_variable_callback(build_time)),
			"build_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(move_variable_callback(build_cost)),
			"supply_consumption", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(supply_consumption)),
			"supply_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(move_variable_callback(supply_cost))
		);

		switch (type) {
		case Unit::type_t::LAND: {
			bool primary_culture = false;
			std::string_view sprite_override {}, sprite_mount {}, sprite_mount_attach_node {};
			fixed_point_t reconnaissance = 0, attack = 0, defence = 0, discipline = 0, support = 0, maneuver = 0, siege = 0;
			
			ret &= add_key_map_entries(key_map,
				"primary_culture", ZERO_OR_ONE, expect_bool(assign_variable_callback(primary_culture)),
				"sprite_override", ZERO_OR_ONE, expect_identifier(assign_variable_callback(sprite_override)),
				"sprite_mount", ZERO_OR_ONE, expect_identifier(assign_variable_callback(sprite_mount)),
				"sprite_mount_attach_node", ZERO_OR_ONE, expect_identifier(assign_variable_callback(sprite_mount_attach_node)),
				"reconnaissance", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(reconnaissance)),
				"attack", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(attack)),
				"defence", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(defence)),
				"discipline", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(discipline)),
				"support", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(support)),
				"maneuver", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(maneuver)),
				"siege", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(siege))
			);
			
			ret &= expect_dictionary_key_map(key_map)(value);
			
			ret &= add_land_unit(key, UNIT_ARGS, LAND_ARGS);

			return ret;
		}
		case Unit::type_t::NAVAL: {
			Unit::icon_t naval_icon = 0;
			bool sail = false, transport = false, capital = false, build_overseas = false;
			uint32_t min_port_level = 0;
			int32_t limit_per_port = 0;
			fixed_point_t fire_range = 0, evasion = 0, supply_consumption_score = 0, hull = 0;
			fixed_point_t gun_power = 0, colonial_points = 0, torpedo_attack = 0;

			ret &= add_key_map_entries(key_map,
				"naval_icon", ONE_EXACTLY, expect_uint(assign_variable_callback(naval_icon)),
				"sail", ZERO_OR_ONE, expect_bool(assign_variable_callback(sail)),
				"transport", ZERO_OR_ONE, expect_bool(assign_variable_callback(transport)),
				"capital", ZERO_OR_ONE, expect_bool(assign_variable_callback(capital)),
				"colonial_points", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(colonial_points)),
				"can_build_overseas", ZERO_OR_ONE, expect_bool(assign_variable_callback(build_overseas)),
				"min_port_level", ONE_EXACTLY, expect_uint(assign_variable_callback(min_port_level)),
				"limit_per_port", ONE_EXACTLY, expect_int(assign_variable_callback(limit_per_port)),
				"supply_consumption_score", ONE_EXACTLY,
					expect_fixed_point(assign_variable_callback(supply_consumption_score)),
				"hull", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(hull)),
				"gun_power", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(gun_power)),
				"fire_range", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(fire_range)),
				"evasion", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(evasion)),
				"torpedo_attack", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(torpedo_attack))
			);

			ret &= expect_dictionary_key_map(key_map)(value);
			
			ret &= add_naval_unit(key, UNIT_ARGS, NAVY_ARGS);
			
			return ret;
		}
			default: Logger::error("Unknown unit type for ", key, ": ", static_cast<int>(type)); return false;
		}
	})(root);
}

#define STAT_MODIFIER(name, positive_good, format) \
	ret &= modifier_manager.add_modifier_effect(base_name + name, positive_good, ModifierEffect::format_t::format)

bool UnitManager::generate_modifiers(ModifierManager& modifier_manager) {
	std::function<bool(std::string_view, Unit::type_t)> generate_stat_modifiers = [this, &modifier_manager](std::string_view identifier, Unit::type_t type) -> bool {
		modifier_manager.register_complex_modifier(identifier);
		std::string base_name = std::string(identifier) + "_";
		bool ret = true;

		STAT_MODIFIER("default_organisation", true, RAW_DECIMAL);
		STAT_MODIFIER("maximum_speed", true, RAW_DECIMAL);
		STAT_MODIFIER("build_time", false, INT);
		STAT_MODIFIER("supply_consumption", false, PROPORTION_DECIMAL);

		switch (type) {
		case Unit::type_t::LAND: {
			STAT_MODIFIER("reconnaissance", true, RAW_DECIMAL);
			STAT_MODIFIER("attack", true, RAW_DECIMAL);
			STAT_MODIFIER("defence", true, RAW_DECIMAL);
			STAT_MODIFIER("discipline", true, PROPORTION_DECIMAL);
			STAT_MODIFIER("support", true, PROPORTION_DECIMAL);
			STAT_MODIFIER("maneuver", true, INT);
			STAT_MODIFIER("siege", true, RAW_DECIMAL);
			break;
		}
        case Unit::type_t::NAVAL: {
			STAT_MODIFIER("colonial_points", true, INT);
			STAT_MODIFIER("supply_consumption_score", false, INT);
			STAT_MODIFIER("hull", true, RAW_DECIMAL);
			STAT_MODIFIER("gun_power", true, RAW_DECIMAL);
			STAT_MODIFIER("fire_range", true, RAW_DECIMAL);
			STAT_MODIFIER("evasion", true, PROPORTION_DECIMAL);
			STAT_MODIFIER("torpedo_attack", true, RAW_DECIMAL);
        	break;
        }
		}

		return ret;
	};

	bool ret = true;
	ret &= generate_stat_modifiers("army_base", Unit::type_t::LAND);
	ret &= generate_stat_modifiers("navy_base", Unit::type_t::NAVAL);
	for (Unit const& unit : this->get_units())
		ret &= generate_stat_modifiers(unit.get_identifier(), unit.get_type());

	return ret;
}