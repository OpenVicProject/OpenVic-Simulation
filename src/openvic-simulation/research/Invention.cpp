#include "Invention.hpp"

using namespace OpenVic;

Invention::Invention(
	index_t new_index,
	std::string_view new_identifier,
	ModifierValue&& new_values,
	bool new_is_news,
	std::remove_const_t<decltype(activated_regiment_types)>&& new_activated_regiment_types,
	std::remove_const_t<decltype(activated_ship_types)>&& new_activated_ship_types,
	building_set_t&& new_activated_buildings,
	crime_set_t&& new_enabled_crimes,
	bool new_unlocks_gas_attack,
	bool new_unlocks_gas_defence,
	ConditionScript&& new_limit,
	ConditionalWeightBase&& new_chance,
	memory::vector<memory::string>&& new_raw_associated_tech_identifiers
) : HasIndex { new_index },
	Modifier { new_identifier, std::move(new_values), modifier_type_t::INVENTION },
	is_news { new_is_news },
	activated_regiment_types { std::move(new_activated_regiment_types) },
	activated_ship_types { std::move(new_activated_ship_types) },
	activated_buildings { std::move(new_activated_buildings) },
	enabled_crimes { std::move(new_enabled_crimes) },
	unlocks_gas_attack { new_unlocks_gas_attack },
	unlocks_gas_defence { new_unlocks_gas_defence },
	limit { std::move(new_limit) },
	chance { std::move(new_chance) },
	raw_associated_tech_identifiers { std::move(new_raw_associated_tech_identifiers) } {}

bool Invention::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	ret &= limit.parse_script(false, definition_manager);
	ret &= chance.parse_scripts(definition_manager);

	return ret;
}
