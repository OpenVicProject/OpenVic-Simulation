#include "UnitType.hpp"

#include "openvic-simulation/map/TerrainType.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

using enum UnitType::branch_t;
using enum UnitType::unit_category_t;

UnitType::UnitType(
	std::string_view new_identifier, branch_t new_branch, unit_type_args_t& unit_args
) : HasIdentifier { new_identifier },
	branch { new_branch },
	icon { unit_args.icon },
	sprite { unit_args.sprite },
	active { unit_args.active },
	unit_category { unit_args.unit_category },
	floating_flag { unit_args.floating_flag },
	priority { unit_args.priority },
	max_strength { unit_args.max_strength },
	default_organisation { unit_args.default_organisation },
	maximum_speed { unit_args.maximum_speed },
	weighted_value { unit_args.weighted_value },
	move_sound { unit_args.move_sound },
	select_sound { unit_args.select_sound },
	build_time { unit_args.build_time },
	build_cost { std::move(unit_args.build_cost) },
	supply_consumption { unit_args.supply_consumption },
	supply_cost { std::move(unit_args.supply_cost) },
	terrain_modifiers { std::move(unit_args.terrain_modifiers) } {}

RegimentType::RegimentType(
	std::string_view new_identifier, unit_type_args_t& unit_args, regiment_type_args_t const& regiment_type_args
) : UnitType { new_identifier, LAND, unit_args },
	allowed_cultures { regiment_type_args.allowed_cultures },
	sprite_override { regiment_type_args.sprite_override },
	sprite_mount { regiment_type_args.sprite_mount },
	sprite_mount_attach_node { regiment_type_args.sprite_mount_attach_node },
	reconnaissance { regiment_type_args.reconnaissance },
	attack { regiment_type_args.attack },
	defence { regiment_type_args.defence },
	discipline { regiment_type_args.discipline },
	support { regiment_type_args.support },
	maneuver { regiment_type_args.maneuver },
	siege { regiment_type_args.siege } {}

ShipType::ShipType(
	std::string_view new_identifier, unit_type_args_t& unit_args, ship_type_args_t const& ship_type_args
) : UnitType { new_identifier, NAVAL, unit_args },
	naval_icon { ship_type_args.naval_icon },
	sail { ship_type_args.sail },
	transport { ship_type_args.transport },
	capital { ship_type_args.capital },
	colonial_points { ship_type_args.colonial_points },
	build_overseas { ship_type_args.build_overseas },
	min_port_level { ship_type_args.min_port_level },
	limit_per_port { ship_type_args.limit_per_port },
	supply_consumption_score { ship_type_args.supply_consumption_score },
	hull { ship_type_args.hull },
	gun_power { ship_type_args.gun_power },
	fire_range { ship_type_args.fire_range },
	evasion { ship_type_args.evasion },
	torpedo_attack { ship_type_args.torpedo_attack } {}

void UnitTypeManager::reserve_all_unit_types(size_t size) {
	reserve_more_unit_types(size);
	reserve_more_regiment_types(size);
	reserve_more_ship_types(size);
}

void UnitTypeManager::lock_all_unit_types() {
	unit_types.lock();
	regiment_types.lock();
	ship_types.lock();
}

static bool _check_shared_parameters(std::string_view identifier, UnitType::unit_type_args_t const& unit_args) {
	if (identifier.empty()) {
		Logger::error("Invalid unit identifier - empty!");
		return false;
	}

	if (unit_args.icon <= 0) {
		Logger::error("Invalid icon for unit ", identifier, " - ", unit_args.icon, " (must be positive)");
		return false;
	}

	if (unit_args.unit_category == INVALID_UNIT_CATEGORY) {
		Logger::error("Invalid unit type for unit ", identifier, "!");
		return false;
	}

	// TODO check that sprite, move_sound and select_sound exist

	return true;
}

bool UnitTypeManager::add_regiment_type(
	std::string_view identifier, UnitType::unit_type_args_t& unit_args, RegimentType::regiment_type_args_t const& regiment_type_args
) {
	if (!_check_shared_parameters(identifier, unit_args)) {
		return false;
	}

	// TODO check that sprite_override, sprite_mount, and sprite_mount_attach_node exist

	if (ship_types.has_identifier(identifier)) {
		Logger::error("Land unit ", identifier, " already exists as a naval unit!");
		return false;
	}

	bool ret = regiment_types.add_item({ identifier, unit_args, std::move(regiment_type_args) });
	if (ret) {
		ret &= unit_types.add_item(&regiment_types.get_items().back());
	}
	return ret;
}

bool UnitTypeManager::add_ship_type(
	std::string_view identifier, UnitType::unit_type_args_t& unit_args, ShipType::ship_type_args_t const& ship_type_args
) {
	if (!_check_shared_parameters(identifier, unit_args)) {
		return false;
	}

	if (ship_type_args.naval_icon <= 0) {
		Logger::error("Invalid naval icon identifier - ", ship_type_args.naval_icon, " (must be positive)");
		return false;
	}

	if (ship_type_args.supply_consumption_score <= 0) {
		Logger::warning("Supply consumption score for ", identifier, " is not positive!");
	}

	if (regiment_types.has_identifier(identifier)) {
		Logger::error("Naval unit ", identifier, " already exists as a land unit!");
		return false;
	}

	bool ret = ship_types.add_item({ identifier, unit_args, ship_type_args });
	if (ret) {
		ret &= unit_types.add_item(&ship_types.get_items().back());
	}
	return ret;
}

bool UnitTypeManager::load_unit_type_file(
	GoodManager const& good_manager, TerrainTypeManager const& terrain_type_manager, ModifierManager const& modifier_manager,
	ast::NodeCPtr root
) {
	return expect_dictionary([this, &good_manager, &terrain_type_manager, &modifier_manager](
		std::string_view key, ast::NodeCPtr value
	) -> bool {

		UnitType::branch_t branch = INVALID_BRANCH;

		bool ret = expect_key("type", expect_branch_identifier(assign_variable_callback(branch)))(value);

		/* We shouldn't just check ret as it can be false even if branch was successfully parsed,
		 * but more than one instance of the key was found. */
		if (branch != LAND && branch != NAVAL) {
			Logger::error("Failed to read branch for unit: ", key);
			return false;
		}

		UnitType::unit_type_args_t unit_args {};

		static const string_map_t<UnitType::unit_category_t> unit_type_map {
			{ "infantry", INFANTRY },
			{ "cavalry", CAVALRY },
			{ "support", SUPPORT },
			{ "special", SPECIAL },
			{ "big_ship", BIG_SHIP },
			{ "light_ship", LIGHT_SHIP },
			{ "transport", TRANSPORT }
		};

		key_map_t key_map {};
		/* Shared dictionary entries */
		ret &= add_key_map_entries(key_map,
			"icon", ONE_EXACTLY, expect_uint(assign_variable_callback(unit_args.icon)),
			"type", ONE_EXACTLY, success_callback, /* Already loaded above using expect_key */
			"sprite", ONE_EXACTLY, expect_identifier(assign_variable_callback(unit_args.sprite)),
			"active", ZERO_OR_ONE, expect_bool(assign_variable_callback(unit_args.active)),
			"unit_type", ONE_EXACTLY,
				expect_identifier(expect_mapped_string(unit_type_map, assign_variable_callback(unit_args.unit_category))),
			"floating_flag", ONE_EXACTLY, expect_bool(assign_variable_callback(unit_args.floating_flag)),
			"priority", ONE_EXACTLY, expect_uint(assign_variable_callback(unit_args.priority)),
			"max_strength", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(unit_args.max_strength)),
			"default_organisation", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(unit_args.default_organisation)),
			"maximum_speed", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(unit_args.maximum_speed)),
			"weighted_value", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(unit_args.weighted_value)),
			"move_sound", ZERO_OR_ONE, expect_identifier(assign_variable_callback(unit_args.move_sound)),
			"select_sound", ZERO_OR_ONE, expect_identifier(assign_variable_callback(unit_args.select_sound)),
			"build_time", ONE_EXACTLY, expect_days(assign_variable_callback(unit_args.build_time)),
			"build_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(move_variable_callback(unit_args.build_cost)),
			"supply_consumption", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(unit_args.supply_consumption)),
			"supply_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(move_variable_callback(unit_args.supply_cost))
		);

		const auto add_terrain_modifier = [&unit_args, &terrain_type_manager, &modifier_manager](
			std::string_view default_key, ast::NodeCPtr default_value
		) -> bool {
			TerrainType const* terrain_type = terrain_type_manager.get_terrain_type_by_identifier(default_key);
			if (terrain_type != nullptr) {
				// TODO - restrict what modifier effects can be used here
				return modifier_manager.expect_modifier_value(
					map_callback(unit_args.terrain_modifiers, terrain_type)
				)(default_value);
			}
			return key_value_invalid_callback(default_key, default_value);
		};

		switch (branch) {
		case LAND: {
			RegimentType::regiment_type_args_t regiment_type_args {};
			bool is_restricted_to_primary_culture = false;
			bool is_restricted_to_accepted_cultures = false;

			ret &= add_key_map_entries(key_map,
				"primary_culture", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_restricted_to_primary_culture)),
				"accepted_culture", ZERO_OR_ONE, expect_bool(assign_variable_callback(is_restricted_to_accepted_cultures)),
				"sprite_override", ZERO_OR_ONE, expect_identifier(assign_variable_callback(regiment_type_args.sprite_override)),
				"sprite_mount", ZERO_OR_ONE, expect_identifier(assign_variable_callback(regiment_type_args.sprite_mount)),
				"sprite_mount_attach_node", ZERO_OR_ONE,
					expect_identifier(assign_variable_callback(regiment_type_args.sprite_mount_attach_node)),
				"reconnaissance", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(regiment_type_args.reconnaissance)),
				"attack", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(regiment_type_args.attack)),
				"defence", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(regiment_type_args.defence)),
				"discipline", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(regiment_type_args.discipline)),
				"support", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(regiment_type_args.support)),
				"maneuver", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(regiment_type_args.maneuver)),
				"siege", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(regiment_type_args.siege))
			);

			if (is_restricted_to_accepted_cultures) {
				regiment_type_args.allowed_cultures = RegimentType::allowed_cultures_t::ACCEPTED_CULTURES;
			} else if (is_restricted_to_primary_culture) {
				regiment_type_args.allowed_cultures = RegimentType::allowed_cultures_t::PRIMARY_CULTURE;
			} else {
				regiment_type_args.allowed_cultures = RegimentType::allowed_cultures_t::ALL_CULTURES;
			}

			ret &= expect_dictionary_key_map_and_default(key_map, add_terrain_modifier)(value);

			ret &= add_regiment_type(key, unit_args, regiment_type_args);

			return ret;
		}
		case NAVAL: {
			ShipType::ship_type_args_t ship_type_args {};

			ret &= add_key_map_entries(key_map,
				"naval_icon", ONE_EXACTLY, expect_uint(assign_variable_callback(ship_type_args.naval_icon)),
				"sail", ZERO_OR_ONE, expect_bool(assign_variable_callback(ship_type_args.sail)),
				"transport", ZERO_OR_ONE, expect_bool(assign_variable_callback(ship_type_args.transport)),
				"capital", ZERO_OR_ONE, expect_bool(assign_variable_callback(ship_type_args.capital)),
				"colonial_points", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(ship_type_args.colonial_points)),
				"can_build_overseas", ZERO_OR_ONE, expect_bool(assign_variable_callback(ship_type_args.build_overseas)),
				"min_port_level", ONE_EXACTLY, expect_uint(assign_variable_callback(ship_type_args.min_port_level)),
				"limit_per_port", ONE_EXACTLY, expect_int(assign_variable_callback(ship_type_args.limit_per_port)),
				"supply_consumption_score", ZERO_OR_ONE,
					expect_fixed_point(assign_variable_callback(ship_type_args.supply_consumption_score)),
				"hull", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ship_type_args.hull)),
				"gun_power", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ship_type_args.gun_power)),
				"fire_range", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ship_type_args.fire_range)),
				"evasion", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(ship_type_args.evasion)),
				"torpedo_attack", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(ship_type_args.torpedo_attack))
			);

			ret &= expect_dictionary_key_map_and_default(key_map, add_terrain_modifier)(value);

			ret &= add_ship_type(key, unit_args, ship_type_args);

			return ret;
		}
		default:
			/* Unreachable - an earlier check terminates the function if branch isn't LAND or NAVAL. */
			Logger::error("Unknown branch for unit ", key, ": ", static_cast<int>(branch));
			return false;
		}
	})(root);
}

bool UnitTypeManager::generate_modifiers(ModifierManager& modifier_manager) const {
	bool ret = true;

	const auto generate_stat_modifiers = [&modifier_manager, &ret](std::string_view identifier, UnitType::branch_t branch) -> void {
		const auto stat_modifier = [&modifier_manager, &ret, &identifier](
			std::string_view suffix, bool is_positive_good, ModifierEffect::format_t format
		) -> void {
			ret &= modifier_manager.add_modifier_effect(
				ModifierManager::get_flat_identifier(identifier, suffix), is_positive_good, format
			);
		};

		using enum ModifierEffect::format_t;

		ret &= modifier_manager.register_complex_modifier(identifier);

		stat_modifier("attack", true, RAW_DECIMAL);
		stat_modifier("defence", true, RAW_DECIMAL);
		stat_modifier("default_organisation", true, RAW_DECIMAL);
		stat_modifier("maximum_speed", true, RAW_DECIMAL);
		stat_modifier("build_time", false, INT);
		stat_modifier("supply_consumption", false, PROPORTION_DECIMAL);

		switch (branch) {
		case LAND:
			stat_modifier("reconnaissance", true, RAW_DECIMAL);
			stat_modifier("discipline", true, PROPORTION_DECIMAL);
			stat_modifier("support", true, PROPORTION_DECIMAL);
			stat_modifier("maneuver", true, INT);
			stat_modifier("siege", true, RAW_DECIMAL);
			break;
		case NAVAL:
			stat_modifier("colonial_points", true, INT);
			stat_modifier("supply_consumption_score", false, INT);
			stat_modifier("hull", true, RAW_DECIMAL);
			stat_modifier("gun_power", true, RAW_DECIMAL);
			stat_modifier("fire_range", true, RAW_DECIMAL);
			stat_modifier("evasion", true, PROPORTION_DECIMAL);
			stat_modifier("torpedo_attack", true, RAW_DECIMAL);
			break;
		default:
			/* Unreachable - unit types are only added via add_regiment_type or add_ship_type which set branch to LAND or NAVAL. */
			Logger::error("Invalid branch for unit ", identifier, ": ", static_cast<int>(branch));
		}
	};

	generate_stat_modifiers("army_base", LAND);
	for (RegimentType const& regiment_type : get_regiment_types()) {
		generate_stat_modifiers(regiment_type.get_identifier(), LAND);
	}

	generate_stat_modifiers("navy_base", NAVAL);
	for (ShipType const& ship_type : get_ship_types()) {
		generate_stat_modifiers(ship_type.get_identifier(), NAVAL);
	}

	return ret;
}
