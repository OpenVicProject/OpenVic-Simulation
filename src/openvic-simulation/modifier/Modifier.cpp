#include "Modifier.hpp"

using namespace OpenVic;

Modifier::Modifier(std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type)
	: HasIdentifier { new_identifier }, ModifierValue { std::move(new_values) }, type { new_type } {}

IconModifier::IconModifier(
	std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon
) : Modifier { new_identifier, std::move(new_values), new_type }, icon { new_icon } {}

TriggeredModifier::TriggeredModifier(
	std::string_view new_identifier, ModifierValue&& new_values, modifier_type_t new_type, icon_t new_icon,
	ConditionScript&& new_trigger
) : IconModifier { new_identifier, std::move(new_values), new_type, new_icon }, trigger { std::move(new_trigger) } {}

bool TriggeredModifier::parse_scripts(DefinitionManager const& definition_manager) {
	return trigger.parse_script(false, definition_manager);
}

ModifierInstance::ModifierInstance(Modifier const& new_modifier, Date new_expiry_date)
	: modifier { &new_modifier }, expiry_date { new_expiry_date } {}
