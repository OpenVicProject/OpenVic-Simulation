#include "Technology.hpp"

using namespace OpenVic;

TechnologyFolder::TechnologyFolder(std::string_view new_identifier, index_t new_index)
	: HasIdentifier { new_identifier }, HasIndex { new_index } {}

TechnologyArea::TechnologyArea(std::string_view new_identifier, TechnologyFolder const& new_folder)
	: HasIdentifier { new_identifier }, folder { new_folder } {}

Technology::Technology(
	std::string_view new_identifier,
	index_t new_index,
	TechnologyArea const& new_area,
	Date::year_t new_year,
	fixed_point_t new_cost,
	area_index_t new_index_in_area,
	bool new_unciv_military,
	std::optional<unit_variant_t>&& new_unit_variant,
	std::remove_const_t<decltype(activated_regiment_types)>&& new_activated_regiment_types,
	std::remove_const_t<decltype(activated_ship_types)>&& new_activated_ship_types,
	building_set_t&& new_activated_buildings,
	ModifierValue&& new_values,
	ConditionalWeightFactorMul&& new_ai_chance
) : Modifier { new_identifier, std::move(new_values), modifier_type_t::TECHNOLOGY },
	HasIndex { new_index },
	area { new_area },
	year { new_year },
	cost { new_cost },
	index_in_area { new_index_in_area },
	unciv_military { new_unciv_military },
	unit_variant { std::move(new_unit_variant) },
	activated_regiment_types { std::move(new_activated_regiment_types) },
	activated_ship_types { std::move(new_activated_ship_types) },
	activated_buildings { std::move(new_activated_buildings) },
	ai_chance { std::move(new_ai_chance) } {}

bool Technology::parse_scripts(DefinitionManager const& definition_manager) {
	return ai_chance.parse_scripts(definition_manager);
}

TechnologySchool::TechnologySchool(std::string_view new_identifier, ModifierValue&& new_values)
	: Modifier { new_identifier, std::move(new_values), modifier_type_t::TECH_SCHOOL } {}
