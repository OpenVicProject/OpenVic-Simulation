#include "UnitType.hpp"

#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/map/TerrainType.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "modifier/ModifierEffectCache.hpp"
#include "types/UnitBranchType.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

using enum unit_branch_t;
using enum UnitType::unit_category_t;

UnitType::UnitType(
	std::string_view new_identifier, unit_branch_t new_branch, unit_type_args_t& unit_args
) : HasIdentifier { new_identifier },
	branch { new_branch },
	icon { unit_args.icon },
	sprite { unit_args.sprite },
	starts_unlocked { unit_args.starts_unlocked },
	unit_category { unit_args.unit_category },
	has_floating_flag { unit_args.floating_flag },
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
	supply_cost { std::move(unit_args.supply_cost) } {

	using enum Modifier::modifier_type_t;

	for (auto [terrain, modifier_value] : mutable_iterator(unit_args.terrain_modifier_values)) {
		terrain_modifiers.emplace(terrain, Modifier {
			memory::fmt::format("{} {}", new_identifier, *terrain), std::move(modifier_value),
			UNIT_TERRAIN
		});
	}
}

UnitTypeBranched<LAND>::UnitTypeBranched(
	index_t new_index, std::string_view new_identifier,
	unit_type_args_t& unit_args, regiment_type_args_t const& regiment_type_args
) : UnitType { new_identifier, LAND, unit_args },
	HasIndex { new_index },
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

UnitTypeBranched<NAVAL>::UnitTypeBranched(
	index_t new_index, std::string_view new_identifier,
	unit_type_args_t& unit_args, ship_type_args_t const& ship_type_args
) : UnitType { new_identifier, NAVAL, unit_args },
	HasIndex { new_index },
	naval_icon { ship_type_args.naval_icon },
	can_sail { ship_type_args.sail },
	is_transport { ship_type_args.transport },
	is_capital { ship_type_args.capital },
	colonial_points { ship_type_args.colonial_points },
	can_build_overseas { ship_type_args.build_overseas },
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
		spdlog::error_s("Invalid unit identifier - empty!");
		return false;
	}

	if (unit_args.icon < 0) {
		spdlog::error_s(
			"Invalid icon for unit {} - {} (must be >= 0)",
			identifier, unit_args.icon
		);
		return false;
	}

	if (unit_args.unit_category == INVALID_UNIT_CATEGORY) {
		spdlog::error_s("Invalid unit type for unit {}!", identifier);
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
		spdlog::error_s("Land unit {} already exists as a naval unit!", identifier);
		return false;
	}

	bool ret = regiment_types.emplace_item(
		identifier,
		RegimentType::index_t { get_regiment_type_count() }, identifier,
		unit_args, std::move(regiment_type_args)
	);
	if (ret) {
		// Cannot use get_back_regiment_type() as we need non-const but don't want to generate all non-const functions.
		ret &= unit_types.emplace_via_copy(&regiment_types.back());
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
		spdlog::error_s("Invalid naval icon identifier - {} (must be positive)", ship_type_args.naval_icon);
		return false;
	}

	if (ship_type_args.supply_consumption_score <= 0) {
		spdlog::warn_s("Supply consumption score for {} is not positive!", identifier);
	}

	if (regiment_types.has_identifier(identifier)) {
		spdlog::error_s("Naval unit {} already exists as a land unit!", identifier);
		return false;
	}

	bool ret = ship_types.emplace_item(
		identifier,
		ShipType::index_t { get_ship_type_count() }, identifier,
		unit_args, ship_type_args
	);
	if (ret) {
		// Cannot use get_back_ship_type() as we need non-const but don't want to generate all non-const functions.
		ret &= unit_types.emplace_via_copy(&ship_types.back());
	}
	return ret;
}

bool UnitTypeManager::load_unit_type_file(
	GoodDefinitionManager const& good_definition_manager, TerrainTypeManager const& terrain_type_manager,
	ModifierManager const& modifier_manager, ovdl::v2script::Parser const& parser
) {
	using namespace std::string_view_literals;
	auto type_symbol = parser.find_intern("type"sv);
	if (!type_symbol) {
		spdlog::error_s("type could not be interned.");
	}

	return expect_dictionary([this, &good_definition_manager, &terrain_type_manager, &modifier_manager, &type_symbol](
		std::string_view key, ast::NodeCPtr value
	) -> bool {

		unit_branch_t branch = INVALID_BRANCH;

		bool ret = expect_key(type_symbol, expect_branch_identifier(assign_variable_callback(branch)))(value);

		/* We shouldn't just check ret as it can be false even if branch was successfully parsed,
		 * but more than one instance of the key was found. */
		if (branch != LAND && branch != NAVAL) {
			spdlog::error_s("Failed to read branch for unit: {}", key);
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
			"active", ZERO_OR_ONE, expect_bool(assign_variable_callback(unit_args.starts_unlocked)),
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
			"build_cost", ONE_EXACTLY,
				good_definition_manager.expect_good_definition_decimal_map(move_variable_callback(unit_args.build_cost)),
			"supply_consumption", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(unit_args.supply_consumption)),
			"supply_cost", ONE_EXACTLY,
				good_definition_manager.expect_good_definition_decimal_map(move_variable_callback(unit_args.supply_cost))
		);

		auto add_terrain_modifier_value = [&unit_args, &terrain_type_manager, &modifier_manager](
			std::string_view default_key, ast::NodeCPtr default_value
		) -> bool {
			TerrainType const* terrain_type = terrain_type_manager.get_terrain_type_by_identifier(default_key);

			if (terrain_type != nullptr) {
				ModifierValue& modifier_value = unit_args.terrain_modifier_values[terrain_type];
				return expect_dictionary(
					modifier_manager.expect_unit_terrain_modifier(modifier_value, terrain_type->get_identifier())
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
				regiment_type_args.allowed_cultures = regiment_allowed_cultures_t::ACCEPTED_CULTURES;
			} else if (is_restricted_to_primary_culture) {
				regiment_type_args.allowed_cultures = regiment_allowed_cultures_t::PRIMARY_CULTURE;
			} else {
				regiment_type_args.allowed_cultures = regiment_allowed_cultures_t::ALL_CULTURES;
			}

			ret &= expect_dictionary_key_map_and_default(key_map, add_terrain_modifier_value)(value);

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

			ret &= expect_dictionary_key_map_and_default(key_map, add_terrain_modifier_value)(value);

			ret &= add_ship_type(key, unit_args, ship_type_args);

			return ret;
		}
		default:
			/* Unreachable - an earlier check terminates the function if branch isn't LAND or NAVAL. */
			spdlog::error_s("Unknown branch for unit {}: {}", key, static_cast<int>(branch));
			return false;
		}
	})(parser.get_file_node());
}

bool UnitTypeManager::generate_modifiers(ModifierManager& modifier_manager) const {
	bool ret = true;

	const auto generate_stat_modifiers = [&modifier_manager, &ret](
		std::derived_from<ModifierEffectCache::unit_type_effects_t> auto unit_type_effects, std::string_view identifier
	) -> void {
		using enum ModifierEffect::format_t;

		const auto stat_modifier = [&modifier_manager, &ret, &identifier](
			ModifierEffect const*& effect_cache, std::string_view suffix,
			ModifierEffect::format_t format, std::string_view localisation_key
		) -> void {
			ret &= modifier_manager.register_technology_modifier_effect(
				effect_cache, ModifierManager::get_flat_identifier(identifier, suffix), format,
				memory::fmt::format("${}$: ${}$", identifier, localisation_key)
			);
		};

		ret &= modifier_manager.register_complex_modifier(identifier);

		// These are ordered following how they appear in the base game, hence the broken up land effects.
		stat_modifier(unit_type_effects.default_organisation, "default_organisation", FORMAT_x1_1DP_POS, "DEFAULT_ORG");
		stat_modifier(unit_type_effects.build_time, "build_time", FORMAT_x1_0DP_DAYS_NEG, "BUILD_TIME");
		if constexpr (std::same_as<decltype(unit_type_effects), ModifierEffectCache::regiment_type_effects_t>) {
			stat_modifier(
				unit_type_effects.reconnaissance, "reconnaissance", FORMAT_x1_2DP_POS, "RECONAISSANCE" // paradox typo
			);
			stat_modifier(unit_type_effects.siege, "siege", FORMAT_x1_2DP_POS, "SIEGE");
		}
		stat_modifier(unit_type_effects.attack, "attack", FORMAT_x1_2DP_POS, "ATTACK");
		stat_modifier(unit_type_effects.defence, "defence", FORMAT_x1_2DP_POS, "DEFENCE");
		if constexpr (std::same_as<decltype(unit_type_effects), ModifierEffectCache::regiment_type_effects_t>) {
			stat_modifier(unit_type_effects.discipline, "discipline", FORMAT_x100_0DP_PC_POS, "DISCIPLINE");
			stat_modifier(unit_type_effects.support, "support", FORMAT_x100_0DP_PC_POS, "SUPPORT");
			stat_modifier(unit_type_effects.maneuver, "maneuver", FORMAT_x1_0DP_POS, "Maneuver");
		}
		stat_modifier(unit_type_effects.maximum_speed, "maximum_speed", FORMAT_x1_2DP_SPEED_POS, "MAXIMUM_SPEED");
		if constexpr(std::same_as<decltype(unit_type_effects), ModifierEffectCache::ship_type_effects_t>) {
			stat_modifier(unit_type_effects.gun_power, "gun_power", FORMAT_x1_2DP_POS, "GUN_POWER");
			stat_modifier(unit_type_effects.torpedo_attack, "torpedo_attack", FORMAT_x1_2DP_POS, "TORPEDO_ATTACK");
			stat_modifier(unit_type_effects.hull, "hull", FORMAT_x1_2DP_POS, "HULL");
			stat_modifier(unit_type_effects.fire_range, "fire_range", FORMAT_x100_0DP_POS, "FIRE_RANGE");
			stat_modifier(unit_type_effects.evasion, "evasion", FORMAT_x100_0DP_PC_POS, "EVASION");
		}
		stat_modifier(
			unit_type_effects.supply_consumption, "supply_consumption", FORMAT_x100_0DP_PC_NEG, "SUPPLY_CONSUMPTION"
		);
		if constexpr(std::same_as<decltype(unit_type_effects), ModifierEffectCache::ship_type_effects_t>) {
			stat_modifier(
				unit_type_effects.supply_consumption_score, "supply_consumption_score", FORMAT_x1_0DP_NEG, "SUPPLY_LOAD"
			);
			// Works but doesn't appear in tooltips in the base game
			stat_modifier(unit_type_effects.colonial_points, "colonial_points", FORMAT_x1_0DP_POS, "COLONIAL_POINTS_TECH");
		}
	};

	generate_stat_modifiers(modifier_manager.modifier_effect_cache.army_base_effects, "army_base");

	IndexedFlatMap<RegimentType, ModifierEffectCache::regiment_type_effects_t>& regiment_type_effects =
		modifier_manager.modifier_effect_cache.regiment_type_effects;

	regiment_type_effects = std::move(decltype(ModifierEffectCache::regiment_type_effects){get_regiment_types()});

	for (RegimentType const& regiment_type : get_regiment_types()) {
		generate_stat_modifiers(regiment_type_effects.at(regiment_type), regiment_type.get_identifier());
	}

	generate_stat_modifiers(modifier_manager.modifier_effect_cache.navy_base_effects, "navy_base");

	IndexedFlatMap<ShipType, ModifierEffectCache::ship_type_effects_t>& ship_type_effects =
		modifier_manager.modifier_effect_cache.ship_type_effects;

	ship_type_effects = std::move(decltype(ModifierEffectCache::ship_type_effects){get_ship_types()});

	for (ShipType const& ship_type : get_ship_types()) {
		generate_stat_modifiers(ship_type_effects.at(ship_type), ship_type.get_identifier());
	}

	return ret;
}
