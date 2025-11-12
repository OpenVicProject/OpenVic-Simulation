#include "PopType.hpp"

#include <string_view>

#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/utility/TslHelper.hpp"

using namespace OpenVic;

Strata::Strata(std::string_view new_identifier, index_t new_index)
	: HasIdentifier { new_identifier }, HasIndex { new_index } {}

PopType::PopType(
	std::string_view new_identifier,
	colour_t new_colour,
	index_t new_index,
	Strata const& new_strata,
	pop_sprite_t new_sprite,
	fixed_point_map_t<GoodDefinition const*>&& new_life_needs,
	fixed_point_map_t<GoodDefinition const*>&& new_everyday_needs,
	fixed_point_map_t<GoodDefinition const*>&& new_luxury_needs,
	income_type_t new_life_needs_income_types,
	income_type_t new_everyday_needs_income_types,
	income_type_t new_luxury_needs_income_types,
	rebel_units_t&& new_rebel_units,
	pop_size_t new_max_size,
	pop_size_t new_merge_max_size,
	bool new_state_capital_only,
	bool new_demote_migrant,
	bool new_is_artisan,
	bool new_allowed_to_vote,
	bool new_is_slave,
	bool new_can_be_recruited,
	bool new_can_reduce_consciousness,
	bool new_is_administrator,
	bool new_can_invest,
	bool new_factory,
	bool new_can_work_factory,
	bool new_can_be_unemployed,
	fixed_point_t new_research_points,
	fixed_point_t new_leadership_points,
	fixed_point_t new_research_leadership_optimum,
	fixed_point_t new_state_administration_multiplier,
	PopType const* new_equivalent,
	ConditionalWeightFactorMul&& new_country_migration_target,
	ConditionalWeightFactorMul&& new_migration_target,
	poptype_weight_map_t&& new_promote_to,
	ideology_weight_map_t&& new_ideologies,
	issue_weight_map_t&& new_issues
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	HasIndex { new_index },
	strata { new_strata },
	sprite { new_sprite },
	life_needs { std::move(new_life_needs) },
	everyday_needs { std::move(new_everyday_needs) },
	luxury_needs { std::move(new_luxury_needs) },
	life_needs_income_types { new_life_needs_income_types },
	everyday_needs_income_types { new_everyday_needs_income_types },
	luxury_needs_income_types { new_luxury_needs_income_types },
	rebel_units { std::move(new_rebel_units) },
	max_size { new_max_size },
	merge_max_size { new_merge_max_size },
	state_capital_only { new_state_capital_only },
	demote_migrant { new_demote_migrant },
	is_artisan { new_is_artisan },
	allowed_to_vote { new_allowed_to_vote },
	is_slave { new_is_slave },
	can_be_recruited { new_can_be_recruited },
	can_reduce_consciousness { new_can_reduce_consciousness },
	is_administrator { new_is_administrator },
	can_invest { new_can_invest },
	factory { new_factory },
	can_work_factory { new_can_work_factory },
	can_be_unemployed { new_can_be_unemployed },
	research_points { new_research_points },
	leadership_points { new_leadership_points },
	research_leadership_optimum { new_research_leadership_optimum },
	state_administration_multiplier { new_state_administration_multiplier },
	equivalent { new_equivalent },
	country_migration_target { std::move(new_country_migration_target) },
	migration_target { std::move(new_migration_target) },
	promote_to { std::move(new_promote_to) },
	ideologies { std::move(new_ideologies) },
	issues { std::move(new_issues) } {}

bool PopType::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	ret &= country_migration_target.parse_scripts(definition_manager);
	ret &= migration_target.parse_scripts(definition_manager);
	for (auto& weight : promote_to.get_values()) {
		ret &= weight.parse_scripts(definition_manager);
	}
	for (auto& weight : ideologies.get_values()) {
		ret &= weight.parse_scripts(definition_manager);
	}
	for (auto [issue, weight] : mutable_iterator(issues)) {
		ret &= weight.parse_scripts(definition_manager);
	}
	return ret;
}
