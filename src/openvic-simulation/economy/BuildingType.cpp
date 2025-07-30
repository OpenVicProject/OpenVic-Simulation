#include "BuildingType.hpp"

#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

BuildingType::BuildingType(
	std::string_view identifier, building_type_args_t& building_type_args
) : Modifier { identifier, std::move(building_type_args.modifier), modifier_type_t::BUILDING },
	type { building_type_args.type },
	on_completion { building_type_args.on_completion },
	completion_size { building_type_args.completion_size },
	max_level { building_type_args.max_level },
	goods_cost { std::move(building_type_args.goods_cost) },
	cost { building_type_args.cost },
	build_time { building_type_args.build_time },
	on_map { building_type_args.on_map },
	default_enabled { building_type_args.default_enabled },
	production_type { building_type_args.production_type },
	pop_build_factory { building_type_args.pop_build_factory },
	strategic_factory { building_type_args.strategic_factory },
	advanced_factory { building_type_args.advanced_factory },
	fort_level { building_type_args.fort_level },
	naval_capacity { building_type_args.naval_capacity },
	colonial_points { std::move(building_type_args.colonial_points) },
	in_province { building_type_args.in_province },
	one_per_state { building_type_args.one_per_state },
	colonial_range { building_type_args.colonial_range },
	infrastructure { building_type_args.infrastructure },
	spawn_railway_track { building_type_args.spawn_railway_track },
	sail { building_type_args.sail },
	steam { building_type_args.steam },
	capital { building_type_args.capital },
	port { building_type_args.port } {}

BuildingTypeManager::BuildingTypeManager() : port_building_type { nullptr } {}

bool BuildingTypeManager::add_building_type(
	std::string_view identifier, BuildingType::building_type_args_t& building_type_args
) {
	if (identifier.empty()) {
		Logger::error("Invalid building identifier - empty!");
		return false;
	}

	const bool ret = building_types.emplace_item( identifier, identifier, building_type_args );

	if (ret) {
		building_type_types.emplace(building_type_args.type);
	}

	return ret;
}

bool BuildingTypeManager::load_buildings_file(
	GoodDefinitionManager const& good_definition_manager, ProductionTypeManager const& production_type_manager,
	ModifierManager& modifier_manager, ast::NodeCPtr root
) {
	bool ret = expect_dictionary_reserve_length(
		building_types, [this, &good_definition_manager, &production_type_manager, &modifier_manager](
			std::string_view key, ast::NodeCPtr value
		) -> bool {
			BuildingType::building_type_args_t building_type_args {};

			bool ret = NodeTools::expect_dictionary_keys_and_default(
				modifier_manager.expect_base_province_modifier(building_type_args.modifier),
				"type", ONE_EXACTLY, expect_identifier(assign_variable_callback(building_type_args.type)),
				"on_completion", ZERO_OR_ONE, expect_identifier(assign_variable_callback(building_type_args.on_completion)),
				"completion_size", ZERO_OR_ONE,
					expect_fixed_point(assign_variable_callback(building_type_args.completion_size)),
				"max_level", ONE_EXACTLY, expect_uint(assign_variable_callback(building_type_args.max_level)),
				"goods_cost", ONE_EXACTLY, good_definition_manager.expect_good_definition_decimal_map(
					move_variable_callback(building_type_args.goods_cost)
				),
				"cost", ZERO_OR_MORE, expect_fixed_point(assign_variable_callback(building_type_args.cost)),
				"time", ONE_EXACTLY, expect_days(assign_variable_callback(building_type_args.build_time)),
				"visibility", ONE_EXACTLY, expect_bool([key](bool visibility) -> bool {
					if (!visibility) {
						Logger::warning(
							"Visibility is \"no\" for building type \"", key,
							"\", the building will still be visible as this option has no effect!"
						);
					}
					return true;
				}),
				"onmap", ONE_EXACTLY, expect_bool(assign_variable_callback(building_type_args.on_map)),
				"default_enabled", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.default_enabled)),
				"production_type", ZERO_OR_ONE, production_type_manager.expect_production_type_identifier(
					assign_variable_callback_pointer(building_type_args.production_type)
				),
				"pop_build_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.pop_build_factory)),
				"strategic_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.strategic_factory)),
				"advanced_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.advanced_factory)),
				"fort_level", ZERO_OR_ONE, expect_uint(assign_variable_callback(building_type_args.fort_level)),
				"naval_capacity", ZERO_OR_ONE, expect_uint(assign_variable_callback(building_type_args.naval_capacity)),
				"colonial_points", ZERO_OR_ONE,
					expect_list(expect_fixed_point(vector_callback(building_type_args.colonial_points))),
				"province", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.in_province)),
				"one_per_state", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.one_per_state)),
				"colonial_range", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(building_type_args.colonial_range)),
				"infrastructure", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(building_type_args.infrastructure)),
				"spawn_railway_track", ZERO_OR_ONE,
					expect_bool(assign_variable_callback(building_type_args.spawn_railway_track)),
				"sail", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.sail)),
				"steam", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.steam)),
				"capital", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.capital)),
				"port", ZERO_OR_ONE, expect_bool(assign_variable_callback(building_type_args.port))
			)(value);

			ret &= add_building_type(key, building_type_args);

			return ret;
		}
	)(root);
	lock_building_types();

	IndexedMap<BuildingType, ModifierEffectCache::building_type_effects_t>& building_type_effects =
		modifier_manager.modifier_effect_cache.building_type_effects;

	building_type_effects.set_keys(get_building_types());

	for (BuildingType const& building_type : get_building_types()) {
		using enum ModifierEffect::format_t;
		using enum ModifierEffect::target_t;

		ModifierEffectCache::building_type_effects_t& this_building_type_effects = building_type_effects[building_type];

		static constexpr std::string_view max_prefix = "max_";
		static constexpr std::string_view min_prefix = "min_build_";
		ret &= modifier_manager.register_technology_modifier_effect(
			this_building_type_effects.max_level, StringUtils::append_string_views(max_prefix, building_type.get_identifier()),
			FORMAT_x1_0DP_POS, StringUtils::append_string_views("$", building_type.get_identifier(), "$ $TECH_MAX_LEVEL$")
		);
		// TODO - add custom localisation for "min_build_$building_type$" modifiers
		ret &= modifier_manager.register_terrain_modifier_effect(
			this_building_type_effects.min_level, StringUtils::append_string_views(min_prefix, building_type.get_identifier()),
			FORMAT_x1_0DP_NEG
		);

		if (building_type.is_in_province()) {
			province_building_types.emplace_back(&building_type);
		}

		if (building_type.is_port()) {
			if (building_type.is_in_province()) {
				if (port_building_type == nullptr) {
					port_building_type = &building_type;
				} else {
					Logger::error(
						"Building type ", building_type.get_identifier(), " is marked as a port, but we are already using ",
						port_building_type->get_identifier(), " as the port building type!"
					);
					ret = false;
				}
			} else {
				Logger::error(
					"Building type ", building_type.get_identifier(), " is marked as a port, but is not a province building!"
				);
				ret = false;
			}
		}
	}

	if (port_building_type == nullptr) {
		Logger::error("No port building type found!");
		ret = false;
	}

	if (province_building_types.empty()) {
		Logger::error("No province building types found!");
		ret = false;
	} else {
		Logger::info(
			"Found ", province_building_types.size(), " province building types out of ", get_building_type_count(),
			" total building types"
		);
	}

	return ret;
}
