#include "Invention.hpp"
#include <algorithm>
#include <cstring>
#include <utility>

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Invention::Invention(
	std::string_view identifier, ModifierValue&& values, bool news, unit_set_t activated_units, building_set_t activated_buildings,
	crime_set_t enabled_crimes, bool unlock_gas_attack, bool unlock_gas_defence) 
	: Modifier { identifier, std::move(values), 0 },
		news { news }, activated_units { std::move(activated_units) }, activated_buildings { std::move(activated_buildings) },
		enabled_crimes { std::move(enabled_crimes) }, unlock_gas_attack { unlock_gas_attack }, unlock_gas_defence { unlock_gas_defence } {} //TODO icon

InventionManager::InventionManager() : inventions { "inventions" } {}

bool InventionManager::add_invention(
	std::string_view identifier, ModifierValue&& values, bool news, Invention::unit_set_t activated_units,
	Invention::building_set_t activated_buildings, Invention::crime_set_t enabled_crimes,
	bool unlock_gas_attack, bool unlock_gas_defence
) {
	if (identifier.empty()) {
		Logger::error("Invalid invention identifier - empty!");
		return false;
	}

	return inventions.add_item({
		identifier, std::move(values), news, std::move(activated_units), std::move(activated_buildings),
		std::move(enabled_crimes), unlock_gas_attack, unlock_gas_defence
	});
}

bool InventionManager::load_inventions_file(
	ModifierManager const& modifier_manager, UnitManager const& unit_manager, BuildingManager const& building_manager, ast::NodeCPtr root
) {
	return expect_dictionary_reserve_length(inventions, [this, &modifier_manager, &unit_manager, &building_manager](std::string_view identifier, ast::NodeCPtr value) -> bool {
		ModifierValue loose_modifiers;
		ModifierValue modifiers;

		Invention::unit_set_t activated_units;
		Invention::building_set_t activated_buildings;
		Invention::crime_set_t enabled_crimes;

		bool unlock_gas_attack = false;
		bool unlock_gas_defence = false;
		bool news = true; //defaults to true!

		bool ret = modifier_manager.expect_modifier_value_and_keys(move_variable_callback(loose_modifiers),
			"news", ZERO_OR_ONE, expect_bool(assign_variable_callback(news)),
			"limit", ONE_EXACTLY, success_callback,
			"chance", ONE_EXACTLY, success_callback,
			"effect", ZERO_OR_ONE, modifier_manager.expect_modifier_value_and_keys(
				move_variable_callback(modifiers),
				"gas_attack", ZERO_OR_ONE, expect_bool(assign_variable_callback(unlock_gas_attack)),
				"gas_defence", ZERO_OR_ONE, expect_bool(assign_variable_callback(unlock_gas_defence)),
				"activate_unit", ZERO_OR_MORE, unit_manager.expect_unit_identifier([this, &activated_units](Unit const& unit) -> bool {
					activated_units.insert(&unit);
					return true;
				}),
				"activate_building", ZERO_OR_MORE, building_manager.expect_building_type_identifier([this, &activated_buildings](BuildingType const& type) -> bool {
					activated_buildings.insert(&type);
					return true;
				}),
				"enable_crime", ZERO_OR_ONE, modifier_manager.expect_crime_modifier_identifier([this, &enabled_crimes](Crime const& crime) -> bool {
					enabled_crimes.insert(&crime);
					return true;
				}))
		)(value);

		modifiers += loose_modifiers;

		ret &= add_invention(
			identifier, std::move(modifiers), news, activated_units, activated_buildings, enabled_crimes,
			unlock_gas_attack, unlock_gas_defence
		);

		return ret;
	})(root);
}