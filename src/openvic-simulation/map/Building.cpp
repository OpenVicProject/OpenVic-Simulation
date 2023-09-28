#include "Building.hpp"

#include "openvic-simulation/map/Province.hpp" //imported here so the hpp doesn't get circular imports

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Building::Building(std::string_view identifier, ARGS) : HasIdentifier { identifier }, ModifierValue { std::move(modifiers) }, type { type },
	on_completion { on_completion }, completion_size { completion_size }, max_level { max_level }, goods_cost { goods_cost }, cost { cost }, build_time { build_time },
	visibility { visibility }, on_map { on_map }, default_enabled { default_enabled }, production_type { production_type }, pop_build_factory { pop_build_factory },
	strategic_factory { strategic_factory }, advanced_factory { advanced_factory }, fort_level { fort_level }, naval_capacity { naval_capacity },
	colonial_points { colonial_points }, in_province { in_province }, one_per_state { one_per_state }, colonial_range { colonial_range },
	infrastructure { infrastructure }, movement_cost { movement_cost }, local_ship_build { local_ship_build }, spawn_railway_track { spawn_railway_track },
	sail { sail }, steam { steam }, capital { capital }, port { port } {}

BuildingType const& Building::get_type() const {
	return type;
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

std::map<const Good*, fixed_point_t> const& Building::get_goods_cost() const {
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

std::vector<uint64_t> const& Building::get_colonial_points() const {
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

fixed_point_t Building::get_movement_cost() const {
	return movement_cost;
}

bool Building::spawned_railway_track() const {
	return spawn_railway_track;
}

BuildingType::BuildingType(const std::string_view new_identifier) : HasIdentifier { new_identifier } {}

BuildingInstance::BuildingInstance(Building const& building) : HasIdentifier { building.get_identifier() }, building { building } {}

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

bool BuildingManager::add_building_type(const std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid building type identifier - empty!");
		return false;
	}
	return building_types.add_item({ identifier });
}

bool BuildingManager::add_building(std::string_view identifier, ARGS) {
	if (identifier.empty()) {
		Logger::error("Invalid building identifier - empty!");
		return false;
	}

	return buildings.add_item({
		identifier, type, on_completion, completion_size, max_level, goods_cost, cost, build_time, visibility, on_map, default_enabled,
		production_type, pop_build_factory, strategic_factory, advanced_factory, fort_level, naval_capacity, colonial_points, in_province, one_per_state, 
		colonial_range, infrastructure, movement_cost, local_ship_build, spawn_railway_track, sail, steam, capital, port, std::move(modifiers)
	});
}

bool BuildingManager::load_buildings_file(GoodManager const& good_manager, ProductionTypeManager const& production_type_manager, ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	bool ret = expect_dictionary([this, &good_manager, &production_type_manager, &modifier_manager](std::string_view key, ast::NodeCPtr value) -> bool {
		std::string_view type, on_completion = "", production_type;
		fixed_point_t completion_size = 0, cost = 0, infrastructure = 0, movement_cost = 0, colonial_range = 0, local_ship_build = 0;
		Building::level_t max_level, fort_level = 0;
		std::map<const Good*, fixed_point_t> goods_cost;
		uint32_t build_days;
		bool visibility, on_map, default_enabled, pop_build_factory, strategic_factory, advanced_factory, in_province, one_per_state, spawn_railway_track, sail, steam, capital, port;
		default_enabled = pop_build_factory = strategic_factory = advanced_factory = in_province = one_per_state = spawn_railway_track = sail = steam = capital = port = false;
		uint64_t naval_capacity = 0;
		std::vector<uint64_t> colonial_points;
		ModifierValue modifiers;
		
		bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(modifiers),
			"type", ONE_EXACTLY, expect_identifier(assign_variable_callback(type)),
			"on_completion", ZERO_OR_ONE, expect_identifier(assign_variable_callback(on_completion)),
			"completion_size", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(completion_size)),
			"max_level", ONE_EXACTLY, expect_uint(assign_variable_callback(max_level)),
			"goods_cost", ONE_EXACTLY, good_manager.expect_good_decimal_map(assign_variable_callback(goods_cost)),
			"cost", ZERO_OR_MORE, expect_fixed_point(assign_variable_callback(cost)),
			"time", ONE_EXACTLY, expect_uint(assign_variable_callback_uint("building build time", build_days)),
			"visibility", ONE_EXACTLY, expect_bool(assign_variable_callback(visibility)),
			"onmap", ONE_EXACTLY, expect_bool(assign_variable_callback(on_map)),
			"default_enabled", ZERO_OR_ONE, expect_bool(assign_variable_callback(default_enabled)),
			"production_type", ZERO_OR_ONE, expect_identifier(assign_variable_callback(production_type)),
			"pop_build_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(pop_build_factory)),
			"strategic_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(strategic_factory)),
			"advanced_factory", ZERO_OR_ONE, expect_bool(assign_variable_callback(advanced_factory)),
			"fort_level", ZERO_OR_ONE, expect_uint(assign_variable_callback_uint("building fort level", fort_level)),
			"naval_capacity", ZERO_OR_ONE, expect_uint(assign_variable_callback_uint("building naval capacity", naval_capacity)),
			"colonial_points", ZERO_OR_ONE, expect_list(expect_uint([&colonial_points](uint64_t points) -> bool {
				return colonial_points.emplace_back(points);
			})), "province", ZERO_OR_ONE, expect_bool(assign_variable_callback(in_province)),
			"one_per_state", ZERO_OR_ONE, expect_bool(assign_variable_callback(one_per_state)),
			"colonial_range", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(colonial_range)),
			"infrastructure", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(infrastructure)),
			"movement_cost", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(movement_cost)),
			"local_ship_build", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(local_ship_build)),
			"spawn_railway_track", ZERO_OR_ONE, expect_bool(assign_variable_callback(spawn_railway_track)),
			"sail", ZERO_OR_ONE, expect_bool(assign_variable_callback(sail)),
			"steam", ZERO_OR_ONE, expect_bool(assign_variable_callback(steam)),
			"capital", ZERO_OR_ONE, expect_bool(assign_variable_callback(capital)),
			"port", ZERO_OR_ONE, expect_bool(assign_variable_callback(port))
		)(value);

		BuildingType const* type_ref = building_types.get_item_by_identifier(type);
		if (type_ref == nullptr) {
			building_types.add_item({ type });
			type_ref = building_types.get_item_by_identifier(type);
		}

		Timespan build_time = Timespan(build_days);

		ProductionType const* production_type_ref = production_type_manager.get_production_type_by_identifier(production_type);

		ret &= add_building(
			key, *type_ref, on_completion, completion_size, max_level, goods_cost, cost, build_time, visibility, on_map, default_enabled,
			production_type_ref, pop_build_factory, strategic_factory, advanced_factory, fort_level, naval_capacity, colonial_points, in_province,
			one_per_state, colonial_range, infrastructure, movement_cost, local_ship_build, spawn_railway_track, sail, steam, capital, port, std::move(modifiers)
		);

		return ret;
	})(root);
	lock_building_types();
	lock_buildings();

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
