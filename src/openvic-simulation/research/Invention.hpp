#pragma once

#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct Unit;
	struct BuildingType;
	struct Crime;

	struct UnitManager;
	struct BuildingTypeManager;
	struct CrimeManager;

	struct Invention : Modifier {
		friend struct InventionManager;
		//TODO implement limit and chance
		using unit_set_t = std::set<Unit const*>;
		using building_set_t = std::set<BuildingType const*>;
		using crime_set_t = std::set<Crime const*>;

	private:
		const bool PROPERTY_CUSTOM_PREFIX(news, is);
		unit_set_t PROPERTY(activated_units);
		building_set_t PROPERTY(activated_buildings);
		crime_set_t PROPERTY(enabled_crimes);
		const bool PROPERTY_CUSTOM_PREFIX(unlock_gas_attack, will);
		const bool PROPERTY_CUSTOM_PREFIX(unlock_gas_defence, will);

		Invention(
			std::string_view new_identifier, ModifierValue&& new_values, bool new_news, unit_set_t&& new_activated_units,
			building_set_t&& new_activated_buildings, crime_set_t&& new_enabled_crimes, bool new_unlock_gas_attack,
			bool new_unlock_gas_defence
		);

	public:
		Invention(Invention&&) = default;
	};

	struct InventionManager {
		IdentifierRegistry<Invention> IDENTIFIER_REGISTRY(invention);

	public:
		bool add_invention(
			std::string_view identifier, ModifierValue&& values, bool news, Invention::unit_set_t&& activated_units,
			Invention::building_set_t&& activated_buildings, Invention::crime_set_t&& enabled_crimes, bool unlock_gas_attack,
			bool unlock_gas_defence
		);

		bool load_inventions_file(
			ModifierManager const& modifier_manager, UnitManager const& unit_manager,
			BuildingTypeManager const& building_type_manager, CrimeManager const& crime_manager, ast::NodeCPtr root
		); // inventions/*.txt
	};
}