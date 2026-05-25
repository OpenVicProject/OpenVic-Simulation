#pragma once

#include "openvic-simulation/research/Invention.hpp"
#include "openvic-simulation/scripts/ConditionalWeight.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {
	struct BuildingTypeManager;
	struct CrimeManager;
	struct InventionManager;
	struct UnitTypeManager;
	struct TechnologyManager;

	struct InventionManager {
		IdentifierRegistry<Invention> IDENTIFIER_REGISTRY(invention);

	public:
		bool add_invention(
			std::string_view identifier, ModifierValue&& values, bool news,
			std::remove_const_t<decltype(Invention::activated_regiment_types)>&& activated_regiment_types,
			std::remove_const_t<decltype(Invention::activated_ship_types)>&& activated_ship_types,
			Invention::building_set_t&& activated_buildings, Invention::crime_set_t&& enabled_crimes, bool unlock_gas_attack,
			bool unlock_gas_defence, ConditionScript&& limit, ConditionalWeightBase&& chance,
			memory::vector<memory::string>&& raw_associated_tech_identifiers
		);

		bool load_inventions_file(
			TechnologyManager const& tech_manager,
			ModifierManager const& modifier_manager, UnitTypeManager const& unit_type_manager,
			BuildingTypeManager const& building_type_manager, CrimeManager const& crime_manager, ovdl::v2script::ast::Node const* root
		); // inventions/*.txt

		bool generate_invention_links(TechnologyManager& tech_manager);

		bool parse_scripts(DefinitionManager const& definition_manager);
	};
}