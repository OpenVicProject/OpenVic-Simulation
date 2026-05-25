#include "UnitType.hpp"

#include "openvic-simulation/map/TerrainType.hpp" // IWYU pragma: keep for fmt
#include "openvic-simulation/types/UnitBranchType.hpp"

using namespace OpenVic;

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
