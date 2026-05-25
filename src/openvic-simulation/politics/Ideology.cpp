#include "Ideology.hpp"

#include "openvic-simulation/types/Colour.hpp"

using namespace OpenVic;

IdeologyGroup::IdeologyGroup(std::string_view new_identifier) : HasIdentifier { new_identifier } {}

Ideology::Ideology(
	std::string_view new_identifier,
	index_t new_index,
	colour_t new_colour,
	IdeologyGroup const& new_group,
	bool new_is_allowed_for_uncivilised,
	bool new_can_reduce_consciousness,
	bool new_can_reduce_militancy,
	std::optional<Date> new_spawn_date,
	ConditionalWeightBase&& new_add_political_reform,
	ConditionalWeightBase&& new_remove_political_reform,
	ConditionalWeightBase&& new_add_social_reform,
	ConditionalWeightBase&& new_remove_social_reform,
	ConditionalWeightBase&& new_add_military_reform,
	ConditionalWeightBase&& new_add_economic_reform
) : HasIdentifierAndColour { new_identifier, new_colour, false },
	HasIndex { new_index },
	group { new_group },
	is_allowed_for_uncivilised { new_is_allowed_for_uncivilised },
	can_reduce_consciousness { new_can_reduce_consciousness },
	can_reduce_militancy { new_can_reduce_militancy },
	spawn_date { new_spawn_date },
	add_political_reform { std::move(new_add_political_reform) },
	remove_political_reform { std::move(new_remove_political_reform) },
	add_social_reform { std::move(new_add_social_reform) },
	remove_social_reform { std::move(new_remove_social_reform) },
	add_military_reform { std::move(new_add_military_reform) },
	add_economic_reform { std::move(new_add_economic_reform) } {}

bool Ideology::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;
	ret &= add_political_reform.parse_scripts(definition_manager);
	ret &= remove_political_reform.parse_scripts(definition_manager);
	ret &= add_social_reform.parse_scripts(definition_manager);
	ret &= remove_social_reform.parse_scripts(definition_manager);
	ret &= add_military_reform.parse_scripts(definition_manager);
	ret &= add_economic_reform.parse_scripts(definition_manager);
	return ret;
}
