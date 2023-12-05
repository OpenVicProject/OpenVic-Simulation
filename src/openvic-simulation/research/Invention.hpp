#pragma once

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/military/Unit.hpp"
#include "openvic-simulation/misc/Modifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include <string_view>
#include <unordered_set>

namespace OpenVic {
	struct Invention : Modifier {
		friend struct InventionManager;
		//TODO implement limit and chance
		using unit_set_t = std::unordered_set<Unit const*>;
		using building_set_t = std::unordered_set<BuildingType const*>;
		using crime_set_t = std::unordered_set<Crime const*>;

	private:
		const bool PROPERTY_CUSTOM_PREFIX(news, is);
		unit_set_t PROPERTY(activated_units);
		building_set_t PROPERTY(activated_buildings);
		crime_set_t PROPERTY(enabled_crimes);
		const bool PROPERTY_CUSTOM_PREFIX(unlock_gas_attack, will);
		const bool PROPERTY_CUSTOM_PREFIX(unlock_gas_defence, will);

		Invention(
			std::string_view identifier, ModifierValue&& values, bool news, unit_set_t activated_units,
			building_set_t activated_buildings, crime_set_t enabled_crimes, bool unlock_gas_attack,
			bool unlock_gas_defence
		);

	public:
		Invention(Invention&&) = default;
	};

	struct InventionManager {
		IdentifierRegistry<Invention> inventions;

	public:
		InventionManager();

		bool add_invention(
			std::string_view identifier, ModifierValue&& values, bool news, Invention::unit_set_t activated_units,
			Invention::building_set_t activated_buildings, Invention::crime_set_t enabled_crimes, bool unlock_gas_attack,
			bool unlock_gas_defence
		);

		IDENTIFIER_REGISTRY_ACCESSORS(invention);

		bool load_inventions_file(
			ModifierManager const& modifier_manager, UnitManager const& unit_manager, BuildingManager const& building_manager,
			ast::NodeCPtr root
		); // inventions/*.txt
	};
}