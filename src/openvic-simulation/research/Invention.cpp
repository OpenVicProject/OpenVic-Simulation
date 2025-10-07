#include "Invention.hpp"

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/map/Crime.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

Invention::Invention(
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
) : HasIndex<Invention> { new_index },
	Modifier { new_identifier, std::move(new_values), modifier_type_t::INVENTION },
	news { new_news },
	activated_units { std::move(new_activated_units) },
	activated_buildings { std::move(new_activated_buildings) },
	enabled_crimes { std::move(new_enabled_crimes) },
	unlock_gas_attack { new_unlock_gas_attack },
	unlock_gas_defence { new_unlock_gas_defence },
	limit { std::move(new_limit) },
	chance { std::move(new_chance) } {}

bool Invention::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	ret &= limit.parse_script(false, definition_manager);
	ret &= chance.parse_scripts(definition_manager);

	return ret;
}

bool InventionManager::add_invention(
	std::string_view identifier, ModifierValue&& values, bool news, Invention::unit_set_t&& activated_units,
	Invention::building_set_t&& activated_buildings, Invention::crime_set_t&& enabled_crimes,
	bool unlock_gas_attack, bool unlock_gas_defence, ConditionScript&& limit, ConditionalWeightBase&& chance
) {
	if (identifier.empty()) {
		spdlog::error_s("Invalid invention identifier - empty!");
		return false;
	}

	return inventions.emplace_item(
		identifier,
		get_invention_count(), identifier,
		std::move(values), news, std::move(activated_units), std::move(activated_buildings),
		std::move(enabled_crimes), unlock_gas_attack, unlock_gas_defence, std::move(limit), std::move(chance)
	);
}

bool InventionManager::load_inventions_file(
	ModifierManager const& modifier_manager, UnitTypeManager const& unit_type_manager,
	BuildingTypeManager const& building_type_manager, CrimeManager const& crime_manager, ast::NodeCPtr root
) {
	return expect_dictionary_reserve_length(
		inventions, [this, &modifier_manager, &unit_type_manager, &building_type_manager, &crime_manager](
			std::string_view identifier, ast::NodeCPtr value
		) -> bool {
			using enum scope_type_t;

			// TODO - use the same variable for all modifiers rather than combining them at the end?
			ModifierValue loose_modifiers;
			ModifierValue modifiers;

			Invention::unit_set_t activated_units;
			Invention::building_set_t activated_buildings;
			Invention::crime_set_t enabled_crimes;

			bool unlock_gas_attack = false;
			bool unlock_gas_defence = false;
			bool news = true; //defaults to true!

			ConditionScript limit { COUNTRY, COUNTRY, NO_SCOPE };
			ConditionalWeightBase chance { COUNTRY, COUNTRY, NO_SCOPE };

			bool ret = NodeTools::expect_dictionary_keys_and_default(
				modifier_manager.expect_base_country_modifier(loose_modifiers),
				"news", ZERO_OR_ONE, expect_bool(assign_variable_callback(news)),
				"limit", ONE_EXACTLY, limit.expect_script(),
				"chance", ONE_EXACTLY, chance.expect_conditional_weight(),
				"effect", ZERO_OR_ONE, NodeTools::expect_dictionary_keys_and_default(
					modifier_manager.expect_technology_modifier(modifiers),
					"gas_attack", ZERO_OR_ONE, expect_bool(assign_variable_callback(unlock_gas_attack)),
					"gas_defence", ZERO_OR_ONE, expect_bool(assign_variable_callback(unlock_gas_defence)),
					"activate_unit", ZERO_OR_MORE,
						unit_type_manager.expect_unit_type_identifier(set_callback_pointer(activated_units)),
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
				std::move(enabled_crimes), unlock_gas_attack, unlock_gas_defence, std::move(limit), std::move(chance)
			);

			return ret;
		}
	)(root);
}

bool InventionManager::parse_scripts(DefinitionManager const& definition_manager) {
	bool ret = true;

	for (Invention& invention : inventions.get_items()) {
		ret &= invention.parse_scripts(definition_manager);
	}

	return ret;
}
