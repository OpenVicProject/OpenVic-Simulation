#include "BuildingType.hpp"

#include <optional>

#include "openvic-simulation/core/error/ErrorMacros.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/State.hpp"
#include "openvic-simulation/modifier/ModifierEffectCache.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

using namespace OpenVic;

BuildingType::BuildingType(
	index_t new_index,
	std::optional<province_building_index_t> new_province_building_index,
	std::string_view new_identifier,
	building_type_args_t& building_type_args
) : HasIndex { new_index },
	Modifier { new_identifier, std::move(building_type_args.modifier), modifier_type_t::BUILDING },
	province_building_index { new_province_building_index },
	type { building_type_args.type },
	on_completion { building_type_args.on_completion },
	completion_size { building_type_args.completion_size },
	max_level { building_type_args.max_level },
	goods_cost { std::move(building_type_args.goods_cost) },
	cost { building_type_args.cost },
	build_time { building_type_args.build_time },
	should_display_on_map { building_type_args.on_map },
	is_enabled_by_default { building_type_args.default_enabled },
	production_type { building_type_args.production_type },
	is_pop_build_factory { building_type_args.pop_build_factory },
	is_strategic_factory { building_type_args.strategic_factory },
	is_advanced_factory { building_type_args.advanced_factory },
	fort_level { building_type_args.fort_level },
	naval_capacity { building_type_args.naval_capacity },
	colonial_points { std::move(building_type_args.colonial_points) },
	is_in_province { building_type_args.in_province },
	restriction_category {
		building_type_args.pop_build_factory
			? building_type_args.in_province
				? BuildingRestrictionCategory::INFRASTRUCTURE
				: BuildingRestrictionCategory::FACTORY
			: BuildingRestrictionCategory::UNRESTRICTED
	},
	is_limited_to_one_per_state { building_type_args.one_per_state },
	colonial_range { building_type_args.colonial_range },
	infrastructure { building_type_args.infrastructure },
	should_spawn_railway_track { building_type_args.spawn_railway_track },
	is_sail { building_type_args.sail },
	is_steam { building_type_args.steam },
	capital { building_type_args.capital },
	is_port { building_type_args.port } {}

bool BuildingType::can_be_built_in(
	ModifierEffectCache const& modifier_effect_cache,
	const building_level_t desired_level,
	CountryInstance const& actor,
	ProvinceInstance const& location
) const {
	OV_ERR_FAIL_COND_V_MSG(
		!is_in_province,
		false,
		memory::fmt::format("BuildingType::can_be_built_in (province variant) was called on state level building {}", get_identifier())
	);

	if (desired_level > max_level) {
		return false;
	}

	if (!can_be_built_in(location.province_definition)) {
		return false;
	}

	if (!location.may_build_here()) {
		return false;
	}

	if (!actor.may_build_in(restriction_category, location)) {
		return false;
	}

	if (is_limited_to_one_per_state) {
		State const* state = location.get_state();
		if (state != nullptr) {
			for (ProvinceInstance const& province_in_state : state->get_provinces()) {
				if (province_in_state == location) {
					continue;
				}

				const building_level_t other_building_level = province_in_state.get_buildings()[province_building_index.value()].get_level();
				if (other_building_level > building_level_t(0)) {
					return false;
				}
			}
		}
	}

	building_level_t const& unlocked_max_level = actor.get_building_type_unlock_levels(*this);
	ModifierEffectCache::building_type_effects_t const& effects = modifier_effect_cache.get_building_type_effects(*this);
	const fixed_point_t min_level_modifier = location.get_modifier_effect_value(*effects.get_min_level());
	return fixed_point_t { type_safe::get(unlocked_max_level) }
		>= fixed_point_t { type_safe::get(desired_level) } + min_level_modifier;
}

bool BuildingType::can_be_built_in(ProvinceDefinition const& location) const {
	return !is_port || location.has_port();
}
