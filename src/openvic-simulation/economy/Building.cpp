#include "Building.hpp"

#include "openvic-simulation/map/Province.hpp" //imported here so the hpp doesn't get circular imports

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Building::Building(
	std::string_view identifier, BuildingType const& type, ARGS
) : HasIdentifier { identifier }, type { type }, modifier { std::move(modifier) }, on_completion { on_completion },
	completion_size { completion_size }, max_level { max_level }, goods_cost { std::move(goods_cost) }, cost { cost },
	build_time { build_time }, visibility { visibility }, on_map { on_map }, default_enabled { default_enabled },
	production_type { production_type }, pop_build_factory { pop_build_factory }, strategic_factory { strategic_factory },
	advanced_factory { advanced_factory }, fort_level { fort_level }, naval_capacity { naval_capacity },
	colonial_points { std::move(colonial_points) }, in_province { in_province }, one_per_state { one_per_state },
	colonial_range { colonial_range }, infrastructure { infrastructure }, spawn_railway_track { spawn_railway_track },
	sail { sail }, steam { steam }, capital { capital }, port { port } {}

BuildingType const& Building::get_type() const {
	return type;
}

ModifierValue const& Building::get_modifier() const {
	return modifier;
}

std::string_view Building::get_on_completion() const {
	return on_completion;
}

fixed_point_t Building::get_completion_size() const {
	return completion_size;
}

Building::level_t Building::get_max_level() const {
	return max_level;
}

Good::good_map_t const& Building::get_goods_cost() const {
	return goods_cost;
}

fixed_point_t Building::get_cost() const {
	return cost;
}

Timespan Building::get_build_time() const {
	return build_time;
}

bool Building::has_visibility() const {
	return visibility;
}

bool Building::is_on_map() const {
	return on_map;
}

bool Building::is_default_enabled() const {
	return default_enabled;
}

ProductionType const* Building::get_production_type() const {
	return production_type;
}

bool Building::is_pop_built_factory() const {
	return pop_build_factory;
}

bool Building::is_strategic_factory() const {
	return strategic_factory;
}

bool Building::is_advanced_factory() const {
	return advanced_factory;
}

Building::level_t Building::get_fort_level() const {
	return fort_level;
}

uint64_t Building::get_naval_capacity() const {
	return naval_capacity;
}

std::vector<fixed_point_t> const& Building::get_colonial_points() const {
	return colonial_points;
}

bool Building::is_in_province() const {
	return in_province;
}

bool Building::is_one_per_state() const {
	return one_per_state;
}

fixed_point_t Building::get_colonial_range() const {
	return colonial_range;
}

fixed_point_t Building::get_infrastructure() const {
	return infrastructure;
}

bool Building::spawned_railway_track() const {
	return spawn_railway_track;
}

BuildingType::BuildingType(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

BuildingInstance::BuildingInstance(Building const& building)
	: HasIdentifier { building.get_identifier() }, building { building } {}

Building const& BuildingInstance::get_building() const {
	return building;
}

bool BuildingInstance::_can_expand() const {
	return level < building.get_max_level();
}

BuildingInstance::level_t BuildingInstance::get_current_level() const {
	return level;
}

ExpansionState BuildingInstance::get_expansion_state() const {
	return expansion_state;
}

Date const& BuildingInstance::get_start_date() const {
	return start;
}

Date const& BuildingInstance::get_end_date() const {
	return end;
}

float BuildingInstance::get_expansion_progress() const {
	return expansion_progress;
}

bool BuildingInstance::expand() {
	if (expansion_state == ExpansionState::CanExpand) {
		expansion_state = ExpansionState::Preparing;
		expansion_progress = 0.0f;
		return true;
	}
	return false;
}

/* REQUIREMENTS:
 * MAP-71, MAP-74, MAP-77
 */
void BuildingInstance::update_state(Date const& today) {
	switch (expansion_state) {
	case ExpansionState::Preparing:
		start = today;
		end = start + building.get_build_time();
		break;
	case ExpansionState::Expanding:
		expansion_progress = static_cast<double>(today - start) / static_cast<double>(end - start);
		break;
	default: expansion_state = _can_expand() ? ExpansionState::CanExpand : ExpansionState::CannotExpand;
	}
}

void BuildingInstance::tick(Date const& today) {
	if (expansion_state == ExpansionState::Preparing) {
		expansion_state = ExpansionState::Expanding;
	}
	if (expansion_state == ExpansionState::Expanding) {
		if (end <= today) {
			level++;
			expansion_state = ExpansionState::CannotExpand;
		}
	}
}

BuildingManager::BuildingManager() : building_types { "building types" }, buildings { "buildings" } {}

bool BuildingManager::add_building_type(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid building type identifier - empty!");
		return false;
	}
	return building_types.add_item({ identifier }, duplicate_ignore_callback);
}

bool BuildingManager::add_building(std::string_view identifier, BuildingType const* type, ARGS) {
	if (identifier.empty()) {
		Logger::error("Invalid building identifier - empty!");
		return false;
	}
	if (type == nullptr) {
		Logger::error("Invalid building type for ", identifier, ": null");
		return false;
	}

	return buildings.add_item({
		identifier, *type, std::move(modifier), on_completion, completion_size, max_level, std::move(goods_cost),
		cost, build_time, visibility, on_map, default_enabled, production_type, pop_build_factory, strategic_factory,
		advanced_factory, fort_level, naval_capacity, std::move(colonial_points), in_province, one_per_state,
		colonial_range, infrastructure, spawn_railway_track, sail, steam, capital, port
	});
}

bool BuildingManager::load_buildings_file(
	GoodManager const& good_manager, ProductionTypeManager const& production_type_manager, ModifierManager& modifier_manager,
	ast::NodeCPtr root
) {
	bool ret = expect_dictionary_reserve_length(buildings, [this](std::string_view, ast::NodeCPtr value) -> bool {
		return expect_key("type", expect_identifier(
			std::bind(&BuildingManager::add_building_type, this, std::placeholders::_1)
		))(value);
	})(root);
	lock_building_types();

	ret &= expect_dictionary(
		[this, &good_manager, &production_type_manager, &modifier_manager](std::string_view key, ast::NodeCPtr value) -> bool {
			BuildingType const* type = nullptr;
			ProductionType const* production_type = nullptr;
			std::string_view on_completion;
			fixed_point_t completion_size = 0, cost = 0, infrastructure = 0, colonial_range = 0;
			Building::level_t max_level = 0, fort_level = 0;
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
				"type", ONE_EXACTLY, expect_building_type_identifier(assign_variable_callback_pointer(type)),
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
				"colonial_points", ZERO_OR_ONE, expect_list(expect_fixed_point(
					[&colonial_points](fixed_point_t points) -> bool {
						colonial_points.push_back(points);
						return true;
					}
				)),
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

			ret &= add_building(
				key, type, std::move(modifier), on_completion, completion_size, max_level, std::move(goods_cost), cost,
				build_time, visibility, on_map, default_enabled, production_type, pop_build_factory, strategic_factory,
				advanced_factory, fort_level, naval_capacity, std::move(colonial_points), in_province, one_per_state,
				colonial_range, infrastructure, spawn_railway_track, sail, steam, capital, port
			);

			return ret;
		}
	)(root);
	lock_buildings();

	for (Building const& building : buildings.get_items()) {
		std::string max_modifier_prefix = "max_";
		std::string min_modifier_prefix = "min_build_";
		modifier_manager.add_modifier_effect(
			max_modifier_prefix.append(building.get_identifier()), true, ModifierEffect::format_t::INT
		);
		modifier_manager.add_modifier_effect(
			min_modifier_prefix.append(building.get_identifier()), false, ModifierEffect::format_t::INT
		);
	}

	return ret;
}

bool BuildingManager::generate_province_buildings(Province& province) const {
	province.reset_buildings();
	if (!building_types.is_locked()) {
		Logger::error("Cannot generate buildings until building types are locked!");
		return false;
	}
	bool ret = true;
	if (!province.get_water()) {
		for (Building const& building : buildings.get_items()) {
			ret &= province.add_building({ building });
		}
	}
	province.lock_buildings();
	return ret;
}
