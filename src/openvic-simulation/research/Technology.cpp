#include "Technology.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

TechnologyFolder::TechnologyFolder(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

TechnologyArea::TechnologyArea(std::string_view new_identifier, TechnologyFolder const& new_folder)
	: HasIdentifier { new_identifier }, folder { new_folder } {}

Technology::Technology(
	std::string_view new_identifier, TechnologyArea const& new_area, Date::year_t new_year, fixed_point_t new_cost,
	bool new_unciv_military, uint8_t new_unit, unit_set_t&& new_activated_units, building_set_t&& new_activated_buildings,
	ModifierValue&& new_values
) : Modifier { new_identifier, std::move(new_values), 0 }, area { new_area }, year { new_year }, cost { new_cost },
	unciv_military { new_unciv_military }, unit { new_unit }, activated_buildings { std::move(new_activated_units) },
	activated_units { std::move(new_activated_buildings) } {}

TechnologySchool::TechnologySchool(std::string_view new_identifier, ModifierValue&& new_values)
	: Modifier { new_identifier, std::move(new_values), 0 } {}

TechnologyManager::TechnologyManager()
  : technology_folders { "technology folders" }, technology_areas { "technology areas" }, technologies { "technologies" },
	technology_schools { "technology schools" } {}

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

bool TechnologyManager::add_technology(
	std::string_view identifier, TechnologyArea const* area, Date::year_t year, fixed_point_t cost, bool unciv_military,
	uint8_t unit, Technology::unit_set_t&& activated_units, Technology::building_set_t&& activated_buildings,
	ModifierValue&& values
) {
	if (identifier.empty()) {
		Logger::error("Invalid technology identifier - empty!");
		return false;
	}

	if (area == nullptr) {
		Logger::error("Null area for technology \"", identifier, "\"!");
		return false;
	}

	return technologies.add_item({
		identifier, *area, year, cost, unciv_military, unit, std::move(activated_units), std::move(activated_buildings),
		std::move(values)
	});
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

bool TechnologyManager::load_technologies_file(
	ModifierManager const& modifier_manager, UnitManager const& unit_manager, BuildingTypeManager const& building_type_manager,
	ast::NodeCPtr root
) {
	return expect_dictionary_reserve_length(technologies, [this, &modifier_manager, &unit_manager, &building_type_manager](
		std::string_view tech_key, ast::NodeCPtr tech_value) -> bool {
		ModifierValue modifiers;
		TechnologyArea const* area = nullptr;
		Date::year_t year = 0;
		fixed_point_t cost = 0;
		bool unciv_military = false;
		uint8_t unit = 0;
		Technology::unit_set_t activated_units;
		Technology::building_set_t activated_buildings;

		bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(modifiers),
			"area", ONE_EXACTLY, expect_technology_area_identifier(assign_variable_callback_pointer(area)),
			"year", ONE_EXACTLY, expect_uint(assign_variable_callback(year)),
			"cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(cost)),
			"unciv_military", ZERO_OR_ONE, expect_bool(assign_variable_callback(unciv_military)),
			"unit", ZERO_OR_ONE, expect_uint(assign_variable_callback(unit)),
			"activate_unit", ZERO_OR_MORE, unit_manager.expect_unit_identifier(set_callback_pointer(activated_units)),
			"activate_building", ZERO_OR_MORE, building_type_manager.expect_building_type_identifier(
				set_callback_pointer(activated_buildings)
			),
			"ai_chance", ONE_EXACTLY, success_callback //TODO
		)(tech_value);

		ret &= add_technology(
			tech_key, area, year, cost, unciv_military, unit, std::move(activated_units), std::move(activated_buildings),
			std::move(modifiers)
		);
		return ret;
	})(root);
}

#define TECH_MODIFIER(NAME, POS) ret &= modifier_manager.add_modifier_effect(NAME, POS)
#define UNCIV_TECH_MODIFIER(NAME) TECH_MODIFIER(NAME, false); TECH_MODIFIER(StringUtils::append_string_views("self_", NAME), false);
bool TechnologyManager::generate_modifiers(ModifierManager& modifier_manager) {
	bool ret = true;

	UNCIV_TECH_MODIFIER("unciv_military_modifier");
	UNCIV_TECH_MODIFIER("unciv_economic_modifier");

	for (TechnologyFolder const& folder : get_technology_folders()) {
		TECH_MODIFIER(StringUtils::append_string_views(folder.get_identifier(), "_research_bonus"), true);
	}

	return ret;
}
#undef UNCIV_TECH_MODIFIER
#undef TECH_MODIFIER