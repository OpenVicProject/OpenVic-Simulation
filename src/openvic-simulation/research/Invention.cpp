#include "Invention.hpp"

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/military/Unit.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Invention::Invention(
	std::string_view new_identifier, ModifierValue&& new_values, bool new_news, unit_set_t&& new_activated_units,
	building_set_t&& new_activated_buildings, crime_set_t&& new_enabled_crimes, bool new_unlock_gas_attack,
	bool new_unlock_gas_defence
) : Modifier { new_identifier, std::move(new_values), 0 }, news { new_news },
	activated_units { std::move(new_activated_units) }, activated_buildings { std::move(new_activated_buildings) },
	enabled_crimes { std::move(new_enabled_crimes) }, unlock_gas_attack { new_unlock_gas_attack },
	unlock_gas_defence { new_unlock_gas_defence } {} //TODO icon

InventionManager::InventionManager() : inventions { "inventions" } {}

bool InventionManager::add_invention(
	std::string_view identifier, ModifierValue&& values, bool news, Invention::unit_set_t&& activated_units,
	Invention::building_set_t&& activated_buildings, Invention::crime_set_t&& enabled_crimes,
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
	ModifierManager const& modifier_manager, UnitManager const& unit_manager, BuildingTypeManager const& building_type_manager,
	CrimeManager const& crime_manager, ast::NodeCPtr root
) {
	return expect_dictionary_reserve_length(
		inventions, [this, &modifier_manager, &unit_manager, &building_type_manager, &crime_manager](
			std::string_view identifier, ast::NodeCPtr value) -> bool {
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
					"activate_unit", ZERO_OR_MORE, unit_manager.expect_unit_identifier(set_callback_pointer(activated_units)),
					"activate_building", ZERO_OR_MORE, building_type_manager.expect_building_type_identifier(
						set_callback_pointer(activated_buildings)
					),
					"enable_crime", ZERO_OR_ONE, crime_manager.expect_crime_modifier_identifier(
						set_callback_pointer(enabled_crimes)
					)
				)
			)(value);

			modifiers += loose_modifiers;

			ret &= add_invention(
				identifier, std::move(modifiers), news, std::move(activated_units), std::move(activated_buildings),
				std::move(enabled_crimes), unlock_gas_attack, unlock_gas_defence
			);

			return ret;
		}
	)(root);
}