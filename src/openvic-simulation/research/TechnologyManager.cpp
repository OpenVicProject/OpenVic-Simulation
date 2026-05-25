#include "TechnologyManager.hpp"

#include "openvic-simulation/core/memory/FixedVector.hpp"
#include "openvic-simulation/dataloader/NodeTools.hpp"
#include "openvic-simulation/economy/BuildingTypeManager.hpp"
#include "openvic-simulation/military/UnitTypeManager.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

bool TechnologyManager::add_technology_folder(std::string_view identifier) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid technology folder identifier - empty!");
		return false;
	}

	return technology_folders.emplace_item(
		identifier,
		identifier, TechnologyFolder::index_t { get_technology_folder_count() }
	);
}

bool TechnologyManager::add_technology_area(std::string_view identifier, TechnologyFolder const& folder) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid technology area identifier - empty!");
		return false;
	}

	return technology_areas.emplace_item(
		identifier,
		identifier, folder
	);
}

bool TechnologyManager::add_technology(
	std::string_view identifier, TechnologyArea* area, Date::year_t year, fixed_point_t cost, bool unciv_military,
	std::optional<unit_variant_t>&& unit_variant,
	std::remove_const_t<decltype(Technology::activated_regiment_types)>&& activated_regiment_types,
	std::remove_const_t<decltype(Technology::activated_ship_types)>&& activated_ship_types,
	Technology::building_set_t&& activated_buildings, ModifierValue&& values, ConditionalWeightFactorMul&& ai_chance
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid technology identifier - empty!");
		return false;
	}

	if (area == nullptr) {
		spdlog::error_s("Null area for technology \"{}\"!", identifier);
		return false;
	}

	static constexpr size_t MAX_TECHS_IN_AREA = std::numeric_limits<Technology::area_index_t>::max() + 1;

	const size_t index_in_area = area->get_tech_count();

	if (index_in_area >= MAX_TECHS_IN_AREA) {
		spdlog::error_s(
			"Cannot add technology \"{}\" - too many technologies in area \"{}\"! Each area can have at most {} technologies.",
			identifier, *area, MAX_TECHS_IN_AREA
		);
		return false;
	}

	if (!technologies.emplace_item(
		identifier,
		identifier,
		Technology::index_t { get_technology_count() },
		*area,
		year,
		cost,
		static_cast<Technology::area_index_t>(index_in_area),
		unciv_military,
		std::move(unit_variant),
		std::move(activated_regiment_types),
		std::move(activated_ship_types),
		std::move(activated_buildings),
		std::move(values),
		std::move(ai_chance)
	)) {
		return false;
	}

	area->tech_count++;
	return true;
}

bool TechnologyManager::add_technology_school(std::string_view identifier, ModifierValue&& values) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid technology school identifier - empty!");
		return false;
	}

	return technology_schools.emplace_item(
		identifier,
		identifier, std::move(values)
	);
}

bool TechnologyManager::load_technology_file_folders_and_areas(ovdl::v2script::ast::Node const* root) {
	return expect_dictionary_keys(
		"folders", ONE_EXACTLY, [this](ovdl::v2script::ast::Node const* root_value) -> bool {
			const bool ret = expect_dictionary_reserve_length(
				technology_folders,
				[this](std::string_view folder_key, ovdl::v2script::ast::Node const* folder_value) -> bool {
					if (!add_technology_folder(folder_key)) {
						spdlog::error_s("Failed to add and retrieve technology folder: \"{}\"", folder_key);
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
	ModifierManager const& modifier_manager, ovdl::v2script::ast::Node const* root
) {
	if (!technology_folders.is_locked() || !technology_areas.is_locked()) {
		spdlog::error_s("Cannot load technology schools until technology folders and areas are locked!");
		return false;
	}
	return expect_dictionary_keys(
		/* Never fail because of "folders", even if it's missing or there are duplicate entries,
		 * those issues will have been caught by load_technology_file_folders_and_areas. */
		"folders", ZERO_OR_MORE, success_callback,
		"schools", ONE_EXACTLY, [this, &modifier_manager](ovdl::v2script::ast::Node const* root_value) -> bool {
			const bool ret = expect_dictionary_reserve_length(
				technology_schools,
				[this, &modifier_manager](std::string_view school_key, ovdl::v2script::ast::Node const* school_value) -> bool {
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
	BuildingTypeManager const& building_type_manager, ovdl::v2script::ast::Node const* root
) {
	return expect_dictionary_reserve_length(technologies, [this, &modifier_manager, &unit_type_manager, &building_type_manager](
		std::string_view tech_key, ovdl::v2script::ast::Node const* tech_value
	) -> bool {
		using enum scope_type_t;

		ModifierValue modifiers;
		TechnologyArea* area = nullptr;
		Date::year_t year = 0;
		fixed_point_t cost = 0;
		bool unciv_military = false;
		std::optional<unit_variant_t> unit_variant;

		std::remove_const_t<decltype(Technology::activated_regiment_types)> activated_regiment_types;
		assert(unit_type_manager.regiment_types_are_locked());

		std::remove_const_t<decltype(Technology::activated_ship_types)> activated_ship_types;
		assert(unit_type_manager.ship_types_are_locked());

		Technology::building_set_t activated_buildings;
		ConditionalWeightFactorMul ai_chance { COUNTRY, COUNTRY, NO_SCOPE };

		bool ret = NodeTools::expect_dictionary_keys_and_default(
			modifier_manager.expect_technology_modifier(modifiers),
			"area", ONE_EXACTLY, technology_areas.expect_item_identifier(assign_variable_callback_pointer(area)),
			"year", ONE_EXACTLY, expect_uint(assign_variable_callback(year)),
			"cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(cost)),
			"unciv_military", ZERO_OR_ONE, expect_bool(assign_variable_callback(unciv_military)),
			"unit", ZERO_OR_ONE, expect_uint<decltype(unit_variant)::value_type>(assign_variable_callback_opt(unit_variant)),
			"activate_unit", ZERO_OR_MORE,NodeTools::expect_identifier(
				[
					&activated_regiment_types, &activated_ship_types,
					&unit_type_manager
				](std::string_view identifier) -> bool {
					if (RegimentType const* const regiment_type_ptr = unit_type_manager.get_regiment_type_by_identifier(identifier);
						regiment_type_ptr != nullptr
					) {
						return (activated_regiment_types.emplace(regiment_type_ptr)).second;
					}
					if (ShipType const* const ship_type_ptr = unit_type_manager.get_ship_type_by_identifier(identifier);
						ship_type_ptr != nullptr
					) {
						return (activated_ship_types.emplace(ship_type_ptr)).second;
					}
					spdlog::error_s("Invalid regiment type or ship type identifier: {}", identifier);
					return false;
				}
			),
			"activate_building", ZERO_OR_MORE, building_type_manager.expect_building_type_identifier(
				set_callback_pointer(activated_buildings)
			),
			"ai_chance", ONE_EXACTLY, ai_chance.expect_conditional_weight()
		)(tech_value);

		ret &= add_technology(
			tech_key, area, year, cost, unciv_military, std::move(unit_variant),
			std::move(activated_regiment_types), std::move(activated_ship_types),
			std::move(activated_buildings), std::move(modifiers), std::move(ai_chance)
		);
		return ret;
	})(root);
}

bool TechnologyManager::generate_modifiers(ModifierManager& modifier_manager) const {
	using enum ModifierEffect::format_t;
	using enum ModifierEffect::target_t;

	memory::FixedVector<
		ModifierEffect const*,
		technology_folder_index_t
	>& research_bonus_effects = modifier_manager.modifier_effect_cache.research_bonus_effects;

	research_bonus_effects = std::move(
		decltype(ModifierEffectCache::research_bonus_effects) {
			generate_values,
			technology_folder_index_t(get_technology_folder_count())
		}
	);

	bool ret = true;

	for (TechnologyFolder const& folder : get_technology_folders()) {
		const memory::string modifier_identifier = memory::fmt::format("{}_research_bonus", folder);

		ret &= modifier_manager.register_base_country_modifier_effect(
			research_bonus_effects[folder.index], modifier_identifier, FORMAT_x100_1DP_PC_POS, modifier_identifier
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
		spdlog::error_s("Cannot generate technology lists until technology folders, areas, and technologies are locked!");
		return false;
	}

	for (TechnologyArea const& area : technology_areas.get_items()) {
		const_cast<TechnologyFolder&>(area.folder).technology_areas.emplace_back(area);
	}

	for (Technology const& tech : technologies.get_items()) {
		const_cast<TechnologyArea&>(tech.area).technologies.emplace_back(tech);
	}

	bool ret = true;

	for (TechnologyArea const& area : technology_areas.get_items()) {
		if (area.get_technologies().size() != area.get_tech_count()) {
			spdlog::error_s(
				"Technology area \"{}\" has a mismatch between tech count ({}) and tech list size ({})!",
				area, area.get_tech_count(), area.get_technologies().size()
			);
			ret = false;
		}
	}

	return ret;
}
