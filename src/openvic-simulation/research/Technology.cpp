#include "Technology.hpp"

#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

TechnologyFolder::TechnologyFolder(std::string_view new_identifier, index_t new_index)
	: HasIdentifier { new_identifier }, HasIndex { new_index } {}

TechnologyArea::TechnologyArea(std::string_view new_identifier, TechnologyFolder const& new_folder)
	: HasIdentifier { new_identifier }, folder { new_folder } {}

Technology::Technology(
	std::string_view new_identifier,
	TechnologyArea const& new_area,
	Date::year_t new_year,
	fixed_point_t new_cost,
	area_index_t new_index_in_area,
	bool new_unciv_military,
	std::optional<CountryInstance::unit_variant_t>&& new_unit_variant,
	unit_set_t&& new_activated_units,
	building_set_t&& new_activated_buildings,
	ModifierValue&& new_values,
	ConditionalWeightFactorMul&& new_ai_chance
) : Modifier { new_identifier, std::move(new_values), modifier_type_t::TECHNOLOGY },
	area { new_area },
	year { new_year },
	cost { new_cost },
	index_in_area { new_index_in_area },
	unciv_military { new_unciv_military },
	unit_variant { std::move(new_unit_variant) },
	activated_units { std::move(new_activated_units) },
	activated_buildings { std::move(new_activated_buildings) },
	ai_chance { std::move(new_ai_chance) } {}

bool Technology::parse_scripts(DefinitionManager const& definition_manager) {
	return ai_chance.parse_scripts(definition_manager);
}

TechnologySchool::TechnologySchool(std::string_view new_identifier, ModifierValue&& new_values)
	: Modifier { new_identifier, std::move(new_values), modifier_type_t::TECH_SCHOOL } {}

bool TechnologyManager::add_technology_folder(std::string_view identifier) {
	if (identifier.empty()) {
		Logger::error("Invalid technology folder identifier - empty!");
		return false;
	}

	return technology_folders.add_item({ identifier, get_technology_folder_count() });
}

bool TechnologyManager::add_technology_area(std::string_view identifier, TechnologyFolder const& folder) {
	if (identifier.empty()) {
		Logger::error("Invalid technology area identifier - empty!");
		return false;
	}

	return technology_areas.add_item({ identifier, folder });
}

bool TechnologyManager::add_technology(
	std::string_view identifier, TechnologyArea* area, Date::year_t year, fixed_point_t cost, bool unciv_military,
	std::optional<CountryInstance::unit_variant_t>&& unit_variant, Technology::unit_set_t&& activated_units,
	Technology::building_set_t&& activated_buildings, ModifierValue&& values, ConditionalWeightFactorMul&& ai_chance
) {
	if (identifier.empty()) {
		Logger::error("Invalid technology identifier - empty!");
		return false;
	}

	if (area == nullptr) {
		Logger::error("Null area for technology \"", identifier, "\"!");
		return false;
	}

	static constexpr size_t MAX_TECHS_IN_AREA = std::numeric_limits<Technology::area_index_t>::max() + 1;

	const size_t index_in_area = area->get_tech_count();

	if (index_in_area >= MAX_TECHS_IN_AREA) {
		Logger::error(
			"Cannot add technology \"", identifier, "\" - too many technologies in area \"", area->get_identifier(),
			"\"! Each area can have at most ", MAX_TECHS_IN_AREA, " technologies."
		);
		return false;
	}

	if (technologies.add_item({
		identifier,
		*area,
		year,
		cost,
		static_cast<Technology::area_index_t>(index_in_area),
		unciv_military,
		std::move(unit_variant),
		std::move(activated_units),
		std::move(activated_buildings),
		std::move(values),
		std::move(ai_chance)
	})) {
		area->tech_count++;
		return true;
	} else {
		return false;
	}
}

bool TechnologyManager::add_technology_school(std::string_view identifier, ModifierValue&& values) {
	if (identifier.empty()) {
		Logger::error("Invalid modifier effect identifier - empty!");
		return false;
	}

	return technology_schools.add_item({ identifier, std::move(values) });
}

bool TechnologyManager::load_technology_file_folders_and_areas(ast::NodeCPtr root) {
	return expect_dictionary_keys(
		"folders", ONE_EXACTLY, [this](ast::NodeCPtr root_value) -> bool {
			const bool ret = expect_dictionary_reserve_length(
				technology_folders,
				[this](std::string_view folder_key, ast::NodeCPtr folder_value) -> bool {
					if (!add_technology_folder(folder_key)) {
						Logger::error("Failed to add and retrieve technology folder: \"", folder_key, "\"");
						return false;
					}

					TechnologyFolder const& current_folder = get_back_technology_folder();

					return expect_list_reserve_length(
						technology_areas,
						expect_identifier(
							[this, &current_folder](std::string_view identifier) -> bool {
								return add_technology_area(identifier, current_folder);
							}
						)
					)(folder_value);
				}
			)(root_value);

			lock_technology_folders();
			lock_technology_areas();

			return ret;
		},
		/* Never fail because of "schools", even if it's missing or there are duplicate entries,
		 * those issues will be caught by load_technology_file_schools. */
		"schools", ZERO_OR_MORE, success_callback
	)(root);
}

bool TechnologyManager::load_technology_file_schools(
	ModifierManager const& modifier_manager, ast::NodeCPtr root
) {
	if (!technology_folders.is_locked() || !technology_areas.is_locked()) {
		Logger::error("Cannot load technology schools until technology folders and areas are locked!");
		return false;
	}
	return expect_dictionary_keys(
		/* Never fail because of "folders", even if it's missing or there are duplicate entries,
		 * those issues will have been caught by load_technology_file_folders_and_areas. */
		"folders", ZERO_OR_MORE, success_callback,
		"schools", ONE_EXACTLY, [this, &modifier_manager](ast::NodeCPtr root_value) -> bool {
			const bool ret = expect_dictionary_reserve_length(
				technology_schools,
				[this, &modifier_manager](std::string_view school_key, ast::NodeCPtr school_value) -> bool {
					ModifierValue modifiers;

					bool ret = NodeTools::expect_dictionary(
						modifier_manager.expect_base_country_modifier(modifiers)
					)(school_value);

					ret &= add_technology_school(school_key, std::move(modifiers));

					return ret;
				}
			)(root_value);

			lock_technology_schools();

			return ret;
		}
	)(root);
}

bool TechnologyManager::load_technologies_file(
	ModifierManager const& modifier_manager, UnitTypeManager const& unit_type_manager,
	BuildingTypeManager const& building_type_manager, ast::NodeCPtr root
) {
	return expect_dictionary_reserve_length(technologies, [this, &modifier_manager, &unit_type_manager, &building_type_manager](
		std::string_view tech_key, ast::NodeCPtr tech_value
	) -> bool {
		using enum scope_type_t;

		ModifierValue modifiers;
		TechnologyArea* area = nullptr;
		Date::year_t year = 0;
		fixed_point_t cost = 0;
		bool unciv_military = false;
		std::optional<CountryInstance::unit_variant_t> unit_variant;
		Technology::unit_set_t activated_units;
		Technology::building_set_t activated_buildings;
		ConditionalWeightFactorMul ai_chance { COUNTRY, COUNTRY, NO_SCOPE };

		bool ret = NodeTools::expect_dictionary_keys_and_default(
			modifier_manager.expect_technology_modifier(modifiers),
			"area", ONE_EXACTLY, technology_areas.expect_item_identifier(assign_variable_callback_pointer(area)),
			"year", ONE_EXACTLY, expect_uint(assign_variable_callback(year)),
			"cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(cost)),
			"unciv_military", ZERO_OR_ONE, expect_bool(assign_variable_callback(unciv_military)),
			"unit", ZERO_OR_ONE, expect_uint<decltype(unit_variant)::value_type>(assign_variable_callback_opt(unit_variant)),
			"activate_unit", ZERO_OR_MORE,
				unit_type_manager.expect_unit_type_identifier(set_callback_pointer(activated_units)),
			"activate_building", ZERO_OR_MORE, building_type_manager.expect_building_type_identifier(
				set_callback_pointer(activated_buildings)
			),
			"ai_chance", ONE_EXACTLY, ai_chance.expect_conditional_weight()
		)(tech_value);

		ret &= add_technology(
			tech_key, area, year, cost, unciv_military, std::move(unit_variant), std::move(activated_units),
			std::move(activated_buildings), std::move(modifiers), std::move(ai_chance)
		);
		return ret;
	})(root);
}

bool TechnologyManager::generate_modifiers(ModifierManager& modifier_manager) const {
	using enum ModifierEffect::format_t;
	using enum ModifierEffect::target_t;

	IndexedMap<TechnologyFolder, ModifierEffect const*>& research_bonus_effects =
		modifier_manager.modifier_effect_cache.research_bonus_effects;

	research_bonus_effects.set_keys(&get_technology_folders());

	bool ret = true;

	for (TechnologyFolder const& folder : get_technology_folders()) {
		const std::string modifier_identifier = StringUtils::append_string_views(folder.get_identifier(), "_research_bonus");

		ret &= modifier_manager.register_base_country_modifier_effect(
			research_bonus_effects[folder], modifier_identifier, true, PROPORTION_DECIMAL, modifier_identifier
		);
	}

	return ret;
}

bool TechnologyManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	for (Technology& technology : technologies.get_items()) {
		ret &= technology.parse_scripts(definition_manager);
	}

	return ret;
}

bool TechnologyManager::generate_technology_lists() {
	if (!technology_folders.is_locked() || !technology_areas.is_locked() || !technologies.is_locked()) {
		Logger::error("Cannot generate technology lists until technology folders, areas, and technologies are locked!");
		return false;
	}

	for (TechnologyArea const& area : technology_areas.get_items()) {
		const_cast<TechnologyFolder&>(area.folder).technology_areas.push_back(&area);
	}

	for (Technology const& tech : technologies.get_items()) {
		const_cast<TechnologyArea&>(tech.area).technologies.push_back(&tech);
	}

	bool ret = true;

	for (TechnologyArea const& area : technology_areas.get_items()) {
		if (area.get_technologies().size() != area.get_tech_count()) {
			Logger::error(
				"Technology area \"", area.get_identifier(), "\" has a mismatch between tech count (", area.get_tech_count(),
				") and tech list size (", area.get_technologies().size(), ")!"
			);
			ret = false;
		}
	}

	return ret;
}
