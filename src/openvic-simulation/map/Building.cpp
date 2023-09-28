#include "Building.hpp"

#include "openvic-simulation/map/Province.hpp" //imported here so the hpp doesn't get circular imports

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Building::Building(std::string_view identifier, ARGS) : HasIdentifier { identifier }, type { type }, on_completion { on_completion }, 
	completion_size { completion_size }, max_level { max_level }, build_cost { build_cost }, build_time { build_time }, visibility { visibility }, on_map { on_map },
	default_enabled { default_enabled }, production_type { production_type }, pop_build_factory { pop_build_factory }, strategic_factory { strategic_factory },
	fort_level { fort_level }, naval_capacity { naval_capacity }, colonial_points { colonial_points }, in_province { in_province }, one_per_state { one_per_state },
	colonial_range { colonial_range }, infrastructure { infrastructure }, movement_cost { movement_cost }, spawn_railway_track { spawn_railway_track } {}

BuildingType const& Building::get_type() const {
	return type;
}

Building::sound_t Building::get_on_completion() const {
	return on_completion; 
}

fixed_point_t Building::get_completion_size() const {
	return completion_size;
}

Building::level_t Building::get_max_level() const {
	return max_level;
}

std::map<const Good*, fixed_point_t> const& Building::get_build_cost() const {
	return build_cost;
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

uint64_t Building::get_colonial_range() const {
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