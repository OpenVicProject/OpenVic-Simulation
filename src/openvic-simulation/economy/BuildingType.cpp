#include "BuildingType.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

BuildingType::BuildingType(
	std::string_view identifier, ARGS
) : HasIdentifier { identifier }, type { type }, modifier { std::move(modifier) }, on_completion { on_completion },
	completion_size { completion_size }, max_level { max_level }, goods_cost { std::move(goods_cost) }, cost { cost },
	build_time { build_time }, visibility { visibility }, on_map { on_map }, default_enabled { default_enabled },
	production_type { production_type }, pop_build_factory { pop_build_factory }, strategic_factory { strategic_factory },
	advanced_factory { advanced_factory }, fort_level { fort_level }, naval_capacity { naval_capacity },
	colonial_points { std::move(colonial_points) }, in_province { in_province }, one_per_state { one_per_state },
	colonial_range { colonial_range }, infrastructure { infrastructure }, spawn_railway_track { spawn_railway_track },
	sail { sail }, steam { steam }, capital { capital }, port { port } {}

BuildingTypeManager::BuildingTypeManager() : building_types { "building types" } {}

bool BuildingTypeManager::add_building_type(std::string_view identifier, ARGS) {
	if (identifier.empty()) {
		Logger::error("Invalid building identifier - empty!");
		return false;
	}

	return building_types.add_item({
		identifier, type, std::move(modifier), on_completion, completion_size, max_level, std::move(goods_cost),
		cost, build_time, visibility, on_map, default_enabled, production_type, pop_build_factory, strategic_factory,
		advanced_factory, fort_level, naval_capacity, std::move(colonial_points), in_province, one_per_state,
		colonial_range, infrastructure, spawn_railway_track, sail, steam, capital, port
	});
}

bool BuildingTypeManager::load_buildings_file(
	GoodManager const& good_manager, ProductionTypeManager const& production_type_manager, ModifierManager& modifier_manager,
	ast::NodeCPtr root
) {
	const bool ret = expect_dictionary_reserve_length(
		building_types,
		[this, &good_manager, &production_type_manager, &modifier_manager](std::string_view key, ast::NodeCPtr value) -> bool {
			std::string_view type;
			ProductionType const* production_type = nullptr;
			std::string_view on_completion;
			fixed_point_t completion_size = 0, cost = 0, infrastructure = 0, colonial_range = 0;
			BuildingType::level_t max_level = 0, fort_level = 0;
			Good::good_map_t goods_cost;
			Timespan build_time;
			bool visibility = false, on_map = false, default_enabled = false, pop_build_factory = false;
			bool strategic_factory = false, advanced_factory = false;
			bool in_province = false, one_per_state = false, spawn_railway_track = false, sail = false, steam = false;
			bool capital = false, port = false;
			uint64_t naval_capacity = 0;
			std::vector<fixed_point_t> colonial_points;
			ModifierValue modifier;

			bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(modifier),
				"type", ONE_EXACTLY, expect_identifier(assign_variable_callback(type)),
				"on_completion", ZERO_OR_ONE, expect_identifier(assign_variable_callback(on_completion)),
				"completion_size", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(completion_size)),
				"max_level", ONE_EXACTLY, expect_uint(assign_variable_callback(max_level)),
				"goods_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(move_variable_callback(goods_cost)),
				"cost", ZERO_OR_MORE, expect_fixed_point(assign_variable_callback(cost)),
				"time", ONE_EXACTLY, expect_days(assign_variable_callback(build_time)),
				"visibility", ONE_EXACTLY, expect_bool(assign_variable_callback(visibility)),
				"onmap", ONE_EXACTLY, expect_bool(assign_variable_callback(on_map)),
				"default_enabled", ZERO_OR_ONE, expect_bool(assign_variable_callback(default_enabled)),
				"production_type", ZERO_OR_ONE, production_type_manager.expect_production_type_identifier(
					assign_variable_callback_pointer(production_type)),
				"pop_build_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(pop_build_factory)),
				"strategic_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(strategic_factory)),
				"advanced_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(advanced_factory)),
				"fort_level", ZERO_OR_ONE, expect_uint(assign_variable_callback(fort_level)),
				"naval_capacity", ZERO_OR_ONE, expect_uint(assign_variable_callback(naval_capacity)),
				"colonial_points", ZERO_OR_ONE, expect_list(expect_fixed_point(vector_callback(colonial_points))),
				"province", ZERO_OR_ONE, expect_bool(assign_variable_callback(in_province)),
				"one_per_state", ZERO_OR_ONE, expect_bool(assign_variable_callback(one_per_state)),
				"colonial_range", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(colonial_range)),
				"infrastructure", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(infrastructure)),
				"spawn_railway_track", ZERO_OR_ONE, expect_bool(assign_variable_callback(spawn_railway_track)),
				"sail", ZERO_OR_ONE, expect_bool(assign_variable_callback(sail)),
				"steam", ZERO_OR_ONE, expect_bool(assign_variable_callback(steam)),
				"capital", ZERO_OR_ONE, expect_bool(assign_variable_callback(capital)),
				"port", ZERO_OR_ONE, expect_bool(assign_variable_callback(port))
			)(value);

			ret &= add_building_type(
				key, type, std::move(modifier), on_completion, completion_size, max_level, std::move(goods_cost), cost,
				build_time, visibility, on_map, default_enabled, production_type, pop_build_factory, strategic_factory,
				advanced_factory, fort_level, naval_capacity, std::move(colonial_points), in_province, one_per_state,
				colonial_range, infrastructure, spawn_railway_track, sail, steam, capital, port
			);

			return ret;
		}
	)(root);
	lock_building_types();

	for (BuildingType const& building_type : building_types.get_items()) {
		std::string max_modifier_prefix = "max_";
		std::string min_modifier_prefix = "min_build_";
		modifier_manager.add_modifier_effect(
			max_modifier_prefix.append(building_type.get_identifier()), true, ModifierEffect::format_t::INT
		);
		modifier_manager.add_modifier_effect(
			min_modifier_prefix.append(building_type.get_identifier()), false, ModifierEffect::format_t::INT
		);
	}

	return ret;
}
