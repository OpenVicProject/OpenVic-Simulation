#pragma once

#include <type_traits>

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

namespace OpenVic {
	struct BuildingType;
	struct Crime;
	struct InventionManager;
	struct UnitType;
	struct Technology;

	struct Invention : HasIndex<Invention, invention_index_t>, Modifier {
		friend struct InventionManager;

		//TODO implement limit and chance
		using building_set_t = ordered_set<BuildingType const*>;
		using crime_set_t = ordered_set<Crime const*>;

	private:
		ConditionScript PROPERTY(limit);
		ConditionalWeightBase PROPERTY(chance);
		
		memory::vector<memory::string> raw_associated_tech_identifiers;

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		const bool is_news;
		const ordered_set<RegimentType const*> activated_regiment_types;
		const ordered_set<ShipType const*> activated_ship_types;
		const building_set_t activated_buildings;
		const crime_set_t enabled_crimes;
		const bool unlocks_gas_attack;
		const bool unlocks_gas_defence;

		Invention(
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
		);
		Invention(Invention&&) = default;
	};
}