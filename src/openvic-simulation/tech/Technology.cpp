#include "Technology.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

TechnologyFolder::TechnologyFolder(std::string_view identifier) : HasIdentifier { identifier } {}

TechnologyArea::TechnologyArea(std::string_view identifier, TechnologyFolder const& folder) : HasIdentifier { identifier }, folder { folder } {}

Technology::Technology(std::string_view identifier, TechnologyArea const& area, year_t year, fixed_point_t cost, bool unciv_military, uint8_t unit, unit_set_t activated_units, building_set_t activated_buildings, ModifierValue&& values)
	: Modifier { identifier, std::move(values), 0 }, area { area }, year { year }, cost { cost }, unciv_military { unciv_military }, unit { unit }, activated_buildings { std::move(activated_units) }, activated_units { std::move(activated_buildings) } {}

TechnologySchool::TechnologySchool(std::string_view new_identifier, ModifierValue&& new_values) 
	: Modifier { new_identifier, std::move(new_values), 0 } {} //TODO: school icon

TechnologyManager::TechnologyManager() : technology_folders { "technology folders" }, technology_areas { "technology areas" }, 
	technologies { "technologies" }, technology_schools { "technology schools" } {}

bool TechnologyManager::add_technology_folder(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid technology folder identifier - empty!");
		return false;
	}

	return technology_folders.add_item({ identifier });
}

bool TechnologyManager::add_technology_area(std::string_view identifier, TechnologyFolder const* folder) {
	if (identifier.empty()) {
		Logger::error("Invalid technology area identifier - empty!");
		return false;
	}

	if (folder == nullptr) {
		Logger::error("Null folder for technology area \"", identifier, "\"!");
		return false;
	}

	return technology_areas.add_item({ identifier, *folder });
}

bool TechnologyManager::add_technology(std::string_view identifier, TechnologyArea const* area, Technology::year_t year, fixed_point_t cost, bool unciv_military, uint8_t unit, Technology::unit_set_t activated_units, Technology::building_set_t activated_buildings, ModifierValue&& values) {
	if (identifier.empty()) {
		Logger::error("Invalid technology identifier - empty!");
		return false;
	}

	if (area == nullptr) {
		Logger::error("Null area for technology \"", identifier, "\"!");
		return false;
	}

	return technologies.add_item({ identifier, *area, year, cost, unciv_military, unit, activated_units, activated_buildings, std::move(values) });
}

bool TechnologyManager::add_technology_school(std::string_view identifier, ModifierValue&& values) {
	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}

	return technology_schools.add_item({ identifier, std::move(values) });
}

bool TechnologyManager::load_technology_file_areas(ast::NodeCPtr root) {
	return expect_dictionary_reserve_length(technology_folders, [this](std::string_view root_key, ast::NodeCPtr root_value) -> bool {
		if (root_key == "folders") {
			return expect_dictionary([this](std::string_view folder_key, ast::NodeCPtr folder_value) -> bool {
				bool ret = add_technology_folder(folder_key);
				TechnologyFolder const* current_folder = get_technology_folder_by_identifier(folder_key);
				if (current_folder == nullptr)
					return false;
				ret &= expect_list(expect_identifier([this, current_folder](std::string_view area) -> bool {
					return add_technology_area(area, current_folder);
				}))(folder_value);
				return ret;
			})(root_value);
			lock_technology_folders();
			lock_technology_areas();
		} else if (root_key == "schools") return true; //ignore
		else return false;
	})(root);
}

bool TechnologyManager::load_technology_file_schools(ModifierManager const& modifier_manager, ast::NodeCPtr root) {
	return expect_dictionary([this, &modifier_manager](std::string_view root_key, ast::NodeCPtr root_value) -> bool {
		if (root_key == "schools") {
			return expect_dictionary_reserve_length(technology_schools, [this, &modifier_manager](std::string_view school_key, ast::NodeCPtr school_value) -> bool {
				ModifierValue modifiers;
				bool ret = modifier_manager.expect_modifier_value(move_variable_callback(modifiers))(school_value);
				ret &= add_technology_school(school_key, std::move(modifiers));
				return ret;
			})(root_value);
			lock_technology_schools();
		} else if (root_key == "folders") return true; //ignore
		else return false;
	})(root);
}

bool TechnologyManager::load_technologies_file(ModifierManager const& modifier_manager, UnitManager const& unit_manager, BuildingManager const& building_manager, ast::NodeCPtr root) {
	return expect_dictionary_reserve_length(technologies, [this, &modifier_manager, &unit_manager, &building_manager](std::string_view tech_key, ast::NodeCPtr tech_value) -> bool {
		ModifierValue modifiers;
		std::string_view area_identifier;
		Technology::year_t year;
		fixed_point_t cost;
		bool unciv_military = false;
		uint8_t unit = 0; 
		Technology::unit_set_t activated_units;
		Technology::building_set_t activated_buildings;
		
		bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(modifiers),
			"area", ONE_EXACTLY, expect_identifier(assign_variable_callback(area_identifier)),
			"year", ONE_EXACTLY, expect_uint(assign_variable_callback(year)),
			"cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(cost)),
			"unciv_military", ZERO_OR_ONE, expect_bool(assign_variable_callback(unciv_military)),
			"unit", ZERO_OR_ONE, expect_uint(assign_variable_callback(unit)),
			"activate_unit", ZERO_OR_MORE, expect_identifier([this, &unit_manager, &activated_units](std::string_view identifier) -> bool {
				activated_units.insert(unit_manager.get_unit_by_identifier(identifier));
				return true;
			}),
			"activate_building", ZERO_OR_MORE, expect_identifier([this, &building_manager, &activated_buildings](std::string_view identifier) -> bool {
				activated_buildings.insert(building_manager.get_building_type_by_identifier(identifier));
				return true;
			}),
			"ai_chance", ONE_EXACTLY, expect_dictionary([this](std::string_view identifier, ast::NodeCPtr value) -> bool { return true; }) //TODO
		)(tech_value);

		TechnologyArea const* area = get_technology_area_by_identifier(area_identifier);

		ret &= add_technology(tech_key, area, year, cost, unciv_military, unit, activated_units, activated_buildings, std::move(modifiers));
		return ret;
	})(root);
}

bool TechnologyManager::generate_modifiers(ModifierManager& modifier_manager) {
	bool ret = true;

	ret &= modifier_manager.add_modifier_effect("unciv_military_modifier", true, ModifierEffect::format_t::PROPORTION_DECIMAL);
	ret &= modifier_manager.add_modifier_effect("unciv_economic_modifier", true, ModifierEffect::format_t::PROPORTION_DECIMAL);

	for (TechnologyFolder const& folder : get_technology_folders()) {
		std::string folder_name = std::string(folder.get_identifier()) + "_research_bonus";
		ret &= modifier_manager.add_modifier_effect(folder_name, true, ModifierEffect::format_t::PROPORTION_DECIMAL);
	}
	return ret;
}