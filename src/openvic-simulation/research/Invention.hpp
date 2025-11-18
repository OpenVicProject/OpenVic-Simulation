#pragma once

#include "openvic-simulation/modifier/Modifier.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/HasIndex.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/OrderedContainers.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {

	struct BuildingType;
	struct BuildingTypeManager;
	struct Crime;
	struct CrimeManager;
	struct InventionManager;
	struct UnitType;
	struct UnitTypeManager;

	struct Invention : HasIndex<Invention, invention_index_t>, Modifier {
		friend struct InventionManager;

		//TODO implement limit and chance
		using unit_set_t = ordered_set<UnitType const*>;
		using building_set_t = ordered_set<BuildingType const*>;
		using crime_set_t = ordered_set<Crime const*>;

	private:
		const bool PROPERTY_CUSTOM_PREFIX(news, is);
		unit_set_t PROPERTY(activated_units);
		building_set_t PROPERTY(activated_buildings);
		crime_set_t PROPERTY(enabled_crimes);
		const bool PROPERTY_CUSTOM_PREFIX(unlock_gas_attack, will);
		const bool PROPERTY_CUSTOM_PREFIX(unlock_gas_defence, will);
		ConditionScript PROPERTY(limit);
		ConditionalWeightBase PROPERTY(chance);

		bool parse_scripts(DefinitionManager const& definition_manager);

	public:
		Invention(
			index_t new_index,
			std::string_view new_identifier,
			ModifierValue&& new_values,
			bool new_news,
			unit_set_t&& new_activated_units,
			building_set_t&& new_activated_buildings,
			crime_set_t&& new_enabled_crimes,
			bool new_unlock_gas_attack,
			bool new_unlock_gas_defence,
			ConditionScript&& new_limit,
			ConditionalWeightBase&& new_chance
		);
		Invention(Invention&&) = default;
	};

	struct InventionManager {
		IdentifierRegistry<Invention> IDENTIFIER_REGISTRY(invention);

	public:
		bool add_invention(
			std::string_view identifier, ModifierValue&& values, bool news, Invention::unit_set_t&& activated_units,
			Invention::building_set_t&& activated_buildings, Invention::crime_set_t&& enabled_crimes, bool unlock_gas_attack,
			bool unlock_gas_defence, ConditionScript&& limit, ConditionalWeightBase&& chance
		);

		bool load_inventions_file(
			ModifierManager const& modifier_manager, UnitTypeManager const& unit_type_manager,
			BuildingTypeManager const& building_type_manager, CrimeManager const& crime_manager, ast::NodeCPtr root
		); // inventions/*.txt

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}